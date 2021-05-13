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

#include <vector>
#include <iterator>
#include <utility>

namespace IG
{

template <class T, class COMPARE = std::less<T>>
class FlatSet
{
public:
	using VectorType = std::vector<T>;
	using key_type = T;
	using value_type = T;
	using size_type = typename VectorType::size_type;
	using iterator = typename VectorType::iterator;
	using const_iterator = typename VectorType::const_iterator;
	using reverse_iterator = typename VectorType::reverse_iterator;
	using const_reverse_iterator = typename VectorType::const_reverse_iterator;

	constexpr FlatSet(const COMPARE& c = COMPARE()): c{c} {}

	// Iterators (STL API)
	iterator begin() { return v.begin(); }
	iterator end() { return v.end(); }
	const_iterator begin() const { return v.begin(); }
	const_iterator end() const { return v.end(); }
	const_iterator cbegin() const { return v.cbegin(); }
	const_iterator cend() const { return v.cend(); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const { return rbegin(); }
	const_reverse_iterator crend() const { return rend(); }

	// Capacity (STL API)
	size_type size() const { return v.size(); }
	bool empty() const { return v.empty(); };
	size_type max_size() const { return v.max_size(); }

	// Element Access (STL API)
	T &front() { return v.front(); }
	T &back() { return v.back(); }

	T &at(size_type idx)
	{
		return v.at(idx);
	}

	T *data() { return v.data(); }
	const T *data() const { return v.data(); }

	T& operator[] (int idx) { return v[idx]; }
	const T& operator[] (int idx) const { return v[idx]; }

	// Modifiers (STL API)
	void clear()
	{
		v.clear();
	}

	template <class... ARGS>
	std::pair<iterator, bool> emplace(ARGS&&... args)
	{
		return insert(T(std::forward<ARGS>(args)...));
	}

	std::pair<iterator, bool> insert(const T& val)
	{
		iterator i = std::lower_bound(begin(), end(), val, c);
		bool inserted = false;
		if (i == end() || c(val, *i))
		{
			v.insert(i, val);
			inserted = true;
		}
		return std::make_pair(i, inserted);
	}

	iterator erase(iterator position)
	{
		return v.erase(position);
	}

	iterator erase(iterator first, iterator last)
	{
		return v.erase(first, last);
	}

	const_iterator find(const T& val) const
	{
		const_iterator i = std::lower_bound(begin(), end(), val, c);
		return i == end() || c(val, *i) ? end() : i;
	}

protected:
	VectorType v{};
	[[no_unique_address]] COMPARE c;
};

template <class T, class COMPARE = std::less<T>>
class FlatMultiSet : public FlatSet<T, COMPARE>
{
public:
	using FlatSet<T, COMPARE>::FlatSet;
	using iterator = typename FlatSet<T, COMPARE>::iterator;

	template <class... ARGS>
	iterator emplace(ARGS&&... args)
	{
		return insert(T(std::forward<ARGS>(args)...));
	}

	iterator insert(const T& val)
	{
		iterator i = std::lower_bound(this->begin(), this->end(), val, this->c);
		this->v.insert(i, val);
		return i;
	}
};

}
