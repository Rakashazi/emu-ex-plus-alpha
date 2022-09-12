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

#include <imagine/glm/ext/vector_float4.hpp>

namespace IG::Gfx
{

using Vec4Impl = glm::vec4;

class Vec4 : public Vec4Impl
{
public:
	using Vec4Impl::Vec4Impl;
	constexpr Vec4(Vec4Impl v): Vec4Impl{v} {}
};

}
