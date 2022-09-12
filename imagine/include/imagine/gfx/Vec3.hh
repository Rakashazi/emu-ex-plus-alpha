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

#include <imagine/glm/ext/vector_float3.hpp>

namespace IG::Gfx
{

using Vec3Impl = glm::vec3;

class Vec3 : public Vec3Impl
{
public:
	using Vec3Impl::Vec3Impl;
	constexpr Vec3(Vec3Impl v): Vec3Impl{v} {}
};

}
