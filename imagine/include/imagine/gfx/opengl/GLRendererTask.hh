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
#include "GLMainTask.hh"
#include <imagine/gfx/RendererTaskDrawContext.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/MessagePort.hh>

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
class RendererCommands;

class GLRendererTask : public GLMainTask
{
public:
	using Command = GLMainTask::Command;
	using CommandMessage = GLMainTask::CommandMessage;

	using GLMainTask::GLMainTask;
	GLRendererTask(const char *debugLabel, Renderer &r, Base::GLContext context);
	GLRendererTask(GLRendererTask &&o) = default;
	GLRendererTask &operator=(GLRendererTask &&o) = default;
	void initVBOs();
	GLuint getVBO();
	void initVAO();
	void initDefaultFramebuffer();
	GLuint defaultFBO() const { return defaultFB; }
	GLuint bindFramebuffer(Texture &tex);
	bool handleDrawableReset();
	void initialCommands(RendererCommands &cmds);
	void setRenderer(Renderer *r);
	void verifyCurrentContext(Base::GLDisplay glDpy) const;
	template<class Func>
	void run(Func &&del, bool awaitReply = false) { GLMainTask::run(std::forward<Func>(del), awaitReply); }
	template<class Func>
	void draw(DrawableHolder &drawableHolder, Base::Window &win, Base::WindowDrawParams winParams, DrawParams params, Func &&del)
	{
		doPreDraw(drawableHolder, win, winParams, params);
		bool notifySemaphoreAfterPresent = params.asyncMode() == DrawAsyncMode::NONE;
		runUnmangedSem([this, &drawableHolder, &win, del, notifySemaphoreAfterPresent](TaskContext ctx)
			{
				del(drawableHolder, win, RendererTaskDrawContext{*this, ctx, notifySemaphoreAfterPresent});
			}, params.asyncMode() != DrawAsyncMode::FULL);
	}
	// for iOS EAGLView renderbuffer management
	void setIOSDrawableDelegates();
	IG::Point2D<int> makeIOSDrawableRenderbuffer(void *layer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer);
	void deleteIOSDrawableRenderbuffer(GLuint colorRenderbuffer, GLuint depthRenderbuffer);

protected:
	Renderer *r{};
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

	void doPreDraw(DrawableHolder &drawableHolder, Base::Window &win, Base::WindowDrawParams winParams, DrawParams &params);
};

using RendererTaskImpl = GLRendererTask;

}
