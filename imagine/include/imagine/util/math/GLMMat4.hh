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
#include <imagine/util/operators.hh>
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <imagine/glm/mat4x4.hpp>

class GLMMat4 : NotEquals<GLMMat4>
{
public:
	glm::mat4 m;

	constexpr GLMMat4() {}
	GLMMat4(const glm::mat4 &m): m{m} {}

	GLMMat4 translate(const GLMVec3 &translation) const;

	static GLMMat4 makeTranslate(const GLMVec3 &translation);

	static GLMMat4 makePerspectiveFovRH(float fovy, float aspect, float znear, float zfar);

	GLMMat4 scale(const GLMVec3 &factors) const;

	GLMMat4 scale(float s) const { return scale({s, s, 1.}); }
	GLMMat4 scale(IG::Point2D<float> p) const { return scale({p.x, p.y, 1.}); }

	GLMMat4 rotate(float angle, const GLMVec3 &axis) const;

	GLMMat4 pitchRotate(float t) const
	{
		return rotate(t, {1., 0., 0.});
	}

	GLMMat4 rollRotate(float t) const
	{
		return rotate(t, {0., 0., 1.});
	}

	GLMMat4 yawRotate(float t) const
	{
		return rotate(t, {0., 1., 0.});
	}

	GLMMat4 invert() const;

	GLMVec4 mult(const GLMVec4 vec) const
	{
		return m * vec.v;
	}

	GLMVec3 project(IG::Rect2<int> viewport, GLMVec3 obj) const;

	GLMVec3 unproject(IG::Rect2<int> viewport, GLMVec3 win, const GLMMat4 &inverse) const;

	bool operator ==(GLMMat4 const &rhs) const
	{
		return m == rhs.m;
	}

	glm::vec4 &operator[](int i)
	{
		return m[i];
	}

	glm::vec4 const &operator[](int i) const
	{
		return m[i];
	}
};
