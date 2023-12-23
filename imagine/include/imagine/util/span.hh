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

#include <span>
#include <cstdint>

namespace IG
{

inline std::span<const uint8_t> asBytes(auto &v)
{
	return {reinterpret_cast<const uint8_t*>(&v), sizeof(v)};
}

inline std::span<uint8_t> asWritableBytes(auto &v)
{
	return {reinterpret_cast<uint8_t*>(&v), sizeof(v)};
}

}
