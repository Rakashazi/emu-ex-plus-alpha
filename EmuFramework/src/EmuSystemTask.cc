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

namespace EmuEx
{

EmuSystemTask::EmuSystemTask(EmuApp &app):
	appPtr{&app}
{}

void EmuSystemTask::start()
{
	if(taskThread.joinable())
		return;
	taskThread = IG::makeThreadSync(
		[this](auto &sem)
		{
			auto eventLoop = IG::EventLoop::makeForThread();
			bool started = true;
			commandPort.attach(eventLoop,
				[this, &started](auto msgs)
				{
					for(auto msg : msgs)
					{
						switch(msg.command)
						{
							case Command::RUN_FRAME:
							{
								auto frames = msg.args.run.frames;
								assumeExpr(frames);
								//logMsg("running %d frame(s)", frames);
								app().runFrames({this, msg.semPtr}, msg.args.run.video, msg.args.run.audio,
									frames, msg.args.run.skipForward);
								break;
							}
							case Command::PAUSE:
							{
								//logMsg("got pause command");
								assumeExpr(msg.semPtr);
								msg.semPtr->release();
								break;
							}
							case Command::EXIT:
							{
								//logMsg("got exit command");
								started = false;
								IG::EventLoop::forThread().stop();
								return false;
							}
							default:
							{
								logWarn("unknown CommandMessage value:%d", (int)msg.command);
							}
						}
					}
					return true;
				});
			sem.release();
			logMsg("starting thread event loop");
			eventLoop.run(started);
			logMsg("exiting thread");
			commandPort.detach();
		});
}

void EmuSystemTask::pause()
{
	if(!taskThread.joinable())
		return;
	commandPort.send({Command::PAUSE}, true);
	app().flushMainThreadMessages();
}

void EmuSystemTask::stop()
{
	if(!taskThread.joinable())
		return;
	commandPort.send({Command::EXIT});
	taskThread.join();
	app().flushMainThreadMessages();
}

void EmuSystemTask::runFrame(EmuVideo *video, EmuAudio *audio, int8_t frames, bool skipForward, bool runSync)
{
	assumeExpr(frames);
	if(!taskThread.joinable()) [[unlikely]]
		return;
	commandPort.send({Command::RUN_FRAME, video, audio, frames, skipForward}, runSync);
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video, std::binary_semaphore *frameFinishedSemPtr)
{
	if(frameFinishedSemPtr)
	{
		videoFormatChanged = true;
	}
	else
	{
		app().runOnMainThread(
			[&video](IG::ApplicationContext)
			{
				video.dispatchFormatChanged();
			});
	}
}

void EmuSystemTask::sendFrameFinishedReply(EmuVideo &video, std::binary_semaphore *frameFinishedSemPtr)
{
	if(frameFinishedSemPtr)
	{
		frameFinishedSemPtr->release(); // main thread continues execution
	}
	else
	{
		app().runOnMainThread(
			[&video](IG::ApplicationContext)
			{
				video.dispatchFrameFinished();
			});
	}
}

void EmuSystemTask::sendScreenshotReply(bool success)
{
	app().runOnMainThread(
		[=](IG::ApplicationContext ctx)
		{
			EmuApp::get(ctx).printScreenshotResult(success);
		});
}

EmuApp &EmuSystemTask::app() const
{
	return *appPtr;
}

}
