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
#include <assert.h>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/opengl/GLRendererTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"

namespace Gfx
{

GLTask::GLTask(Base::ApplicationContext ctx):
	GLTask{ctx, nullptr}
{}

GLTask::GLTask(Base::ApplicationContext ctx, const char *debugLabel):
	onExit{ctx},
	commandPort{debugLabel}
{}

Error GLTask::makeGLContext(GLTaskConfig config)
{
	deinit();
	thread = IG::makeThreadSync(
		[this, &config](auto &sem)
		{
			auto &glManager = *config.glManagerPtr;
			glManager.bindAPI(glAPI);
			context = makeGLContext(glManager, config.bufferConfig);
			if(!context) [[unlikely]]
			{
				sem.notify();
				return;
			}
			context.setCurrentContext(config.initialDrawable);
			auto eventLoop = Base::EventLoop::makeForThread();
			commandPort.attach(eventLoop,
				[this, glDpy = context.display()](auto msgs)
				{
					for(auto msg : msgs)
					{
						switch(msg.command)
						{
							case Command::RUN_FUNC:
							{
								msg.args.run.func(glDpy, msg.semPtr);
								break;
							}
							case Command::EXIT:
							{
								glDpy.resetCurrentContext();
								logMsg("exiting GL context:%p thread", (Base::NativeGLContext)context);
								context = {};
								Base::EventLoop::forThread().stop();
								return false;
							}
							default:
							{
								logWarn("unknown ThreadCommandMessage value:%d", (int)msg.command);
							}
						}
					}
					return true;
				});
			logMsg("starting GL context:%p thread event loop", (Base::NativeGLContext)context);
			if(config.threadPriority)
				Base::setThisThreadPriority(config.threadPriority);
			sem.notify();
			eventLoop.run(context);
			commandPort.detach();
		});
	if(!context) [[unlikely]]
	{
		return std::runtime_error("error creating GL context");
	}
	bufferConfig = config.bufferConfig;
	onExit =
		{
			[this](Base::ApplicationContext, bool backgrounded)
			{
				if(backgrounded)
				{
					run(
						[&glContext = context](TaskContext ctx)
						{
							// unset the drawable and finish all commands before entering background
							if(Base::GLManager::hasCurrentDrawable())
								glContext.setCurrentDrawable({});
							#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
							glReleaseShaderCompiler();
							#endif
							glFinish();
						}, true);
				}
				return true;
			}, appContext(), Base::RENDERER_TASK_ON_EXIT_PRIORITY
		};
	return {};
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

Base::GLBufferConfig GLTask::glBufferConfig() const
{
	return bufferConfig;
}

const Base::GLContext &GLTask::glContext() const
{
	return context;
}

Base::ApplicationContext GLTask::appContext() const
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
	commandPort.send({Command::EXIT});
	onExit.reset();
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

static Base::GLContextAttributes makeGLContextAttributes(unsigned majorVersion, unsigned minorVersion)
{
	Base::GLContextAttributes glAttr{majorVersion, minorVersion, glAPI};
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	else
		glAttr.setNoError(true);
	return glAttr;
}

static Base::GLContext makeVersionedGLContext(Base::GLManager &mgr, Base::GLBufferConfig config,
	unsigned majorVersion, unsigned minorVersion)
{
	auto glAttr = makeGLContextAttributes(majorVersion, minorVersion);
	IG::ErrorCode ec{};
	return mgr.makeContext(glAttr, config, ec);
}

Base::GLContext GLTask::makeGLContext(Base::GLManager &mgr, Base::GLBufferConfig bufferConf)
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
