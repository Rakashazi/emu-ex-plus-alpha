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

#include <imagine/util/span.hh>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <bit>

namespace IG
{

extern uintptr_t pageSize;

std::span<uint8_t> vAlloc(size_t bytes);
std::span<uint8_t> vAllocMirrored(size_t bytes);
void vFree(std::span<uint8_t>);

inline uintptr_t truncPageSize(uintptr_t addr)
{
	return addr & ~(pageSize - 1);
}

inline auto truncPageSize(uint8_t *p)
{
	auto origAddr = std::bit_cast<uintptr_t>(p);
	auto alignedAddr = truncPageSize(origAddr);
	return p + (alignedAddr - origAddr);
}

inline uintptr_t roundPageSize(uintptr_t addr)
{
	return (addr + pageSize - 1 ) & ~(pageSize - 1);
}

inline auto roundPageSize(uint8_t *p)
{
	auto origAddr = std::bit_cast<uintptr_t>(p);
	auto alignedAddr = roundPageSize(origAddr);
	return p + (alignedAddr - origAddr);
}

template <class T>
inline std::span<T> vNew(size_t size)
{
	auto buff = vAlloc(size * sizeof(T));
	return {reinterpret_cast<T*>(buff.data()), size};
}

template <class T>
inline void vDelete(std::span<T> buff)
{
	vFree(asWritableBytes(buff));
}

template <class T>
inline std::span<T> vNewMirrored(size_t size)
{
	static_assert(std::has_single_bit(sizeof(T)));
	auto buff = vAllocMirrored(roundPageSize(size * sizeof(T)));
	return {reinterpret_cast<T*>(buff.data()), buff.size_bytes() / sizeof(T)};
}

template<class T>
struct VPtrDeleter
{
	size_t size{};

	void operator()(T* ptr) const
	{
		for(auto &o : std::span{ptr, size}) // run all destructors
		{
			o.~T();
		}
		vFree({reinterpret_cast<uint8_t*>(ptr), size * sizeof(T)});
	}
};

template<class T>
using UniqueVPtr = std::unique_ptr<T[], VPtrDeleter<T>>;

template<class T>
inline UniqueVPtr<T> makeUniqueVPtr(size_t size)
{
	auto buff = vNew<T>(size);
	return {buff.data(), VPtrDeleter<T>{buff.size()}};
}

template<class T>
inline UniqueVPtr<T> makeUniqueMirroredVPtr(size_t size)
{
	auto buff = vNewMirrored<T>(size);
	return {buff.data(), VPtrDeleter<T>{buff.size()}};
}

template<class T>
inline void resetVPtr(UniqueVPtr<T> &ptr)
{
	ptr = {nullptr, VPtrDeleter<T>{}};
}

}
