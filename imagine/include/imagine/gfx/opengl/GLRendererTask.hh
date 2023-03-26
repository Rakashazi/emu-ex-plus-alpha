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
#include "GLTask.hh"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <array>

namespace IG
{
class Window;
}

namespace IG::Gfx
{

class RendererTask;
class RendererCommands;
class DrawContextSupport;

class GLRendererTask : public GLTask
{
public:
	using Command = GLTask::Command;
	using CommandMessage = GLTask::CommandMessage;

	GLRendererTask(ApplicationContext, Renderer &);
	GLRendererTask(ApplicationContext, const char *debugLabel, Renderer &);
	void initVBOs();
	GLuint getVBO();
	void initVAO();
	void initDefaultFramebuffer();
	GLuint defaultFBO() const { return defaultFB; }
	GLuint bindFramebuffer(Texture &tex);
	void runInitialCommandsInGL(TaskContext ctx, DrawContextSupport &support);
	void setRenderer(Renderer *r);
	void verifyCurrentContext() const;
	void destroyDrawable(GLDrawable &drawable);
	RendererCommands makeRendererCommands(GLTask::TaskContext taskCtx, bool manageSemaphore,
		bool notifyWindowAfterPresent, Window &win);

	void run(std::invocable auto &&f, bool awaitReply = false) { GLTask::run(IG_forward(f), awaitReply); }

	bool draw(Window &win, WindowDrawParams winParams, DrawParams params,
		std::invocable<Window &, RendererCommands &> auto &&f)
	{
		doPreDraw(win, winParams, params);
		assert(params.asyncMode != DrawAsyncMode::AUTO); // doPreDraw() should set mode
		bool manageSemaphore = params.asyncMode == DrawAsyncMode::PRESENT;
		bool notifyWindowAfterPresent = params.asyncMode != DrawAsyncMode::NONE;
		bool awaitReply = params.asyncMode != DrawAsyncMode::FULL;
		GLTask::run([=, this, &win](TaskContext ctx)
			{
				auto cmds = makeRendererCommands(ctx, manageSemaphore, notifyWindowAfterPresent, win);
				f(win, cmds);
			}, awaitReply);
		return params.asyncMode != DrawAsyncMode::NONE;
	}

	// for iOS EAGLView renderbuffer management
	void setIOSDrawableDelegates();
	IP makeIOSDrawableRenderbuffer(void *layer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer);
	void deleteIOSDrawableRenderbuffer(GLuint colorRenderbuffer, GLuint depthRenderbuffer);

protected:
	Renderer *r{};
	#ifndef CONFIG_GFX_OPENGL_ES
	GLuint streamVAO = 0;
	std::array<GLuint, 6> streamVBO{};
	uint32_t streamVBOIdx = 0;
	#endif
	IG_UseMemberIf(Config::Gfx::GLDRAWABLE_NEEDS_FRAMEBUFFER, GLuint, defaultFB){};
	GLuint fbo = 0;
	IG_UseMemberIf(Config::Gfx::OPENGL_DEBUG_CONTEXT, bool, debugEnabled){};

	void doPreDraw(Window &win, WindowDrawParams winParams, DrawParams &params) const;
};

using RendererTaskImpl = GLRendererTask;

}
