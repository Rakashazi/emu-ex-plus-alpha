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
#include <assert.h>
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

	constexpr VMemArray() {}

	VMemArray(unsigned size)
	{
		resize(size);
	}

	VMemArray(VMemArray &&o)
	{
		*this = std::move(o);
	}

	VMemArray &operator=(VMemArray &&o)
	{
		freeStorage();
		data_ = std::exchange(o.data_, {});
		size_ = std::exchange(o.size_, 0);
		return *this;
	}

	~VMemArray()
	{
		freeStorage();
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
	size_t size() const { return size_; }
	bool empty() const { return !size(); };
	size_t max_size() const { return size(); }

	void resize(size_t size)
	{
		if(size == size_)
			return;
		allocateStorage(size);
	}

	// Element Access (STL API)
	T &front() { return at(0);	}
	T &back() { return at(size()-1);	}

	T &at(size_t idx)
	{
		assert(idx < size());
		return (*this)[idx];
	}

	T *data() { return data_; }
	const T *data() const { return data_; }

	T& operator[] (int idx) { return data()[idx]; }
	const T& operator[] (int idx) const { return data()[idx]; }

	void resetElements()
	{
		// destroy and re-allocate storage
		allocateStorage(size());
	}

private:
	T *data_{};
	unsigned size_ = 0;

	void freeStorage()
	{
		if(!size_)
			return;
		for(auto &o : *this) // run all destructors
		{
			o.~T();
		}
		freeVMemObjects(data_, size_);
		data_ = {};
		size_ = 0;
	}

	void allocateStorage(size_t size)
	{
		freeStorage();
		if(!size)
			return;
		data_ = allocVMemObjects<T>(size);
		if(!data_) [[unlikely]]
			return;
		size_= size;
	}

};

}
