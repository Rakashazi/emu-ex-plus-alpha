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
#include <bit>

namespace IG
{

uintptr_t pageSize();

inline uintptr_t roundDownToPageSize(uintptr_t addr)
{
	return addr & ~(pageSize() - 1);
}

inline auto roundDownToPageSize(uint8_t *p)
{
	auto origAddr = std::bit_cast<uintptr_t>(p);
	auto alignedAddr = roundDownToPageSize(origAddr);
	return p + (alignedAddr - origAddr);
}

inline uintptr_t roundUpToPageSize(uintptr_t addr)
{
	return (addr + pageSize() - 1 ) & ~(pageSize() - 1);
}

inline auto roundUpToPageSize(uint8_t *p)
{
	auto origAddr = std::bit_cast<uintptr_t>(p);
	auto alignedAddr = roundUpToPageSize(origAddr);
	return p + (alignedAddr - origAddr);
}

}
