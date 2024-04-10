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

#include <imagine/vmem/memory.hh>
#include <cassert>
#include <cstddef>
#include <iterator>

namespace IG
{

template<class T>
class VMemArray
{
public:
	using value_type = T;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	constexpr VMemArray() = default;

	VMemArray(size_t size)
	{
		resize(size);
	}

	// Iterators (STL API)
	iterator begin() { return data(); }
	iterator end() { return data() + size(); }
	const_iterator begin() const { return data(); }
	const_iterator end() const { return data() + size(); }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const { return rbegin(); }
	const_reverse_iterator crend() const { return rend(); }

	// Capacity (STL API)
	size_t size() const { return buff.get_deleter().size; }
	bool empty() const { return !size(); };
	size_t max_size() const { return size(); }

	void resize(size_t size_)
	{
		if(size_ == size())
			return;
		allocateStorage(size_);
	}

	// Element Access (STL API)
	T &front() { return at(0);	}
	T &back() { return at(size()-1);	}

	T &at(size_t idx)
	{
		assert(idx < size());
		return (*this)[idx];
	}

	T *data() { return buff.get(); }
	const T *data() const { return buff.get(); }

	T& operator[] (size_t idx) { return data()[idx]; }
	const T& operator[] (size_t idx) const { return data()[idx]; }

	void resetElements()
	{
		// destroy and re-allocate storage
		allocateStorage(size());
	}

private:
	UniqueVPtr<T> buff;

	void allocateStorage(size_t size)
	{
		buff.reset();
		buff = makeUniqueVPtr<T>(size);
	}

};

}
