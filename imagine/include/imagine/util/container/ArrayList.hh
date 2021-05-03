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

#include <assert.h>
#include <cstddef>
#include <iterator>
#include <cstring>

template <class T, size_t SIZE>
class StaticStorageBase
{
public:
	constexpr size_t maxSize() const { return SIZE; }

protected:
	T arr[SIZE];

	constexpr StaticStorageBase() {}
	constexpr T *storage() { return arr; }
	constexpr const T *storage() const { return arr; }
};

template <class T>
class PointerStorageBase
{
public:
	constexpr size_t maxSize() const { return size; }

protected:
	T *arr{};
	size_t size{};

	constexpr PointerStorageBase() {}
	constexpr T *storage() { return arr; }
	constexpr const T *storage() const { return arr; }
	constexpr void setStorage(T *s, size_t size) { arr = s; this->size = size;}
};

template<class T, class STORAGE_BASE>
class ArrayListBase : public STORAGE_BASE
{
public:
	using STORAGE_BASE::storage;
	using value_type = T;
	using size_type = size_t;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	constexpr ArrayListBase() {}

	// Iterators (STL API)
	constexpr iterator begin() { return data(); }
	constexpr iterator end() { return data() + size(); }
	constexpr const_iterator begin() const { return data(); }
	constexpr const_iterator end() const { return data() + size(); }
	constexpr const_iterator cbegin() const { return begin(); }
	constexpr const_iterator cend() const { return end(); }
	constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }
	constexpr reverse_iterator rend() { return reverse_iterator(begin()); }
	constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	constexpr const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	constexpr const_reverse_iterator crbegin() const { return rbegin(); }
	constexpr const_reverse_iterator crend() const { return rend(); }

	// Capacity (STL API)
	constexpr size_t size() const { return size_; }
	constexpr bool empty() const { return !size(); };
	constexpr size_t capacity() const { return STORAGE_BASE::maxSize(); }
	constexpr size_t max_size() const { return STORAGE_BASE::maxSize(); }

	constexpr void resize(size_t size)
	{
		assert(size <= max_size());
		size_= size;
	}

	// Capacity
	constexpr bool isFull() const
	{
		return !freeSpace();
	}

	constexpr size_t freeSpace() const
	{
		return capacity() - size();
	}

	// Element Access (STL API)
	constexpr T &front() { return at(0);	}
	constexpr T &back() { return at(size()-1);	}

	constexpr T &at(size_t idx)
	{
		assert(idx < size());
		return (*this)[idx];
	}

	constexpr T *data() { return storage(); }
	constexpr const T *data() const { return storage(); }

	constexpr T& operator[] (int idx) { return data()[idx]; }
	constexpr const T& operator[] (int idx) const { return data()[idx]; }

	// Modifiers (STL API)
	constexpr void clear()
	{
		//logMsg("removing all array list items (%d)", size_);
		size_ = 0;
	}

	constexpr void pop_back()
	{
		assert(size_);
		size_--;
	}

	constexpr void push_back(const T &d)
	{
		assert(size_ < max_size());
		data()[size_] = d;
		size_++;
	}

	template <class... ARGS>
	constexpr T &emplace_back(ARGS&&... args)
	{
		assert(size_ < max_size());
		auto newAddr = &data()[size_];
		new(newAddr) T(std::forward<ARGS>(args)...);
		size_++;
		return *newAddr;
	}

	constexpr iterator insert(const_iterator position, const T& val)
	{
		// TODO: re-write using std::move
		uintptr_t idx = position - data();
		assert(idx <= size());
		uintptr_t elemsAfterInsertIdx = size()-idx;
		if(elemsAfterInsertIdx)
		{
			std::memmove(&data()[idx+1], &data()[idx], sizeof(T)*elemsAfterInsertIdx);
		}
		data()[idx] = val;
		size_++;
		return &data()[idx];
	}

	constexpr iterator erase(iterator position)
	{
		if(position + 1 != end())
			std::move(position + 1, end(), position);
		size_--;
		return position;
	}

	constexpr iterator erase(iterator first, iterator last)
	{
		if(first != last)
		{
			if(last != end())
				std::move(last, end(), first);
			size_ -= std::distance(first, last);
		}
		return first;
	}

private:
	size_t size_{};
};

template<class T, size_t SIZE>
using StaticArrayList = ArrayListBase<T, StaticStorageBase<T, SIZE> >;

namespace IG
{

template<class T, size_t SIZE, class Pred>
static constexpr typename StaticArrayList<T,SIZE>::size_type
	erase_if(StaticArrayList<T,SIZE>& c, Pred pred)
{
	auto it = std::remove_if(c.begin(), c.end(), pred);
	auto r = std::distance(it, c.end());
	c.erase(it, c.end());
	return r;
}

}
