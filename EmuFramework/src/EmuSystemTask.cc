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

#define LOGTAG "EmuSystemTask"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuSystemTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>

EmuSystemTask::EmuSystemTask(EmuApp &app):
	appPtr{&app}
{}

void EmuSystemTask::start()
{
	if(started)
		return;
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
								auto frames = msg.args.run.frames;
								assumeExpr(frames);
								auto *video = msg.args.run.video;
								auto *audio = msg.args.run.audio;
								//logMsg("running %d frame(s)", frames);
								auto &emuApp = app();
								if(msg.args.run.skipForward) [[unlikely]]
								{
									if(emuApp.skipForwardFrames(this, frames - 1))
									{
										// don't write any audio while skip is in progress
										audio = nullptr;
									}
									else
									{
										// restore normal speed when skip ends
										EmuSystem::setSpeedMultiplier(*audio, 1);
									}
								}
								else
								{
									emuApp.skipFrames(this, frames - 1, audio);
								}
								emuApp.runTurboInputEvents();
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
			logMsg("starting thread event loop");
			eventLoop.run(started);
			logMsg("exiting thread");
			commandPort.detach();
		});
}

void EmuSystemTask::pause()
{
	if(!started)
		return;
	commandPort.send({Command::PAUSE}, true);
	app().flushMainThreadMessages();
}

void EmuSystemTask::stop()
{
	if(!started)
		return;
	commandPort.send({Command::EXIT}, true);
	app().flushMainThreadMessages();
}

void EmuSystemTask::runFrame(EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward)
{
	assumeExpr(frames);
	if(!started) [[unlikely]]
		return;
	commandPort.send({Command::RUN_FRAME, video, audio, frames, skipForward});
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video)
{
	app().runOnMainThread(
		[&video](Base::ApplicationContext)
		{
			video.dispatchFormatChanged();
		});
}

void EmuSystemTask::sendFrameFinishedReply(EmuVideo &video)
{
	app().runOnMainThread(
		[&video](Base::ApplicationContext)
		{
			video.dispatchFrameFinished();
		});
}

void EmuSystemTask::sendScreenshotReply(int num, bool success)
{
	app().runOnMainThread(
		[=](Base::ApplicationContext ctx)
		{
			EmuApp::get(ctx).printScreenshotResult(num, success);
		});
}

EmuApp &EmuSystemTask::app() const
{
	return *appPtr;
}
