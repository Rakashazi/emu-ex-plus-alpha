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

#include <GLKit/GLKMathTypes.h>
#include <GLKit/GLKVector3.h>

class GLKitVec3
{
public:
	constexpr GLKitVec3() {}
	constexpr GLKitVec3(float x, float y, float z): v{{x, y, z}} {}
	constexpr GLKitVec3(const GLKVector3 &v): v{{v.x, v.y, v.z}} {}
	constexpr GLKVector3 vec() const { return v; }

	float &operator[](int i)
	{
		return v.v[i];
	}

	constexpr float const &operator[](int i) const
	{
		return v.v[i];
	}

	constexpr float x() const { return v.x; }
	constexpr float y() const { return v.y; }
	constexpr float z() const { return v.z; }

protected:
	GLKVector3 v{};
};
