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
	struct ForOverwrite{};

	constexpr DynArray() = default;
	constexpr explicit DynArray(size_t size):
		ptr{std::make_unique<T[]>(size)},
		size_{size} {}
	constexpr DynArray(size_t size, ForOverwrite):
		ptr{std::make_unique_for_overwrite<T[]>(size)},
		size_{size} {}
	constexpr auto data(this auto&& self) { return self.ptr.get(); }
	constexpr size_t size() const { return size_; }
	constexpr auto& operator[] (this auto&& self, size_t idx) { return self.data()[idx]; }
	constexpr auto begin(this auto&& self) { return self.data(); }
	constexpr auto end(this auto&& self) { return self.data() + self.size(); }
	constexpr std::span<T> span() const { return {data(), size()}; }
	constexpr operator std::span<T>() const { return span(); }
	auto reset(size_t size) { *this = DynArray{size}; }
	auto resetForOverwrite(size_t size) { *this = DynArray{size, ForOverwrite{}}; }
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

template<class T>
inline DynArray<T> dynArrayForOverwrite(size_t size) { return {size, typename DynArray<T>::ForOverwrite{}}; }

}
