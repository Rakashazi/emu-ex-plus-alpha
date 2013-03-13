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

#include <string.h>
#include <stdio.h>
#include <util/ansiTypes.h>
#include <util/cLang.h>
#include <assert.h>

static void mem_zero(void *buff, size_t count)
{
	memset(buff, 0, count);
}

#ifdef __cplusplus

template <class T>
static void mem_zero(T (&buff))
{
	memset(&buff, 0, sizeof(T));
}

template <class T, size_t S>
static void mem_setElem(T (&arr)[S], T val)
{
	forEachInArray(arr, e)
		*e = val;
}

namespace IG
{

template <class T>
static void swap(T& a, T& b)
{
	T temp = a; a = b;  b = temp;
}

}

template <class T>
static void toggle(T& x)
{
	x = (x == 0) ? 1 : 0;
}

#endif

#define sizeofArrayConst(x) (sizeof(x) / sizeof((x)[0]))

// offset in row-major order, standard C arrays
static uint mem_arr2DOffsetRM(uint col, uint row, uint numCols)
{
	return (row*numCols) + col;
}

// offset in column-major order
static uint mem_arr2DOffsetCM(uint col, uint row, uint numRows)
{
	return row + (col*numRows);
}

static int mem_equal(const void *m1, const void *m2, size_t len)
{
	return memcmp(m1, m2, len) == 0;
}

// logical xor
#define lxor(a, b) ( !(a) != !(b) )

static void printMem(const void *mem, int count)
{
	for(int i = 0; i < count; i++)
	{
		printf("0x%X ",((const unsigned char*)mem)[i]);
	}
	printf("\n");
}

// Creates a static array (in the global variable area) and returns it's address
// Each statement creates its own unique array.
// Using it in a loop or a function called multiple times still
// returns the address of the same array.
// TODO: align the array for speed/compatibility with platforms that need aligned access
#define mem_static(size) ({ static uchar* store[size]; (void*)store; })
