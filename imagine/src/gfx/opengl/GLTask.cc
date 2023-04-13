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

#define LOGTAG "GLTask"
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/opengl/GLRendererTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/base/Error.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"
#include <cassert>

namespace IG::Gfx
{

GLTask::GLTask(ApplicationContext ctx):
	GLTask{ctx, nullptr}
{}

GLTask::GLTask(ApplicationContext ctx, const char *debugLabel):
	onExit{ctx},
	commandPort{debugLabel}
{}

bool GLTask::makeGLContext(GLTaskConfig config)
{
	deinit();
	thread = IG::makeThreadSync(
		[this, &config](auto &sem)
		{
			threadId_ = thisThreadId();
			auto &glManager = *config.glManagerPtr;
			glManager.bindAPI(glAPI);
			context = makeGLContext(glManager, config.bufferConfig);
			if(!context) [[unlikely]]
			{
				sem.release();
				return;
			}
			context.setCurrentContext(config.initialDrawable);
			auto eventLoop = EventLoop::makeForThread();
			commandPort.attach(eventLoop,
				[this, glDpy = context.display()](auto msgs)
				{
					for(auto msg : msgs)
					{
						if(msg.func) [[likely]]
						{
							msg.func(glDpy, msg.semPtr);
						}
						else
						{
							glDpy.resetCurrentContext();
							logMsg("exiting GL context:%p thread", (NativeGLContext)context);
							context = {};
							EventLoop::forThread().stop();
							return false;
						}
					}
					return true;
				});
			logMsg("starting GL context:%p thread event loop", (NativeGLContext)context);
			sem.release();
			eventLoop.run(context);
			commandPort.detach();
		});
	if(!context) [[unlikely]]
	{
		return false;
	}
	bufferConfig = config.bufferConfig;
	onExit =
		{
			[this](ApplicationContext, bool backgrounded)
			{
				if(backgrounded)
				{
					run(
						[&glContext = context](TaskContext ctx)
						{
							// unset the drawable and finish all commands before entering background
							if(GLManager::hasCurrentDrawable())
								glContext.setCurrentDrawable({});
							glFinish();
						}, true);
				}
				return true;
			}, appContext(), RENDERER_TASK_ON_EXIT_PRIORITY
		};
	return true;
}

GLTask::~GLTask()
{
	deinit();
}

void GLTask::runFunc(FuncDelegate del, bool awaitReply)
{
	assert(context);
	commandPort.send({.func = del}, awaitReply);
}

GLBufferConfig GLTask::glBufferConfig() const
{
	return bufferConfig;
}

const GLContext &GLTask::glContext() const
{
	return context;
}

ApplicationContext GLTask::appContext() const
{
	return onExit.appContext();
}

GLTask::operator bool() const
{
	return (bool)context;
}

void GLTask::deinit()
{
	if(!context)
		return;
	commandPort.send({}); // exit
	onExit.reset();
	thread.join(); // GL implementation may assign thread destructor so must join() to make sure it completes
}

void GLTask::TaskContext::notifySemaphore()
{
	assumeExpr(semPtr);
	assumeExpr(semaphoreNeedsNotifyPtr);
	semPtr->release();
	markSemaphoreNotified();
}

void GLTask::TaskContext::markSemaphoreNotified()
{
	*semaphoreNeedsNotifyPtr = false;
}

static GLContextAttributes makeGLContextAttributes(int majorVersion, int minorVersion)
{
	GLContextAttributes glAttr{majorVersion, minorVersion, glAPI};
	if(Config::DEBUG_BUILD)
		glAttr.debug = true;
	else
		glAttr.noError = true;
	return glAttr;
}

static GLContext makeVersionedGLContext(GLManager &mgr, GLBufferConfig config,
	int majorVersion, int minorVersion)
{
	auto glAttr = makeGLContextAttributes(majorVersion, minorVersion);
	try
	{
		return mgr.makeContext(glAttr, config);
	}
	catch(...)
	{
		return {};
	}
}

GLContext GLTask::makeGLContext(GLManager &mgr, GLBufferConfig bufferConf)
{
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		if constexpr(Config::Gfx::OPENGL_ES == 1)
		{
			return makeVersionedGLContext(mgr, bufferConf, 1, 0);
		}
		else
		{
			if(bufferConf.maySupportGLES(mgr.display(), 3))
			{
				auto ctx = makeVersionedGLContext(mgr, bufferConf, 3, 0);
				if(ctx)
				{
					return ctx;
				}
			}
			// fall back to OpenGL ES 2.0
			return makeVersionedGLContext(mgr, bufferConf, 2, 0);
		}
	}
	else
	{
		if(Config::Gfx::OPENGL_SHADER_PIPELINE)
		{
			auto ctx = makeVersionedGLContext(mgr, bufferConf, 3, 3);
			if(ctx)
			{
				return ctx;
			}
		}
		if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE)
		{
			// fall back to OpenGL 1.3
			return makeVersionedGLContext(mgr, bufferConf, 1, 3);
		}
	}
	return {};
}

}
