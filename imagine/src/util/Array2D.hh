#pragma once
#include <util/memory.h>
#include <assert.h>
#include <util/cLang.h>

template <class T>
struct Array2D
{
	T *arr = nullptr;
	uint columns = 0;

	constexpr Array2D() { }
	constexpr Array2D(T *arr, uint columns): arr(arr), columns(columns) { }

	operator T*() const
	{
		return arr;
	}

	T& operator[] (uint i) const
	{
		return arr[i];
	}

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
