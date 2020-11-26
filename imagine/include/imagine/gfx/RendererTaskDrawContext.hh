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

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLMainTask.hh>
#endif

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Mat4.hh>

namespace Base
{
class Window;
}

namespace Gfx
{

class RendererTask;
class RendererCommands;
class DrawableHolder;
class Viewport;

class RendererTaskDrawContext : public RendererTaskDrawContextImpl
{
public:
	using RendererTaskDrawContextImpl::RendererTaskDrawContextImpl;

	RendererCommands makeRendererCommands(DrawableHolder &drawableHolder, Base::Window &win, Viewport viewport, Mat4 projMat);
	RendererTask &rendererTask() const;
	Renderer &renderer() const;
};

}
