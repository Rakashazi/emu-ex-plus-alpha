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

namespace Audio
{

struct SampleFormat
{
	uint bits;
	bool isSigned;

	uint toBits() const { return bits; }
	uint toBytes() const { return bits / 8; }

	bool operator ==(SampleFormat const& rhs) const
	{
		return bits == rhs.bits;
	}
};

namespace SampleFormats
{
	static const SampleFormat s8 = { 8, true };
	static const SampleFormat u8 = { 8, false };
	static const SampleFormat s16 = { 16, true };

	static const SampleFormat *getFromBits(uint bits)
	{
		assert(bits == 8 || bits == 16);
		switch(bits)
		{
			case 8: return &s8;
			default: return &s16;
		}
	}
}

}
