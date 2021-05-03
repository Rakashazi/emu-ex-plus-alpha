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
#include <cstdint>

namespace IG
{

template<class T>
class BufferView
{
public:
	constexpr BufferView() {}
	constexpr BufferView(T *data, size_t size, void(*deleter)(T*)):
		data_{data, deleter}, size_{size} {}

	constexpr operator std::span<T>() const
	{
		return span();
	}

	constexpr std::span<T> span() const
	{
		return {data_.get(), size_};
	}

	constexpr T *data() const
	{
		return data_.get();
	}

	constexpr size_t size() const
	{
		return size_;
	}

	constexpr explicit operator bool() const
	{
		return data_.get();
	}

protected:
	std::unique_ptr<T[], void(*)(T*)> data_{nullptr, [](T*){}};
	size_t size_{};
};

using ByteBufferView = BufferView<uint8_t>;
using ConstByteBufferView = BufferView<const uint8_t>;

}
