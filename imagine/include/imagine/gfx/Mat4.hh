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

#include <imagine/gfx/Vec3.hh>
#include <imagine/gfx/Vec4.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/glm/ext/matrix_float4x4.hpp>

namespace IG
{
class Viewport;
}

namespace IG::Gfx
{

using Mat4Impl = glm::mat4;

class Mat4 : public Mat4Impl
{
public:
	using Mat4Impl::Mat4Impl;
	constexpr Mat4(Mat4Impl m): Mat4Impl{m} {}

	static Mat4 ident();
	static Mat4 makeTranslate(Vec3 translation);
	static Mat4 makeTranslate(WPt p) { return makeTranslate(Vec3{float(p.x), float(p.y), 0.f}); }
	static Mat4 makeTranslateScale(FRect r) { return makeTranslate(Vec3{r.x, r.y, 0.f}).scale(r.size()); }
	static Mat4 makeTranslateScale(WRect r) { return makeTranslateScale(r.as<float>()); }
	static Mat4 makePerspectiveFovRH(float fovy, float aspect, float znear, float zfar);
	Mat4 projectionPlane(Viewport viewport, float z, float rollAngle);
	Mat4 translate(Vec3 translation) const;
	Mat4 scale(Vec3 factors) const;
	Mat4 rotate(float angle, Vec3 axis) const;
	Mat4 invert() const;
	Vec3 project(Rect2<int> viewport, Vec3 obj) const;
	Vec3 unproject(Rect2<int> viewport, Vec3 win, Mat4 inverse) const;

	// Convenience functions
	Mat4 scale(float s) const { return scale({s, s, 1.}); }
	Mat4 scale(F2Size p) const { return scale({p.x, p.y, 1.}); }
	Mat4 scale(I2Size p) const { return scale({float(p.x), float(p.y), 1.}); }
	Mat4 pitchRotate(float t) const { return rotate(t, {1., 0., 0.}); }
	Mat4 rollRotate(float t) const { return rotate(t, {0., 0., 1.}); }
	Mat4 yawRotate(float t) const { return rotate(t, {0., 1., 0.}); }
};

}
