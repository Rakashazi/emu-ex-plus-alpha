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

// Static array for storing non-null elements, terminated by null
template<class T, size_t maxSize>
class ZArray : public std::array<T, maxSize>
{
public:
	using Base = std::array<T, maxSize>;
	using iterator = T*;
	using const_iterator = const T*;

	// Define constructor so underlying array is zero-init
	constexpr ZArray(auto &&...args): Base{IG_forward(args)...} {}
	constexpr size_t size() const { return findIndex(std::span{this->data(), maxSize}, T{}, maxSize); }
	constexpr size_t capacity() const { return maxSize; }
	constexpr auto begin(this auto&& self) { return self.data(); }
	constexpr auto end(this auto&& self) { return self.data() + self.size(); }
	constexpr const_iterator cbegin() const { return begin(); }
	constexpr const_iterator cend() const { return end(); }
	constexpr size_t freeSpace() const { return capacity() - size(); }
	constexpr bool isFull() const { return !freeSpace(); }

	constexpr void push_back(const T &val)
	{
		assert(size() < capacity());
		this->data()[size()] = val;
	}

	constexpr iterator insert(const_iterator position, const T &val)
	{
		assert(size() < maxSize);
		iterator p{this->data() + (position - this->cbegin())};
		if(p == end())
		{
			push_back(val);
		}
		else
		{
			std::move_backward(p, end(), end() + 1);
			*p = val;
		}
		return p;
	}

	constexpr bool tryPushBack(const T &val)
	{
		if(isFull())
			return false;
		push_back(val);
		return true;
	}

	constexpr bool tryInsert(const_iterator position, const T &val)
	{
		if(isFull())
			return false;
		insert(position, val);
		return true;
	}
};

template <class T, size_t size>
class RingArray: public std::array<T, size>
{
public:
	constexpr void push_back(const T &e)
	{
		this->data()[writeIdx] = e;
		writeIdx = (writeIdx + 1) % size;
	}

private:
	size_t writeIdx{};
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
