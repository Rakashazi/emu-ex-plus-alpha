#pragma once
#include <imagine/util/ansiTypes.h>
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

	// offset in row-major order, standard C arrays
	static uint arrOffsetRM(uint col, uint row, uint numCols)
	{
		return (row*numCols) + col;
	}

	// offset in column-major order
	static uint arrOffsetCM(uint col, uint row, uint numRows)
	{
		return row + (col*numRows);
	}

	uint idxOf(uint row, uint col) const
	{
		return arrOffsetRM(col, row, columns);
	}

	T& operator() (uint row, uint col) const
	{
		assert(col < columns);
		return arr[idxOf(row, col)];
	}
};
