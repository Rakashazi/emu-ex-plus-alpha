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

DrawableHolder::DrawableHolder(DrawableHolder &&o)
{
	*this = std::move(o);
}

DrawableHolder &DrawableHolder::operator=(DrawableHolder &&o)
{
	destroyDrawable();
	GLDrawableHolder::operator=(std::move(o));
	o.drawable_ = {};
	return *this;
}

GLDrawableHolder::~GLDrawableHolder()
{
	destroyDrawable();
}

DrawableHolder::operator Drawable() const
{
	return drawable_;
}

DrawableHolder::operator bool() const
{
	return (bool)drawable_;
}

void GLDrawableHolder::makeDrawable(Base::GLDisplay dpy, Base::Window &win, Base::GLBufferConfig bufferConfig)
{
	destroyDrawable();
	auto [ec, drawable] = dpy.makeDrawable(win, bufferConfig);
	if(ec)
	{
		logErr("Error creating GL drawable");
		return;
	}
	drawable_ = drawable;
	if constexpr(Config::envIsIOS)
	{
		onExit =
		{
			[drawable = drawable, dpy](bool backgrounded)
			{
				if(backgrounded)
				{
					IG::copySelf(drawable).freeCaches();
					Base::addOnResume(
						[drawable](bool focused)
						{
							IG::copySelf(drawable).restoreCaches();
							return false;
						}, Base::RENDERER_DRAWABLE_ON_RESUME_PRIORITY
					);
				}
				return true;
			}, Base::RENDERER_DRAWABLE_ON_EXIT_PRIORITY
		};
	}
}

void GLDrawableHolder::destroyDrawable()
{
	if(!drawable_)
		return;
	drawable_.destroy(Base::GLDisplay::getDefault());
	drawable_ = {};
	onExit = {};
}

}
