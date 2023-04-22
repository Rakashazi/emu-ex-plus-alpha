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
	app{app} {}

static void releaseAndSet(std::binary_semaphore *semPtr, std::binary_semaphore *newSemPtr)
{
	if(semPtr)
		semPtr->release();
	semPtr = newSemPtr;
}

void EmuSystemTask::start()
{
	if(taskThread.joinable())
		return;
	taskThread = makeThreadSync(
		[this](auto &sem)
		{
			threadId_ = thisThreadId();
			auto eventLoop = EventLoop::makeForThread();
			bool started = true;
			commandPort.attach(eventLoop, [this, &started](auto msgs)
			{
				std::binary_semaphore *semPtr{}; // semaphore of the most recent run frame command, if in sync mode
				RunFrameCommand runCmd{};
				for(auto msg : msgs)
				{
					bool threadIsRunning = visit(overloaded
					{
						[&](RunFrameCommand &run)
						{
							releaseAndSet(semPtr, msg.semPtr);
							runCmd.video = run.video;
							runCmd.audio = run.audio;
							// accumulate the total frames from all commands in queue
							constexpr int maxFrameSkip = 20;
							runCmd.frames = std::min(int(runCmd.frames + run.frames), maxFrameSkip);
							runCmd.skipForward = run.skipForward;
							return true;
						},
						[&](PauseCommand &)
						{
							//logMsg("got pause command");
							releaseAndSet(semPtr, {});
							runCmd.frames = 0;
							assumeExpr(msg.semPtr);
							msg.semPtr->release();
							return true;
						},
						[&](ExitCommand &)
						{
							releaseAndSet(semPtr, {});
							started = false;
							EventLoop::forThread().stop();
							return false;
						},
					}, msg.command);
					if(!threadIsRunning)
						return false;
				}
				if(!runCmd.frames)
					return true;
				assumeExpr(runCmd.frames > 0);
				//logMsg("running %d frame(s)", runCmd.frames);
				app.runFrames({this, semPtr}, runCmd.video, runCmd.audio,
					runCmd.frames, runCmd.skipForward);
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
	app.flushMainThreadMessages();
}

void EmuSystemTask::stop()
{
	if(!taskThread.joinable())
		return;
	commandPort.send({.command = ExitCommand{}});
	taskThread.join();
	threadId_ = 0;
	app.flushMainThreadMessages();
}

void EmuSystemTask::runFrame(EmuVideo *video, EmuAudio *audio, int8_t frames, bool skipForward, bool runSync)
{
	assumeExpr(frames > 0);
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
		app.runOnMainThread([&video](ApplicationContext)
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
		video.dispatchFrameFinished();
	}
}

void EmuSystemTask::sendScreenshotReply(bool success)
{
	app.runOnMainThread([&app = app, success](ApplicationContext ctx)
	{
		app.printScreenshotResult(success);
	});
}

}
