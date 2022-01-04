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

#include <imagine/util/math/GLMVec3.hh>
#include <imagine/util/math/GLMVec4.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/glm/ext/matrix_float4x4.hpp>
#include <compare>

class GLMMat4
{
public:
	constexpr GLMMat4() = default;
	constexpr GLMMat4(glm::mat4 m): m{m} {}
	static GLMMat4 makeTranslate(GLMVec3 translation);
	static GLMMat4 makePerspectiveFovRH(float fovy, float aspect, float znear, float zfar);
	GLMMat4 translate(GLMVec3 translation) const;
	GLMMat4 scale(GLMVec3 factors) const;
	GLMMat4 rotate(float angle, GLMVec3 axis) const;
	GLMMat4 invert() const;
	GLMMat4 mult(GLMMat4 mat) const;
	GLMVec4 mult(GLMVec4 vec) const;
	GLMVec3 project(IG::Rect2<int> viewport, GLMVec3 obj) const;
	GLMVec3 unproject(IG::Rect2<int> viewport, GLMVec3 win, GLMMat4 inverse) const;
	bool operator ==(GLMMat4 const &rhs) const;

	glm::vec4 &operator[](int i)
	{
		return m[i];
	}

	constexpr glm::vec4 const &operator[](int i) const
	{
		return m[i];
	}

	// Convenience functions
	GLMMat4 scale(float s) const { return scale({s, s, 1.}); }
	GLMMat4 scale(IG::Point2D<float> p) const { return scale({p.x, p.y, 1.}); }
	GLMMat4 pitchRotate(float t) const { return rotate(t, {1., 0., 0.}); }
	GLMMat4 rollRotate(float t) const { return rotate(t, {0., 0., 1.}); }
	GLMMat4 yawRotate(float t) const { return rotate(t, {0., 1., 0.}); }

protected:
	glm::mat4 m
	{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1},
	};
};
