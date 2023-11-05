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

#include <memory>
#include <span>

namespace IG
{

// A simple wrapper around an array unique_ptr & size with utility functions

template<class T>
class DynArray
{
public:
	constexpr DynArray() = default;
	constexpr explicit DynArray(size_t size):
		ptr{std::make_unique<T[]>(size)},
		size_{size} {}

		constexpr T *data() const { return ptr.get(); }
		constexpr size_t size() const { return size_; }
		constexpr T& operator[] (size_t idx) { return data()[idx]; }
		constexpr const T& operator[] (size_t idx) const { return data()[idx]; }
		constexpr auto begin() { return data(); }
		constexpr auto end() { return data() + size(); }
		constexpr auto begin() const { return data(); }
		constexpr auto end() const { return data() + size(); }
		constexpr std::span<T> span() const { return {data(), size()}; }
		constexpr operator std::span<T>() const { return span(); }
		auto reset(size_t size) { *this = DynArray{size}; }
		auto release() { return ptr.release(); }

		constexpr void trim(size_t smallerSize)
		{
			if(smallerSize < size_)
				size_ = smallerSize;
		}

private:
	std::unique_ptr<T[]> ptr;
	size_t size_{};
};


}
