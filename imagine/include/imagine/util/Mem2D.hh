#pragma once
#include <imagine/util/memory.h>
#include <assert.h>

template <class T>
class Mem2D
{
public:
	T *arr = nullptr;
	uint columns = 0;

	constexpr Mem2D() {}
	constexpr Mem2D(T *arr, uint columns): arr(arr), columns(columns) {}

	operator T*() const { return arr; }
	T& operator[] (int i) const { return arr[i]; }

	uint idxOf(uint row, uint col) const
	{
		return mem_arr2DOffsetRM(col, row, columns);
	}

	T& operator() (uint row, uint col) const
	{
		assert(col < columns);
		return arr[idxOf(row, col)];
	}
};
