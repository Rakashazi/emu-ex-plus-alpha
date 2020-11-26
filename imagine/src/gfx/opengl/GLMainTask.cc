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
#include <imagine/base/Base.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/opengl/GLRendererTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "private.hh"

namespace Gfx
{

GLMainTask::GLMainTask() {}

GLMainTask::GLMainTask(const char *debugLabel, Base::GLContext context_, int threadPriority):
	context{context_}, commandPort{debugLabel}
{
	assert(context);
	thread = IG::makeThreadSync(
		[this, threadPriority](auto &sem)
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
								TaskContext ctx{glDpy, msg.semPtr, nullptr};
								msg.args.run.func(ctx);
							}
							bcase Command::RUN_FUNC_SEMAPHORE:
							{
								bool semaphoreWasNotifiedPtr = !msg.semPtr;
								TaskContext ctx{glDpy, msg.semPtr, &semaphoreWasNotifiedPtr};
								msg.args.run.func(ctx);
								if(!semaphoreWasNotifiedPtr) // semaphore wasn't already notified in the delegate
								{
									//logDMsg("notifying semaphore:%p", msg.semPtr);
									msg.semPtr->notify();
								}
							}
							bcase Command::EXIT:
							{
								Base::GLContext::setCurrent(glDpy, {}, {});
								logMsg("exiting GL context:%p thread", context.nativeObject());
								context.deinit(glDpy);
								context = {};
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
			sem.notify();
			logMsg("starting GL context:%p thread event loop", context.nativeObject());
			if(threadPriority)
				Base::setThisThreadPriority(threadPriority);
			eventLoop.run(context);
			commandPort.detach();
		});
	onExit =
		[this](bool backgrounded)
		{
			if(backgrounded)
			{
				run(
					[]()
					{
						#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
						glReleaseShaderCompiler();
						#endif
						glFinish();
					}, true);
			}
			else
			{
				deinit();
			}
			return true;
		};
	Base::addOnExit(onExit, Base::RENDERER_TASK_ON_EXIT_PRIORITY);
}

GLMainTask::~GLMainTask()
{
	deinit();
}

void GLMainTask::runFunc(FuncDelegate del, bool awaitReply, bool manageSemaphore)
{
	assert(context);
	Command cmd = manageSemaphore ? Command::RUN_FUNC_SEMAPHORE : Command::RUN_FUNC;
	commandPort.send({cmd, del}, awaitReply);
}

Base::GLContext GLMainTask::glContext() const
{
	return context;
}

GLMainTask::operator bool() const
{
	return (bool)context;
}

void GLMainTask::deinit()
{
	if(!context)
		return;
	commandPort.send({Command::EXIT});
	Base::removeOnExit(onExit);
	thread.join(); // GL implementation may assign thread destructor so must join() to make sure it completes
}

void GLMainTask::TaskContext::notifySemaphore()
{
	assumeExpr(semPtr);
	assumeExpr(semaphoreWasNotifiedPtr);
	semPtr->notify();
	markSemaphoreNotified();
}

void GLMainTask::TaskContext::markSemaphoreNotified()
{
	*semaphoreWasNotifiedPtr = true;
}

}
