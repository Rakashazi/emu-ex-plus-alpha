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

#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <cstdint>
#include <array>
#include <span>
#include <tuple>
#include <cassert>

namespace IG
{

template <size_t SIZE>
using ByteArray = std::array<uint8_t, SIZE>;

// simple 2D view into an array
template <class T>
struct ArrayView2
{
	T *arr{};
	std::size_t pitch{};

	constexpr std::size_t flatOffset(std::size_t row, std::size_t col) const
	{
		return (row * pitch) + col;
	}

	constexpr std::span<T> operator[](std::size_t row) const
	{
		return {arr + (row * pitch), pitch};
	}

	constexpr T *data() { return arr; }
};

// Static array for storing non-null elements, terminated by null
template<class T, size_t maxSize>
struct ZArray
{
	T arr[maxSize]{};

	constexpr auto &operator[](size_t pos) { return arr[pos]; }
	constexpr auto &operator[](size_t pos) const { return arr[pos]; }
	constexpr auto data() const { return std::data(arr); }
	constexpr auto data() { return std::data(arr); }
	constexpr size_t size() const { return findIndex(arr, T{}, maxSize); }
	constexpr size_t capacity() const { return maxSize; }
	constexpr auto begin() { return data(); }
	constexpr auto begin() const { return data(); }
	constexpr auto end() { return data() + size(); }
	constexpr auto end() const { return data() + size(); }
	constexpr bool operator==(const ZArray &) const = default;
	constexpr size_t freeSpace() const { return capacity() - size(); }
	constexpr bool isFull() const { return !freeSpace(); }

	constexpr void push_back(const T &d)
	{
		assert(size() < capacity());
		data()[size()] = d;
	}

	constexpr bool tryPushBack(const T &d)
	{
		if(isFull())
			return false;
		push_back(d);
		return true;
	}
};

constexpr auto toArray = [](auto &&...vals){ return std::array{IG_forward(vals)...}; };

constexpr auto concatToArray(auto &&...vals)
{
	return std::apply(toArray, std::tuple_cat(IG_forward(vals)...));
}

template <auto ...vals>
static constexpr auto concatToArrayNow = concatToArray(vals...);

template <typename Type, typename... Values>
constexpr auto makeArray(Values &&... v) -> std::array <Type, sizeof...(Values)>
{
	return {{std::forward<Values>(v)...}};
}

}
