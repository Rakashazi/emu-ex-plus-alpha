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
#include <imagine/gfx/Viewport.hh>
#include <imagine/util/rectangle2.h>

namespace Gfx
{

class RendererCommands;
class Mat4;

class ProjectionPlane
{
public:
	constexpr ProjectionPlane() {}
	static ProjectionPlane makeWithMatrix(Viewport viewport, Mat4 mat);
	GC wHalf() const { return rect.x2; }
	GC hHalf() const { return rect.y2; }
	GCRect bounds() const { return rect; }
	void updateMMSize(Viewport v);
	GC width() const;
	GC height() const;
	GP size() const;
	GC focalZ() const;
	Viewport viewport() const;
	GC unprojectXSize(int x) const;
	GC unprojectYSize(int y) const;
	GC unprojectX(int x) const;
	GC unprojectY(int y) const;
	int projectXSize(GC x) const;
	int projectYSize(GC y) const;
	int projectX(GC x) const;
	int projectY(GC y) const;
	GC unprojectXSize(IG::WindowRect r) const { return unprojectXSize(r.xSize()); }
	GC unprojectYSize(IG::WindowRect r) const { return unprojectYSize(r.ySize()); }
	GP unprojectSize(IG::WindowRect r) const { return {unprojectXSize(r), unprojectYSize(r)}; }
	GCRect unProjectRect(int x, int y, int x2, int y2) const;
	GCRect unProjectRect(IG::WindowRect src) const;
	IG::WindowRect projectRect(GCRect src) const;
	GC alignXToPixel(GC x) const;
	GC alignYToPixel(GC y) const;
	IG::Point2D<GC> alignToPixel(IG::Point2D<GC> p) const;
	GC xMMSize(GC mm) const;
	GC yMMSize(GC mm) const;
	GC xSMMSize(GC mm) const;
	GC ySMMSize(GC mm) const;
	Mat4 makeTranslate(IG::Point2D<float> p) const;
	Mat4 makeTranslate() const;
	void loadTranslate(Gfx::RendererCommands &cmds, GC x, GC y) const;
	void loadTranslate(Gfx::RendererCommands &cmds, GP p) const;
	void resetTransforms(Gfx::RendererCommands &cmds) const;

private:
	Viewport viewport_;
	GCRect rect;
	GC w = 0, h = 0;
		GC  focal = 0,
		xToPixScale = 0, yToPixScale = 0, // screen -> projection space at focal z
		pixToXScale = 0, pixToYScale = 0, // projection -> screen space at focal z
		mmToXScale = 0, mmToYScale = 0;   // MM of screen -> projection space at focal z
	#ifdef __ANDROID__
	GC smmToXScale = 0, smmToYScale = 0;
	#endif
};

}
