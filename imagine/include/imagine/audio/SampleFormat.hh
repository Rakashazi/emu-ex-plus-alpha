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

#include <imagine/util/operators.hh>

namespace IG::Audio
{

class SampleFormat : public NotEquals<SampleFormat>
{
public:
	constexpr SampleFormat() {}
	constexpr SampleFormat(uint8_t bytes, bool isFloat = false):
		bytes_{bytes}, isFloatType{isFloat}
	{}

	uint8_t bytes() const
	{
		return bytes_;
	}

	uint8_t bits() const
	{
		return bytes_ * 8;
	}

	bool isFloat() const
	{
		return isFloatType;
	}

	bool operator ==(SampleFormat const& rhs) const
	{
		return bytes_ == rhs.bytes_
			&& isFloatType == rhs.isFloatType;
	}

	explicit operator bool() const
	{
		return bytes_;
	}

protected:
	uint8_t bytes_ = 0;
	bool isFloatType = 0;
};

namespace SampleFormats
{
	static constexpr SampleFormat   i8 {1};
	static constexpr SampleFormat  i16 {2};
	static constexpr SampleFormat  i32 {4};
	static constexpr SampleFormat  f32 {4, true};
	static constexpr SampleFormat none {};
}

}
