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

#include <concepts>
#include <ranges>

namespace IG
{

template<std::integral T>
constexpr auto iotaCount(T count) { return std::views::iota((T)0, count); }

template<std::ranges::range T>
constexpr auto enumerate(T&& rng) { return std::views::zip(std::views::iota(0), std::forward<T>(rng)); }

template<std::ranges::range T>
constexpr auto lastIndex(T&& rng) { return std::ranges::size(std::forward<T>(rng)) - 1; }

constexpr auto toIterator(std::ranges::range auto&& rng, auto& elem) { return rng.begin() + std::distance(rng.data(), &elem); }

}
