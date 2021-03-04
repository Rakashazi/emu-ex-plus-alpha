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
#include <imagine/base/Base.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/opengl/GLRendererTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"

namespace Gfx
{

GLTask::GLTask() {}

GLTask::GLTask(const char *debugLabel):
	commandPort{debugLabel}
{}

Error GLTask::makeGLContext(GLTaskConfig config)
{
	deinit();
	thread = IG::makeThreadSync(
		[this, &config](auto &sem)
		{
			auto glDpy = Base::GLDisplay::getDefault(glAPI);
			context = makeGLContext(glDpy, config.bufferConfig);
			if(unlikely(!context))
			{
				sem.notify();
				return;
			}
			context.setCurrent(glDpy, context, config.initialDrawable);
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
			logMsg("starting GL context:%p thread event loop", context.nativeObject());
			if(config.threadPriority)
				Base::setThisThreadPriority(config.threadPriority);
			sem.notify();
			eventLoop.run(context);
			commandPort.detach();
		});
	if(unlikely(!context))
	{
		return std::runtime_error("error creating GL context");
	}
	onExit =
		{
			[this](bool backgrounded)
			{
				if(backgrounded)
				{
					run(
						[glContext = context](TaskContext ctx)
						{
							// unset the drawable and finish all commands before entering background
							if(glContext.hasCurrentDrawable(ctx.glDisplay()))
								glContext.setDrawable(ctx.glDisplay(), {}, glContext);
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
	onExit = {};
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

static Base::GLContextAttributes makeGLContextAttributes(uint32_t majorVersion, uint32_t minorVersion)
{
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	glAttr.setMajorVersion(majorVersion);
	#ifdef CONFIG_GFX_OPENGL_ES
	glAttr.setOpenGLESAPI(true);
	#else
	glAttr.setMinorVersion(minorVersion);
	#endif
	return glAttr;
}

static Base::GLContext makeVersionedGLContext(Base::GLDisplay dpy, Base::GLBufferConfig config,
	unsigned majorVersion, unsigned minorVersion)
{
	auto glAttr = makeGLContextAttributes(majorVersion, minorVersion);
	IG::ErrorCode ec{};
	Base::GLContext glCtx{dpy, glAttr, config, ec};
	return glCtx;
}

Base::GLContext GLTask::makeGLContext(Base::GLDisplay dpy, Base::GLBufferConfig bufferConf)
{
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		if constexpr(Config::Gfx::OPENGL_ES == 1)
		{
			return makeVersionedGLContext(dpy, bufferConf, 1, 0);
		}
		else
		{
			if(bufferConf.maySupportGLES(dpy, 3))
			{
				auto ctx = makeVersionedGLContext(dpy, bufferConf, 3, 0);
				if(ctx)
				{
					return ctx;
				}
			}
			// fall back to OpenGL ES 2.0
			return makeVersionedGLContext(dpy, bufferConf, 2, 0);
		}
	}
	else
	{
		if(Config::Gfx::OPENGL_SHADER_PIPELINE)
		{
			auto ctx = makeVersionedGLContext(dpy, bufferConf, 3, 3);
			if(ctx)
			{
				return ctx;
			}
		}
		if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE)
		{
			// fall back to OpenGL 1.3
			return makeVersionedGLContext(dpy, bufferConf, 1, 3);
		}
	}
	return {};
}

}
