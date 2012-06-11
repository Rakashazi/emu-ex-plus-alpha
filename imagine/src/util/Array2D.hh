#pragma once
#include <util/memory.h>
#include <assert.h>
#include <util/cLang.h>

template <class T>
struct Array2D
{
	constexpr Array2D() { }
	uint columns = 0;
	T *arr = nullptr;

	void init(T *arr, uint columns)
	{
		var_selfs(arr);
		var_selfs(columns);
	}

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
