/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "GLRenderer"
#include <assert.h>
#include <imagine/gfx/Renderer.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "private.hh"

namespace Gfx
{

void GLMainTask::TaskContext::notifySemaphore()
{
	assumeExpr(semPtr);
	semPtr->notify();
	*notifySemaporeAfterDelegatePtr = false;
}

GLMainTask::~GLMainTask()
{
	stop();
}

void GLMainTask::start(Base::GLContext context)
{
	if(started)
		return;
	thread = IG::makeThreadSync(
		[this, context](auto &sem)
		{
			auto glDpy = Base::GLDisplay::getDefault(glAPI);
			assumeExpr(glDpy);
			context.setCurrent(glDpy, context, {});
			auto eventLoop = Base::EventLoop::makeForThread();
			commandPort.attach(eventLoop,
				[this, glDpy](auto msgs)
				{
					for(auto msg : msgs)
					{
						switch(msg.command)
						{
							bcase Command::RUN_FUNC:
							{
								bool notifySemaporeAfterDelegate = msg.semPtr;
								TaskContext ctx{msg.semPtr, &notifySemaporeAfterDelegate};
								msg.args.run.func(ctx);
								if(notifySemaporeAfterDelegate)
								{
									//logDMsg("notifying semaphore:%p", msg.semPtr);
									msg.semPtr->notify();
								}
							}
							bcase Command::EXIT:
							{
								Base::GLContext::setCurrent(glDpy, {}, {});
								started = false;
								Base::EventLoop::forThread().stop();
								return false;
							}
							bdefault:
							{
								logWarn("unknown ThreadCommandMessage value:%d", (int)msg.command);
							}
						}
					}
					return true;
				});
			started = true;
			sem.notify();
			logMsg("starting main GL thread event loop");
			eventLoop.run(started);
			commandPort.detach();
			logMsg("main GL thread exit");
		});
}

void GLMainTask::runFunc(FuncDelegate del, bool awaitReply)
{
	assert(started);
	commandPort.send({Command::RUN_FUNC, del}, awaitReply);
}

void GLMainTask::stop()
{
	if(!started)
		return;
	commandPort.send({Command::EXIT});
	thread.join(); // GL implementation may assign thread destructor so must join() to make sure it completes
}

bool GLMainTask::isStarted() const
{
	return started;
}

void GLMainTask::runPendingTasksOnThisThread()
{
	assert(started);
	commandPort.dispatchMessages();
}

}
