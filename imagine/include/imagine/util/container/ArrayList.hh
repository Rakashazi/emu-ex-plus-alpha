#pragma once
#include <imagine/util/ansiTypes.h>
#include "containerUtils.hh"
#include <assert.h>
#include <iterator>
#include <algorithm>

template <class T, uint SIZE>
struct StaticStorageBase
{
	constexpr StaticStorageBase() {}
	T arr[SIZE]{};
	T *storage() { return arr; }
	const T *storage() const { return arr; }
	constexpr uint maxSize() const { return SIZE; }
};

template <class T>
struct PointerStorageBase
{
	constexpr PointerStorageBase() {}
	T *arr{};
	T *storage() { return arr; }
	const T *storage() const { return arr; }

	uint size = 0;
	void setStorage(T *s, uint size) { arr = s; var_selfs(size);}
	uint maxSize() const { return size; }
};

template<class T, class STORAGE_BASE>
class ArrayListBase : public STORAGE_BASE
{
private:
	uint size_ = 0;

public:
	using STORAGE_BASE::storage;
	using value_type = T;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	constexpr ArrayListBase() {}

	bool remove(const T &val)
	{
		auto it = std::find(begin(), end(), val);
		if(it == end())
			return false;
		erase(it);
		return true;
	}

	// Iterators (STL API)
	iterator begin() { return data(); }
	iterator end() { return data() + size(); }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const { return rbegin(); }
	const_reverse_iterator crend() const { return rend(); }

	// Capacity (STL API)
	uint size() const { return size_; }
	bool empty() const { return !size(); };
	uint capacity() const { return STORAGE_BASE::maxSize(); }
	uint max_size() const { return STORAGE_BASE::maxSize(); }

	void resize(uint size)
	{
		assert(size <= max_size());
		size_= size;
	}

	// Capacity
	bool isFull() const
	{
		return !freeSpace();
	}

	uint freeSpace() const
	{
		return capacity() - size();
	}

	// Element Access (STL API)
	T &front() { return at(0);	}
	T &back() { return at(size()-1);	}

	T &at(uint idx)
	{
		assert(idx < size());
		return (*this)[idx];
	}

	T *data() { return storage(); }
	const T *data() const { return storage(); }

	T& operator[] (int idx) { return data()[idx]; }
	const T& operator[] (int idx) const { return data()[idx]; }

	// Modifiers (STL API)
	void clear()
	{
		//logMsg("removing all array list items (%d)", size_);
		size_ = 0;
	}

	void pop_back()
	{
		assert(size_);
		size_--;
	}

	void push_back(const T &d)
	{
		assert(size_ < max_size());
		data()[size_] = d;
		size_++;
	}

	template <class... ARGS>
	void emplace_back(ARGS&&... args)
	{
		assert(size_ < max_size());
		new(&data()[size_]) T(std::forward<ARGS>(args)...);
		size_++;
	}

	iterator insert(const_iterator position, const T& val)
	{
		ptrsize idx = position - data();
		assert(idx <= size());
		ptrsize elemsAfterInsertIdx = size()-idx;
		if(elemsAfterInsertIdx)
		{
			memmove(&data()[idx+1], &data()[idx], sizeof(T)*elemsAfterInsertIdx);
		}
		data()[idx] = val;
		size_++;
		return &data()[idx];
	}

	iterator erase(const_iterator position)
	{
		ptrsize idx = position - data();
		assert(idx < size());
		ptrsize elemsAfterEraseIdx = (size()-1)-idx;
		if(elemsAfterEraseIdx)
		{
			memmove(&data()[idx], &data()[idx+1], sizeof(T)*elemsAfterEraseIdx);
		}
		size_--;
		return &data()[idx];
	}
};


template<class T, uint SIZE>
using StaticArrayList = ArrayListBase<T, StaticStorageBase<T, SIZE> >;
