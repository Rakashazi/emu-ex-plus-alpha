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

#include <assert.h>

namespace IG::Audio
{

struct SampleFormat
{
	uint32_t bits = 0;
	bool isSigned = false;

	constexpr SampleFormat() {}
	constexpr SampleFormat(uint32_t bits, bool isSigned): bits(bits), isSigned(isSigned) {}
	uint32_t toBits() const { return bits; }
	uint32_t toBytes() const { return bits / 8; }

	bool operator ==(SampleFormat const& rhs) const
	{
		return bits == rhs.bits;
	}

	explicit operator bool() const
	{
		return bits;
	}
};

namespace SampleFormats
{
	static constexpr SampleFormat s8 {8, true};
	static constexpr SampleFormat u8 {8, false};
	static constexpr SampleFormat s16 {16, true};
	static constexpr SampleFormat none {};

	static const SampleFormat &getFromBits(uint32_t bits)
	{
		assert(bits == 8 || bits == 16);
		switch(bits)
		{
			case 8: return s8;
			default: return s16;
		}
	}
}

}
