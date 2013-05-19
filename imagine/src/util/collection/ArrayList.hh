#pragma once
#include <util/ansiTypes.h>
#include <util/cLang.h>

#define forEachInContainer(container, e) for(auto e = container.begin(); e != container.end(); e++)

template <class T, uint SIZE>
struct StaticStorageBase
{
	constexpr StaticStorageBase() { }
	T arr[SIZE];
	T *storage() { return arr; }
	uint maxSize() const { return SIZE; }
};

template <class T>
struct PointerStorageBase
{
	constexpr PointerStorageBase() { }
	T *arr = nullptr;
	T *storage() { return arr; }

	uint size = 0;
	void setStorage(T *s, uint size) { arr = s; var_selfs(size);}
	uint maxSize() const { return size; }
};

template<class T, class STORAGE_BASE>
class ArrayListBase : public STORAGE_BASE
{
public:
	using STORAGE_BASE::storage;
	constexpr ArrayListBase() {}

	using iterator = T*;
	using const_iterator = const T*;
	iterator begin() { return &(*this)[0]; }
	iterator end() { return &(*this)[size()]; }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

	uint size() const { return size_; }
	uint capacity() const { return STORAGE_BASE::maxSize(); }
	uint max_size() const { return STORAGE_BASE::maxSize(); }

	void clear()
	{
		logMsg("removing all array list items (%d)", size);
		size_ = 0;
	}

	T &front() { return at(0);	}
	T &back() { return at(size()-1);	}

	T &at(uint idx)
	{
		assert(idx < size());
		return (*this)[idx];
	}

	T& operator[] (uint idx) { return storage()[idx]; }
	const T& operator[] (uint idx) const { return storage()[idx]; }

	void pop_back()
	{
		assert(size_);
		size_--;
	}

	void resize(uint size)
	{
		assert(size <= max_size());
		size_= size;
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

	iterator erase(const_iterator position)
	{
		ptrsize idx = position - &storage()[0];
		assert(idx >= 0);
		assert(idx < size());
		ptrsize elemsAfterEraseIdx = (size()-1)-idx;
		if(elemsAfterEraseIdx)
		{
			memmove(&storage()[idx], &storage()[idx+1], sizeof(T)*elemsAfterEraseIdx);
		}
		size_--;
		return &storage()[idx];
	}

	int contains(const T &d) const
	{
		iterateTimes(size(), i)
		{
			if(storage()[i] == d)
				return 1;
		}
		return 0;
	}

	/*int remove(const T &d)
	{
		iterateTimes(size(), i)
		{
			if(storage()[i] == d)
			{
				erase(i);
				//logMsg("removed %p from list", &d);
				return 1;
			}
		}
		return 0;
	}

	int removeFirst()
	{
		if(list)
		{
			removeNode(list);
			return 1;
		}
		return 0;
	}*/

	bool isFull()
	{
		return !freeSpace();
	}

	uint freeSpace() const
	{
		return capacity() - size();
	}

private:
	uint size_ = 0;
};


template<class T, uint SIZE>
using StaticArrayList = ArrayListBase<T, StaticStorageBase<T, SIZE> >;
