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

#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/opengl/GLRendererTask.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"
#include <cassert>

namespace IG::Gfx
{

constexpr SystemLogger log{"GLTask"};

GLTask::GLTask(ApplicationContext ctx):
	GLTask{ctx, nullptr} {}

GLTask::GLTask(ApplicationContext ctx, const char *debugLabel):
	onExit{ctx},
	commandPort{debugLabel} {}

bool GLTask::makeGLContext(GLTaskConfig config)
{
	deinit();
	thread = makeThreadSync([this, &config](auto &sem)
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
		log.info("starting GL context:{} thread message loop", (NativeGLContext)context);
		sem.release();
		auto msgs = commandPort.messages();
		auto glDpy = context.display();
		for(auto msg : msgs)
		{
			if(msg.func) [[likely]]
				msg.func(glDpy, msg.semPtr, msgs);
			else
				break;
		}
		glDpy.resetCurrentContext();
		log.info("exiting GL context:{} thread", (NativeGLContext)context);
		context = {};
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
					runSync(
						[&glContext = context](TaskContext)
						{
							// unset the drawable and finish all commands before entering background
							if(GLManager::hasCurrentDrawable())
								glContext.setCurrentDrawable({});
							glFinish();
						});
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

void GLTask::runFunc(FuncDelegate del, std::span<const uint8_t> extBuff, MessageReplyMode mode)
{
	assert(context);
	if(extBuff.size())
	{
		assert(mode == MessageReplyMode::none);
		commandPort.sendWithExtraData({.func = del}, extBuff);
	}
	else
	{
		commandPort.send({.func = del}, mode);
	}
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
	assumeExpr(semaphorePtr);
	assumeExpr(semaphoreNeedsNotifyPtr);
	semaphorePtr->release();
	markSemaphoreNotified();
}

void GLTask::TaskContext::markSemaphoreNotified()
{
	*semaphoreNeedsNotifyPtr = false;
}

static GLContextAttributes makeGLContextAttributes(GL::Version version)
{
	GLContextAttributes glAttr{version, glAPI};
	if(Config::DEBUG_BUILD)
		glAttr.debug = true;
	else
		glAttr.noError = true;
	return glAttr;
}

static GLContext makeVersionedGLContext(GLManager &mgr, GLBufferConfig config, GL::Version version)
{
	auto glAttr = makeGLContextAttributes(version);
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
		if(bufferConf.maySupportGLES(mgr.display(), 3))
		{
			auto ctx = makeVersionedGLContext(mgr, bufferConf, {3});
			if(ctx)
			{
				return ctx;
			}
		}
		// fall back to OpenGL ES 2.0
		return makeVersionedGLContext(mgr, bufferConf, {2});
	}
	else
	{
		auto ctx = makeVersionedGLContext(mgr, bufferConf, {3, 3});
		if(ctx)
		{
			return ctx;
		}
	}
	return {};
}

}
