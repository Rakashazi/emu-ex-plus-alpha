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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuSystemTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"EmuSystemTask"};

EmuSystemTask::EmuSystemTask(EmuApp &app):
	app{app} {}

using FrameParamsCommand = EmuSystemTask::FrameParamsCommand;
using PauseCommand = EmuSystemTask::PauseCommand;
using ExitCommand = EmuSystemTask::ExitCommand;

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
				FrameParams frameParams{};
				for(auto msg : msgs)
				{
					bool threadIsRunning = visit(overloaded
					{
						[&](FrameParamsCommand &cmd)
						{
							frameParams = cmd.params;
							return true;
						},
						[&](PauseCommand &)
						{
							//log.debug("got pause command");
							frameParams = {};
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
				if(hasTime(frameParams.timestamp))
					app.advanceFrames(frameParams, this);
				return true;
			});
			sem.release();
			log.info("starting thread event loop");
			eventLoop.run(started);
			log.info("exiting thread");
			commandPort.detach();
		});
}

void EmuSystemTask::pause()
{
	if(!taskThread.joinable())
		return;
	commandPort.send({.command = PauseCommand{}}, MessageReplyMode::wait);
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

void EmuSystemTask::updateFrameParams(FrameParams params)
{
	if(!taskThread.joinable()) [[unlikely]]
		return;
	commandPort.send({.command = FrameParamsCommand{params}});
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video)
{
	app.runOnMainThread([&video](ApplicationContext)
	{
		video.dispatchFormatChanged();
	});
}

void EmuSystemTask::sendFrameFinishedReply(EmuVideo &video)
{
	video.dispatchFrameFinished();
}

void EmuSystemTask::sendScreenshotReply(bool success)
{
	app.runOnMainThread([&app = app, success](ApplicationContext ctx)
	{
		app.printScreenshotResult(success);
	});
}

}
