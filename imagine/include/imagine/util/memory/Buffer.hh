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

#include <imagine/util/concepts.hh>
#include <imagine/util/DelegateFunc.hh>
#include <memory>
#include <span>
#include <string_view>
#include <cstdint>
#include <cassert>

namespace IG
{

template<class T, size_t deleterSize>
struct BufferDeleter
{
	using DeleterFunc = DelegateFuncS<deleterSize, void(T *dataPtr, size_t size)>;

	DeleterFunc del{};
	size_t size{};

	constexpr void operator()(T *ptr) const
	{
		del.callSafe(ptr, size);
	}

	constexpr bool operator ==(BufferDeleter const&) const = default;
};

// Wrapper around unique_ptr with custom deleter & a size, used to pass buffers allocated
// with different APIs (new, mmap, Android Assets, etc.) using the same interface

template<class T, size_t deleterSize = sizeof(void*)>
class Buffer
{
public:
	using Deleter = BufferDeleter<std::add_const_t<T>, deleterSize>;
	using DeleterFunc = typename Deleter::DeleterFunc;
	friend Buffer<std::add_const_t<T>>;

	constexpr Buffer() = default;

	constexpr Buffer(std::span<T> span, DeleterFunc deleter = {}):
		data_{span.data(), {deleter, span.size()}} {}

	constexpr Buffer(size_t size):
		data_{new T[size], {[](const T *ptr, size_t){ delete[] ptr; }, size}} {}

	constexpr Buffer(std::unique_ptr<T[]> ptr, size_t size):
		data_{ptr.release(), {[](const T *ptr, size_t){ delete[] ptr; }, size}} {}

	constexpr Buffer(Buffer &&) = default;
	constexpr Buffer &operator=(Buffer &&) = default;

	// Convert non-const T version of Buffer to const
	constexpr Buffer(Buffer<std::remove_const_t<T>> &&o) requires IG::Const<T>:
		data_{std::move(o.data_)} {}

	constexpr operator std::span<T>() const
	{
		return span();
	}

	constexpr std::span<T> span() const
	{
		return {data_.get(), size()};
	}

	constexpr std::string_view stringView(size_t offset, size_t viewSize) const requires (sizeof(T) == 1)
	{
		assert(offset + viewSize <= size());
		return {reinterpret_cast<const char*>(data() + offset), viewSize};
	}

	constexpr std::string_view stringView() const requires (sizeof(T) == 1)
	{
		return stringView(0, size());
	}

	constexpr auto data(this auto&& self) { return self.data_.get(); }

	constexpr size_t size() const
	{
		return data_.get_deleter().size;
	}

	constexpr auto& operator[] (this auto&& self, size_t idx) { return self.data()[idx]; }
	constexpr auto begin(this auto&& self) { return self.data(); }
	constexpr auto end(this auto&& self) { return self.data() + self.size(); }

	constexpr explicit operator bool() const
	{
		return data_.get();
	}

protected:
	std::unique_ptr<T[], Deleter> data_;
};

using ByteBuffer = Buffer<uint8_t>;
using ConstByteBuffer = Buffer<const uint8_t>;

template<size_t deleterSize>
using ByteBufferS = Buffer<uint8_t, deleterSize>;

template<size_t deleterSize>
using ConstByteBufferS = Buffer<const uint8_t, deleterSize>;

}
