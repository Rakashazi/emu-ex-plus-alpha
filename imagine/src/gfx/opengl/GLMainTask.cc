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
#include <imagine/gfx/Gfx.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "private.hh"

namespace Gfx
{

GLMainTask::~GLMainTask()
{
	stop();
}

void GLMainTask::start(Base::GLContext context)
{
	if(started)
		return;
	IG::makeDetachedThreadSync(
		[this, context](auto &sem)
		{
			auto glDpy = Base::GLDisplay::getDefault(glAPI);
			assumeExpr(glDpy);
			context.setCurrent(glDpy, context, {});
			auto eventLoop = Base::EventLoop::makeForThread();
			commandPort.attach(eventLoop,
				[this, glDpy](auto msgs)
				{
					for(auto msg = msgs.get(); msg; msg = msgs.get())
					{
						switch(msg.command)
						{
							bcase Command::RUN_FUNC:
							{
								TaskContext ctx{};
								msg.args.run.func(ctx);
								if(msg.semAddr)
								{
									//logDMsg("notifying semaphore:%p", msg.semAddr);
									msg.semAddr->notify();
								}
							}
							bcase Command::EXIT:
							{
								Base::GLContext::setCurrent(glDpy, {}, {});
								started = false;
								Base::EventLoop::forThread().stop();
								if(msg.semAddr)
									msg.semAddr->notify();
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

void GLMainTask::runFunc(FuncDelegate del, IG::Semaphore *semAddr)
{
	assert(started);
	commandPort.send({Command::RUN_FUNC, del, semAddr});
}

void GLMainTask::runFuncSync(FuncDelegate del)
{
	IG::Semaphore sem{0};
	runFunc(del, &sem);
	//logDMsg("waiting for semaphore:%p", &sem);
	sem.wait();
}

void GLMainTask::stop()
{
	if(!started)
		return;
	IG::Semaphore sem{0};
	commandPort.send({Command::EXIT, &sem});
	sem.wait();
}

bool GLMainTask::isStarted() const
{
	return started;
}

}
