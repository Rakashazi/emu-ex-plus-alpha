/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/thread/Thread.hh>
#include "EmuOptions.hh"
#include "private.hh"
#include "privateInput.hh"

void EmuSystemTask::start()
{
	if(started)
		return;
	replyPort.addToEventLoop({},
		[this](auto msgs)
		{
			for(auto msg = msgs.get(); msg; msg = msgs.get())
			{
				switch(msg.reply)
				{
					bcase Reply::VIDEO_FORMAT_CHANGED:
					{
						emuVideo.setFormat(msg.args.videoFormat.desc);
						auto semAddr = msg.args.videoFormat.semAddr;
						if(semAddr)
						{
							semAddr->notify();
						}
					}
					bcase Reply::TOOK_SCREENSHOT:
					{
						EmuApp::printScreenshotResult(msg.args.screenshot.num, msg.args.screenshot.success);
					}
					bdefault:
					{
						logErr("unknown reply message:%d", (int)msg.reply);
					}
				}
			}
			return true;
		});
	IG::makeDetachedThreadSync(
		[this](auto &sem)
		{
			auto eventLoop = Base::EventLoop::makeForThread();
			commandPort.addToEventLoop(eventLoop,
				[this](auto messages)
				{
					Base::FrameTimeBase timestamp{};
					IG::Semaphore *notifySemAddr{};
					bool notifyAfterFrame = false;
					for(auto msg = messages.get(); msg; msg = messages.get())
					{
						switch(msg.command)
						{
							bcase Command::RUN_FRAME:
							{
								//logMsg("got draw command");
								timestamp = msg.args.run.timestamp;
							}
							bcase Command::PAUSE:
							{
								//logMsg("got pause command");
								assumeExpr(!notifySemAddr);
								notifySemAddr = msg.semAddr;
								assumeExpr(notifySemAddr);
							}
							bcase Command::NOTIFY_AFTER_FRAME:
							{
								//logMsg("got notify after frame command");
								assumeExpr(!notifySemAddr);
								notifySemAddr = msg.semAddr;
								notifyAfterFrame = true;
								assumeExpr(notifySemAddr);
							}
							bcase Command::EXIT:
							{
								//logMsg("got exit command");
								Base::EventLoop::forThread().stop();
								assumeExpr(msg.semAddr);
								msg.semAddr->notify();
								return false;
							}
							bdefault:
							{
								logWarn("unknown CommandMessage value:%d", (int)msg.command);
							}
						}
					}
					if(unlikely(notifySemAddr && (!notifyAfterFrame || !timestamp)))
					{
						notifySemAddr->notify();
						return true;
					}
					if(!timestamp)
						return true;
					bool doFrame = false;
					if(unlikely(fastForwardActive || EmuSystem::shouldFastForward()))
					{
						startVideoFrame();
						doFrame = true;
						if(fastForwardActive)
						{
							EmuSystem::skipFrames(this, (uint)optionFastForwardSpeed, true);
						}
						else
						{
							iterateTimes((uint)optionFastForwardSpeed, i)
							{
								EmuSystem::skipFrames(this, 1, true);
								if(!EmuSystem::shouldFastForward())
								{
									logMsg("fast-forward ended early after %d frame(s)", i);
									break;
								}
							}
						}
					}
					else
					{
						uint frames = EmuSystem::advanceFramesWithTime(timestamp);
						//logDMsg("%d frames elapsed (%fs)", frames, Base::frameTimeBaseToSecsDec(params.timestampDiff()));
						if(frames)
						{
							startVideoFrame();
							doFrame = true;
							constexpr uint maxLateFrameSkip = 6;
							uint maxFrameSkip = optionSkipLateFrames ? maxLateFrameSkip : 0;
							#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
							if(!optionSkipLateFrames)
								maxFrameSkip = optionFrameInterval - 1;
							#endif
							assumeExpr(maxFrameSkip <= maxLateFrameSkip);
							if(frames > 1 && maxFrameSkip)
							{
								//logDMsg("running %d frames", frames);
								uint framesToSkip = frames - 1;
								framesToSkip = std::min(framesToSkip, maxFrameSkip);
								bool renderAudio = optionSound;
								iterateTimes(framesToSkip, i)
								{
									turboActions.update();
									EmuSystem::runFrame(this, nullptr, renderAudio);
								}
							}
						}
					}
					if(doFrame)
					{
						bool renderAudio = optionSound;
						turboActions.update();
						EmuSystem::runFrame(this, &emuVideo, renderAudio);
					}
					if(unlikely(notifySemAddr))
					{
						notifySemAddr->notify();
					}
					return true;
				});
			sem.notify();
			logMsg("starting emu system thread event loop");
			eventLoop.run();
			logMsg("emu system thread exit");
			commandPort.removeFromEventLoop();
		});
	started = true;
}

void EmuSystemTask::pause()
{
	if(!started)
		return;
	IG::Semaphore sem{0};
	commandPort.send({Command::PAUSE, &sem});
	sem.wait();
	emuVideo.waitAsyncFrame();
}

void EmuSystemTask::stop()
{
	if(!started)
		return;
	IG::Semaphore sem{0};
	commandPort.send({Command::EXIT, &sem});
	sem.wait();
	replyPort.clear();
	replyPort.removeFromEventLoop();
	emuVideo.waitAsyncFrame();
	started = false;
	assert(!doingVideoFrame);
}

void EmuSystemTask::runFrame(Base::FrameTimeBase timestamp)
{
	if(unlikely(!started))
		return;
	commandPort.send({Command::RUN_FRAME, timestamp});
}

void EmuSystemTask::waitForFinishedFrame()
{
	if(unlikely(!started))
		return;
	IG::Semaphore sem{0};
	commandPort.send({Command::NOTIFY_AFTER_FRAME, &sem});
	sem.wait();
}

bool EmuSystemTask::videoFrameIsInProgress() const
{
	return doingVideoFrame;
}

void EmuSystemTask::startVideoFrame()
{
	doingVideoFrame = true;
}

void EmuSystemTask::finishVideoFrame()
{
	doingVideoFrame = false;
}

void EmuSystemTask::setFastForwardActive(bool active)
{
	fastForwardActive = active;
}

void EmuSystemTask::sendVideoFormatChangedReply(IG::PixmapDesc desc, IG::Semaphore *semAddr)
{
	replyPort.send({Reply::VIDEO_FORMAT_CHANGED, desc, semAddr});
}

void EmuSystemTask::sendScreenshotReply(int num, bool success)
{
	replyPort.send({Reply::TOOK_SCREENSHOT, num, success});
}
