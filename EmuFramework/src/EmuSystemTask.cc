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
	taskThread = makeThreadSync(
		[this](auto &sem)
		{
			auto eventLoop = IG::EventLoop::makeForThread();
			bool started = true;
			commandPort.attach(eventLoop,
				[this, &started](auto msgs)
				{
					for(auto msg : msgs)
					{
						bool threadIsRunning = visit(overloaded
						{
							[&](RunFrameCommand &run)
							{
								assumeExpr(run.frames);
								//logMsg("running %d frame(s)", run.frames);
								app().runFrames({this, msg.semPtr}, run.video, run.audio,
									run.frames, run.skipForward);
								return true;
							},
							[&](PauseCommand &)
							{
								//logMsg("got pause command");
								assumeExpr(msg.semPtr);
								msg.semPtr->release();
								return true;
							},
							[&](ExitCommand &)
							{
								started = false;
								EventLoop::forThread().stop();
								return false;
							},
						}, msg.command);
						if(!threadIsRunning)
							return false;
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
	commandPort.send({.command = PauseCommand{}}, true);
	app().flushMainThreadMessages();
}

void EmuSystemTask::stop()
{
	if(!taskThread.joinable())
		return;
	commandPort.send({.command = ExitCommand{}});
	taskThread.join();
	app().flushMainThreadMessages();
}

void EmuSystemTask::runFrame(EmuVideo *video, EmuAudio *audio, int8_t frames, bool skipForward, bool runSync)
{
	assumeExpr(frames);
	if(!taskThread.joinable()) [[unlikely]]
		return;
	commandPort.send({.command = RunFrameCommand{video, audio, frames, skipForward}}, runSync);
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
