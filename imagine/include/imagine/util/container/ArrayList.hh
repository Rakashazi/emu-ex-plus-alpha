#pragma once
#include <imagine/util/ansiTypes.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/operators.hh>
#include "containerUtils.hh"
#include <assert.h>
#include <iterator>

template <class T, uint SIZE>
struct StaticStorageBase
{
	constexpr StaticStorageBase() { }
	T arr[SIZE];
	T *storage() { return arr; }
	const T *storage() const { return arr; }
	uint maxSize() const { return SIZE; }
};

template <class T>
struct PointerStorageBase
{
	constexpr PointerStorageBase() { }
	T *arr = nullptr;
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
	constexpr ArrayListBase() {}

	bool remove(const T &val)
	{
		iterateTimes(size(), i)
		{
			if(storage()[i] == val)
			{
				erase(&storage()[i]);
				return true;
			}
		}
		return false;
	}

	// Iterators (STL API)
	template <bool REVERSE = false>
	class ConstIterator : public std::iterator<std::random_access_iterator_tag, T>,
		public NotEquals<ConstIterator<REVERSE>>
	{
	public:
		//using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;
		T *p;

		constexpr ConstIterator(T *p): p(p) {}
		const T& operator*() const { return *p; }
		const T* operator->() const { return p; }
		void operator++() { p = REVERSE ? p-1 : p+1; }
		void operator--() { p = REVERSE ? p+1 : p-1; }
		bool operator==(ConstIterator const& rhs) const { return p == rhs.p; }
	};

	template <bool REVERSE = false>
	class Iterator : public ConstIterator<REVERSE>
	{
	public:
		using ConstIterator<REVERSE>::ConstIterator;
		using ConstIterator<REVERSE>::p;
		T& operator*() const { return *p; }
		T* operator->() const { return p; }
	};

	using iterator = Iterator<>;
	using const_iterator = ConstIterator<>;
	using reverse_iterator = Iterator<true>;
	using const_reverse_iterator = ConstIterator<true>;
	iterator begin() { return &(*this)[0]; }
	iterator end() { return &(*this)[size()]; }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }
	reverse_iterator rbegin() { return reverse_iterator(&(*this)[size()-1]); }
	reverse_iterator rend() { return reverse_iterator(&(*this)[-1]); }
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

	T& operator[] (int idx) { return storage()[idx]; }
	const T& operator[] (int idx) const { return storage()[idx]; }

	// Element Access
//	int contains(const T &d) const
//	{
//		iterateTimes(size(), i)
//		{
//			if(storage()[i] == d)
//				return 1;
//		}
//		return 0;
//	}

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
		storage()[size_] = d;
		size_++;
	}

	template <class... ARGS>
	void emplace_back(ARGS&&... args)
	{
		assert(size_ < max_size());
		new(&storage()[size_]) T(std::forward<ARGS>(args)...);
		size_++;
	}

	iterator insert(const_iterator position, const T& val)
	{
		ptrsize idx = position.p - &storage()[0];
		assert(idx <= size());
		ptrsize elemsAfterInsertIdx = size()-idx;
		if(elemsAfterInsertIdx)
		{
			memmove(&storage()[idx+1], &storage()[idx], sizeof(T)*elemsAfterInsertIdx);
		}
		storage()[idx] = val;
		size_++;
		return &storage()[idx];
	}

	iterator erase(const_iterator position)
	{
		ptrsize idx = position.p - &storage()[0];
		assert(idx < size());
		ptrsize elemsAfterEraseIdx = (size()-1)-idx;
		if(elemsAfterEraseIdx)
		{
			memmove(&storage()[idx], &storage()[idx+1], sizeof(T)*elemsAfterEraseIdx);
		}
		size_--;
		return &storage()[idx];
	}
};


template<class T, uint SIZE>
using StaticArrayList = ArrayListBase<T, StaticStorageBase<T, SIZE> >;
