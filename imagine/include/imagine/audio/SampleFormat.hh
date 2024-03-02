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

#include <cstdint>

namespace IG::Audio
{

class SampleFormat
{
public:
	constexpr SampleFormat() = default;
	constexpr SampleFormat(uint8_t bytes, bool isFloat = false):
		bytes_{bytes}, isFloat_{isFloat}{}

	constexpr int bytes() const
	{
		return bytes_;
	}

	constexpr int bits() const
	{
		return bytes() * 8;
	}

	constexpr bool isFloat() const
	{
		return isFloat_;
	}

	constexpr bool operator ==(SampleFormat const& rhs) const = default;

	constexpr explicit operator bool() const
	{
		return bytes();
	}

protected:
	uint8_t bytes_:7{};
	uint8_t isFloat_:1{};
};

namespace SampleFormats
{
	constexpr SampleFormat   i8{1};
	constexpr SampleFormat  i16{2};
	constexpr SampleFormat  i32{4};
	constexpr SampleFormat  f32{4, true};
	constexpr SampleFormat none;
}

}
