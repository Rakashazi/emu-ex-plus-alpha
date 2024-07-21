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

void EmuSystemTask::start(Window& win)
{
	if(taskThread.joinable())
		return;
	win.setDrawEventPriority(Window::drawEventPriorityLocked); // block UI from posting draws
	winPtr = &win;
	taskThread = makeThreadSync(
		[this](auto &sem)
		{
			threadId_ = thisThreadId();
			auto eventLoop = EventLoop::makeForThread();
			winPtr->setFrameEventsOnThisThread();
			app.addOnFrameDelayed();
			bool started = true;
			commandPort.attach(eventLoop, [this, &started](auto msgs)
			{
				for(auto msg : msgs)
				{
					bool threadIsRunning = msg.command.visit(overloaded
					{
						[&](SetWindowCommand &cmd)
						{
							//log.debug("got set window command");
							cmd.winPtr->moveOnFrame(*winPtr, app.system().onFrameUpdate, app.frameTimeSource);
							winPtr = cmd.winPtr;
							assumeExpr(msg.semPtr);
							msg.semPtr->release();
							suspendSem.acquire();
							return true;
						},
						[&](SuspendCommand &)
						{
							//log.debug("got suspend command");
							isSuspended = true;
							assumeExpr(msg.semPtr);
							msg.semPtr->release();
							suspendSem.acquire();
							return true;
						},
						[&](ExitCommand &)
						{
							started = false;
							app.removeOnFrame();
							winPtr->removeFrameEvents();
							threadId_ = 0;
							EventLoop::forThread().stop();
							return false;
						},
					});
					if(!threadIsRunning)
						return false;
				}
				return true;
			});
			sem.release();
			log.info("starting thread event loop");
			eventLoop.run(started);
			log.info("exiting thread");
			commandPort.detach();
		});
}

EmuSystemTask::SuspendContext EmuSystemTask::setWindow(Window& win)
{
	assert(!isSuspended);
	if(!taskThread.joinable())
		return {};
	auto oldWinPtr = winPtr;
	commandPort.send({.command = SetWindowCommand{&win}}, MessageReplyMode::wait);
	oldWinPtr->setFrameEventsOnThisThread();
	oldWinPtr->setDrawEventPriority(); // allow UI to post draws again
	app.flushMainThreadMessages();
	return {this};
}

EmuSystemTask::SuspendContext EmuSystemTask::suspend()
{
	if(!taskThread.joinable() || isSuspended)
		return {};
	commandPort.send({.command = SuspendCommand{}}, MessageReplyMode::wait);
	app.flushMainThreadMessages();
	return {this};
}

void EmuSystemTask::resume()
{
	if(!taskThread.joinable() || !isSuspended)
		return;
	suspendSem.release();
}

void EmuSystemTask::stop()
{
	if(!taskThread.joinable())
		return;
	assert(threadId_ != thisThreadId());
	commandPort.send({.command = ExitCommand{}});
	taskThread.join();
	winPtr->setFrameEventsOnThisThread();
	winPtr->setDrawEventPriority(); // allow UI to post draws again
	app.flushMainThreadMessages();
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video)
{
	video.dispatchFormatChanged();
}

void EmuSystemTask::sendFrameFinishedReply(EmuVideo &video)
{
	video.dispatchFrameFinished();
}

void EmuSystemTask::sendScreenshotReply(bool success)
{
	app.runOnMainThread([&app = app, success](ApplicationContext)
	{
		app.printScreenshotResult(success);
	});
}

}
