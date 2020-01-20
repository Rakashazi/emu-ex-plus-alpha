#pragma once
#include <cstdint>
#include <assert.h>

template <class T>
class Mem2D
{
public:
	T *arr = nullptr;
	uint32_t columns = 0;

	constexpr Mem2D() {}
	constexpr Mem2D(T *arr, uint32_t columns): arr(arr), columns(columns) {}

	operator T*() const { return arr; }
	T& operator[] (int i) const { return arr[i]; }

	// offset in row-major order, standard C arrays
	static uint32_t arrOffsetRM(uint32_t col, uint32_t row, uint32_t numCols)
	{
		return (row*numCols) + col;
	}

	// offset in column-major order
	static uint32_t arrOffsetCM(uint32_t col, uint32_t row, uint32_t numRows)
	{
		return row + (col*numRows);
	}

	uint32_t idxOf(uint32_t row, uint32_t col) const
	{
		return arrOffsetRM(col, row, columns);
	}

	T& operator() (uint32_t row, uint32_t col) const
	{
		assert(col < columns);
		return arr[idxOf(row, col)];
	}
};
