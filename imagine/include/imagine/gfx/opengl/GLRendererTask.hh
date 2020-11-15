#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/MessagePort.hh>
#include <imagine/gfx/SyncFence.hh>
#include <thread>
#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
#include <mutex>
#include <condition_variable>
#endif

namespace IG
{
class Semaphore;
}

namespace Base
{
class Window;
}

namespace Gfx
{

class DrawableHolder;
class RendererTask;

class GLRendererTask
{
public:
	enum class Command: uint8_t
	{
		UNSET, DRAW, RUN_FUNC, EXIT
	};

	enum class Reply: uint8_t
	{
		UNSET
	};

	struct CommandMessage
	{
		IG::Semaphore *semPtr{};
		union Args
		{
			struct DrawArgs
			{
				DrawDelegate del;
				Base::Window *winPtr;
				DrawableHolder *drawableHolderPtr;
				GLsync fence;
			} draw;
			struct RunFuncArgs
			{
				RenderTaskFuncDelegate func;
			} runFunc;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, DrawDelegate drawDel, DrawableHolder &drawableHolder, Base::Window &win, GLsync fence, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, args{drawDel, &win, &drawableHolder, fence}, command{command} {}
		constexpr CommandMessage(Command command, RenderTaskFuncDelegate func, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command}
		{
			args.runFunc.func = func;
		}
		explicit operator bool() const { return command != Command::UNSET; }
		void setReplySemaphore(IG::Semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	struct ReplyMessage
	{
		Reply reply{Reply::UNSET};

		constexpr ReplyMessage() {}
		constexpr ReplyMessage(Reply reply):
			reply{reply} {}
		explicit operator bool() const { return reply != Reply::UNSET; }
	};

	Base::GLContext glContext() const { return glCtx; };
	void initVBOs();
	GLuint getVBO();
	void initVAO();
	void initDefaultFramebuffer();
	GLuint defaultFBO() const { return defaultFB; }
	GLuint bindFramebuffer(Texture &tex);
	void destroyContext(Base::GLDisplay dpy);
	bool hasSeparateContextThread() const;
	bool handleDrawableReset();
	void initialCommands(RendererCommands &cmds);

protected:
	Base::MessagePort<CommandMessage> commandPort{"RenderTask Command"};
	#ifdef CONFIG_GFX_RENDERER_TASK_REPLY_PORT
	Base::MessagePort<ReplyMessage> replyPort{"RenderTask Reply"}; // currently unused
	#endif
	Base::GLContext glCtx{};
	Base::ExitDelegate onExit{};
	std::thread thread{};
	#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
	std::mutex drawMutex{};
	std::condition_variable drawCondition{};
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	GLuint streamVAO = 0;
	std::array<GLuint, 6> streamVBO{};
	uint32_t streamVBOIdx = 0;
	#endif
	#ifdef CONFIG_GLDRAWABLE_NEEDS_FRAMEBUFFER
	GLuint defaultFB = 0;
	#else
	static constexpr GLuint defaultFB = 0;
	#endif
	GLuint fbo = 0;
	bool resetDrawable = false;
	bool contextInitialStateSet = false;
	bool threadRunning = false;
	#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
	bool canDraw = true;
	#endif

	void replyHandler(Renderer &r, ReplyMessage msg);
	bool commandHandler(decltype(commandPort)::Messages messages, Base::GLDisplay glDpy, bool ownsThread);
};

using RendererTaskImpl = GLRendererTask;

class GLRendererDrawTask
{
public:
	GLRendererDrawTask(RendererTask &task, Base::GLDisplay glDpy, IG::Semaphore *semAddr);
	void setCurrentDrawable(Drawable win);
	void present(Drawable win);
	GLuint bindFramebuffer(Texture &t);
	GLuint getVBO();
	GLuint defaultFramebuffer() const;
	void notifySemaphore();
	Base::GLDisplay glDisplay() const { return glDpy; };

protected:
	RendererTask &task;
	Base::GLDisplay glDpy{};
	IG::Semaphore *semAddr{};
};

using RendererDrawTaskImpl = GLRendererDrawTask;

}
