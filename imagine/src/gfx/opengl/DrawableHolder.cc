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

#define LOGTAG "GLDrawableHolder"
#include <imagine/gfx/DrawableHolder.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT 0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT 0x0403
#endif

namespace Gfx
{

DrawableHolder::operator Drawable() const
{
	return drawable_;
}

DrawableHolder::operator bool() const
{
	return (bool)drawable_;
}

bool DrawableHolder::addOnFrame(Base::OnFrameDelegate del, int priority)
{
	if(!onFrame.size())
	{
		// reset time-stamp when first delegate is added
		lastTimestamp = {};
	}
	return onFrame.add(del, priority);
}

bool DrawableHolder::removeOnFrame(Base::OnFrameDelegate del)
{
	return onFrame.remove(del);
}

void DrawableHolder::dispatchOnFrame()
{
	auto now = IG::steadyClockTimestamp();
	FrameParams frameParams{now, lastTimestamp, screen->frameTime()};
	onFrame.runAll([&](Base::OnFrameDelegate del){ return del(frameParams); });
	lastTimestamp = now;
}

void GLDrawableHolder::makeDrawable(Renderer &r, RendererTask &task, Base::Window &win)
{
	destroyDrawable(r);
	screen = win.screen();
	auto [ec, drawable] = r.glDpy.makeDrawable(win, r.gfxBufferConfig);
	if(ec)
	{
		logErr("Error creating GL drawable");
		return;
	}
	drawable_ = drawable;
	onResume =
		[drawable = drawable](bool focused) mutable
		{
			drawable.restoreCaches();
			return true;
		};
	Base::addOnResume(onResume, Base::RENDERER_DRAWABLE_ON_RESUME_PRIORITY);
	onExit =
		[this, glDpy = r.glDpy](bool backgrounded) mutable
		{
			if(backgrounded)
			{
				drawFinishedEvent.cancel();
				drawable_.freeCaches();
			}
			else
				drawable_.destroy(glDpy);
			return true;
		};
	Base::addOnExit(onExit, Base::RENDERER_DRAWABLE_ON_EXIT_PRIORITY);
	drawFinishedEvent.attach(
		[this]()
		{
			if(!onFrame.size())
				return;
			static_cast<DrawableHolder*>(this)->dispatchOnFrame();
		});
	if(r.support.hasDrawReadBuffers())
	{
		task.run([glCtx = task.glContext(), drawable = drawable,
			&support = std::as_const(r.support)](GLMainTask::TaskContext ctx)
		{
			Base::GLContext::setDrawable(ctx.glDisplay(), drawable, glCtx);
			//logMsg("specifying draw/read buffers");
			const GLenum back = Config::Gfx::OPENGL_ES_MAJOR_VERSION ? GL_BACK : GL_BACK_LEFT;
			support.glDrawBuffers(1, &back);
			support.glReadBuffer(GL_BACK);
		});
	}
}

void GLDrawableHolder::destroyDrawable(Renderer &r)
{
	if(!drawable_)
		return;
	drawable_.destroy(r.glDpy);
	Base::removeOnExit(onResume);
	Base::removeOnExit(onExit);
	drawFinishedEvent.detach();
	lastTimestamp = {};
}

void GLDrawableHolder::notifyOnFrame()
{
	if(onFrame.size())
	{
		drawFinishedEvent.notify();
	}
}

}
