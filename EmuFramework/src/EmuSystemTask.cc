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
#include <imagine/logger/logger.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideo.hh>
#include "EmuSystemTask.hh"
#include "privateInput.hh"

void EmuSystemTask::start()
{
	if(started)
		return;
	replyPort.attach(
		[this](auto msgs)
		{
			for(auto msg : msgs)
			{
				switch(msg.reply)
				{
					bcase Reply::VIDEO_FORMAT_CHANGED:
					{
						msg.args.videoFormat.videoAddr->setFormat(msg.args.videoFormat.desc);
						auto semPtr = msg.semPtr;
						if(semPtr)
						{
							semPtr->notify();
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
			commandPort.attach(eventLoop,
				[this](auto msgs)
				{
					for(auto msg : msgs)
					{
						switch(msg.command)
						{
							bcase Command::RUN_FRAME:
							{
								//logMsg("got draw command");
								auto frames = msg.args.run.frames;
								assumeExpr(frames);
								auto *video = msg.args.run.video;
								auto *audio = msg.args.run.audio;
								if(unlikely(msg.args.run.skipForward))
								{
									if(EmuSystem::skipForwardFrames(this, frames - 1))
									{
										// don't write any audio while skip is in progress
										audio = nullptr;
									}
									else
									{
										// restore normal speed when skip ends
										EmuSystem::setSpeedMultiplier(1);
									}
								}
								else
								{
									EmuSystem::skipFrames(this, frames - 1, audio);
								}
								turboActions.update();
								EmuSystem::runFrame(this, video, audio);
							}
							bcase Command::PAUSE:
							{
								//logMsg("got pause command");
								assumeExpr(msg.semPtr);
								msg.semPtr->notify();
							}
							bcase Command::EXIT:
							{
								//logMsg("got exit command");
								started = false;
								Base::EventLoop::forThread().stop();
								assumeExpr(msg.semPtr);
								msg.semPtr->notify();
								return false;
							}
							bdefault:
							{
								logWarn("unknown CommandMessage value:%d", (int)msg.command);
							}
						}
					}
					return true;
				});
			started = true;
			sem.notify();
			logMsg("starting emu system thread event loop");
			eventLoop.run(started);
			logMsg("emu system thread exit");
			commandPort.detach();
		});
}

void EmuSystemTask::pause()
{
	if(!started)
		return;
	commandPort.send({Command::PAUSE}, true);
}

void EmuSystemTask::stop()
{
	if(!started)
		return;
	commandPort.send({Command::EXIT}, true);
	replyPort.clear();
	replyPort.detach();
}

void EmuSystemTask::runFrame(EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward)
{
	assumeExpr(frames);
	if(unlikely(!started))
		return;
	commandPort.send({Command::RUN_FRAME, video, audio, frames, skipForward});
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video, IG::PixmapDesc desc)
{
	replyPort.send({Reply::VIDEO_FORMAT_CHANGED, video, desc}, true);
}

void EmuSystemTask::sendScreenshotReply(int num, bool success)
{
	replyPort.send({Reply::TOOK_SCREENSHOT, num, success});
}
