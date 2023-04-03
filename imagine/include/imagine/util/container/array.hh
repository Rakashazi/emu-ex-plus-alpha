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
#include <cstdint>
#include <array>
#include <span>
#include <tuple>

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

constexpr auto toArray = [](auto &&...vals){ return std::array{IG_forward(vals)...}; };

constexpr auto concatToArray(auto &&...vals)
{
	return std::apply(toArray, std::tuple_cat(IG_forward(vals)...));
}

template <auto ...vals>
static constexpr auto concatToArrayNow = concatToArray(vals...);

}
