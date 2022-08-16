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
#include <imagine/util/rectangle2.h>

namespace IG
{
class Viewport;
}

namespace IG::Gfx
{

class Mat4;

class ProjectionPlane
{
public:
	constexpr ProjectionPlane() = default;
	ProjectionPlane(Viewport viewport, Mat4 mat);
	float wHalf() const { return rect.x2; }
	float hHalf() const { return rect.y2; }
	GCRect bounds() const { return rect; }
	float width() const { return w; }
	float height() const { return h; }
	FP size() const { return {w, h}; }
	float focalZ() const { return focal; }
	WindowRect windowBounds() const { return winBounds; }
	float unprojectXSize(float x) const;
	float unprojectYSize(float y) const;
	float unprojectX(float x) const;
	float unprojectY(float y) const;
	float projectXSize(float x) const;
	float projectYSize(float y) const;
	float projectX(float x) const;
	float projectY(float y) const;
	float unprojectXSize(WindowRect r) const { return unprojectXSize(r.xSize()); }
	float unprojectYSize(WindowRect r) const { return unprojectYSize(r.ySize()); }
	FP unprojectSize(WindowRect r) const { return {unprojectXSize(r), unprojectYSize(r)}; }
	GCRect unProjectRect(int x, int y, int x2, int y2) const;
	GCRect unProjectRect(WindowRect src) const;
	IG::WindowRect projectRect(GCRect src) const;
	float alignXToPixel(float x) const;
	float alignYToPixel(float y) const;
	FP alignToPixel(FP p) const;
	Mat4 makeTranslate(FP p) const;
	Mat4 makeTranslate() const;

private:
	WindowRect winBounds{};
	GCRect rect{};
	float w{}, h{};
	float focal{};
	float xToPixScale{}, yToPixScale{}; // screen -> projection space at focal z
	float pixToXScale{}, pixToYScale{}; // projection -> screen space at focal z
};

}
