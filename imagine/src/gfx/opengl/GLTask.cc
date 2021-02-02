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
#include "internalDefs.hh"

namespace Gfx
{

GLTask::GLTask() {}

GLTask::GLTask(const char *debugLabel, Base::GLContext context_, int threadPriority):
	context{context_},
	onExit
	{
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
		}, Base::RENDERER_TASK_ON_EXIT_PRIORITY
	},
	commandPort{debugLabel}
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
								msg.args.run.func(glDpy, msg.semPtr);
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
}

GLTask::~GLTask()
{
	deinit();
}

void GLTask::runFunc(FuncDelegate del, bool awaitReply)
{
	assert(context);
	commandPort.send({Command::RUN_FUNC, del}, awaitReply);
}

Base::GLContext GLTask::glContext() const
{
	return context;
}

GLTask::operator bool() const
{
	return (bool)context;
}

void GLTask::deinit()
{
	if(!context)
		return;
	commandPort.send({Command::EXIT});
	thread.join(); // GL implementation may assign thread destructor so must join() to make sure it completes
}

void GLTask::TaskContext::notifySemaphore()
{
	assumeExpr(semPtr);
	assumeExpr(semaphoreNeedsNotifyPtr);
	semPtr->notify();
	markSemaphoreNotified();
}

void GLTask::TaskContext::markSemaphoreNotified()
{
	*semaphoreNeedsNotifyPtr = false;
}

}
