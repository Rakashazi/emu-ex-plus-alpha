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

#include <imagine/util/ranges.hh>
#include <memory>
#include <cstdint>

namespace IG
{

// A wrapper to ease working with an array of structs containing
// a flex array member, e.g. struct { Header header; uint8_t data[]; }

template<class T>
class FlexArray
{
public:
	constexpr FlexArray() = default;
	constexpr explicit FlexArray(size_t size, size_t flexSize):
		ptr{std::make_unique_for_overwrite<uint8_t[]>(alignedSize(sizeof(T) + flexSize) * size)},
		size_{size},
		elemSize{alignedSize(sizeof(T) + flexSize)}
	{
		resetStaticData();
	}

	void resetStaticData()
	{
		for(auto i : iotaCount(size()))
		{
			(*this)[i] = {};
		}
	}

	auto data(this auto&& self) { return reinterpret_cast<T*>(self.ptr.get()); }
	constexpr size_t size() const { return size_; }
	constexpr size_t elementSize() const { return elemSize; }
	auto& operator[](this auto&& self, size_t idx) { return reinterpret_cast<T&>(self.ptr[idx * self.elemSize]); }
	auto release() { return ptr.release(); }

	auto reset(size_t size, size_t flexSize)
	{
		if(size)
			*this = FlexArray{size, flexSize};
		else
			*this = {};
	}

private:
	std::unique_ptr<uint8_t[]> ptr;
	size_t size_{};
	size_t elemSize{};

	static constexpr size_t alignedSize(size_t s)
	{
		static constexpr size_t alignBits = alignof(T) - 1;
		return (s + alignBits) & ~alignBits;
	}
};

}
