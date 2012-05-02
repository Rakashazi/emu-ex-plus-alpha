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

template <class T>
static T* mem_findFirstValue(T *start, uint count, int val)
{
	iterateTimes(count, i)
	{
		if(start[i] == val)
			return &start[i];
	}
	return 0;
}

template <class T, size_t S>
static T* mem_findFirstValue(T (&a)[S], int val)
{
	return mem_findFirstValue(a, S, val);
}

template <class T>
static T* mem_findFirstZeroValue(T *start, uint count)
{
	return mem_findFirstValue(start, count, 0);
	/*iterateTimes(count, i)
	{
		if(start[i] == 0)
			return &start[i];
	}
	return 0;*/
}

template <class T, size_t S>
static T* mem_findFirstZeroValue(T (&a)[S])
{
	return mem_findFirstZeroValue(a, S);
}

#endif

#define sizeofArrayConst(x) (sizeof(x) / sizeof((x)[0]))
//#define sizeof2DArray(x) (sizeof(x) / sizeof((x)[0][0]))

// version for template args in form T *arr, where arr is a static sized array
//#define sizeofArrayT(x) (sizeof(typeof(*x)) / sizeof(x[0]))

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

static int mem_equal(const void *m1, const void *m2, size_t len)
{
	return memcmp(m1, m2, len) == 0;
}

static const void* mem_locate(const void *voidData, const void *endAddr, const void *voidSeq, uint seqLen)
{
	const uchar *data = (const uchar*)voidData, *seq = (const uchar*)voidSeq;
	assert(data < endAddr);
	for(uint count = 0, seqByte = 0; &data[count] != endAddr; count++)
	{
		if(data[count] == seq[seqByte])
		{
			seqByte++;
			if(seqByte == seqLen)
			{
				// whole sequence found
				return &data[count-(seqLen-1)];
			}
		}
		else seqByte = 0;
	}
	// no sequence found
	return 0;
}

#ifdef __cplusplus
static const void* mem_locate(const void *data, size_t size, const char *seqStr)
{
	return mem_locate(data, (uchar*)data + size, seqStr, strlen(seqStr));
}

static const void* mem_locate(const void *data, size_t size, const void *voidSeq, uint seqLen)
{
	return mem_locate(data, (uchar*)data + size, voidSeq, seqLen);
}

static ptrsize mem_locateRelPos(const void *data, size_t size, const char *seqStr)
{
	const void *offset = mem_locate(data, size, seqStr);
	if(!offset)
		return 0;
	return (ptrsize)offset - (ptrsize)data;
}

static ptrsize mem_locateRelPos(const void *data, size_t size, const void *voidSeq, uint seqLen)
{
	const void *offset = mem_locate(data, (uchar*)data + size, voidSeq, seqLen);
	if(!offset)
		return 0;
	return (ptrsize)offset - (ptrsize)data;
}
#endif


// Find the offset of first different byte in 2 buffers
static size_t mem_locateDiff(const void *vdata1, const void *vdata2, size_t size)
{
	const uchar *data1 = (const uchar*)vdata1, *data2 = (const uchar*)vdata2;
	iterateTimes(size, i)
	{
		if(data1[i] != data2[i])
			return i;
	}
	return size;
}

// Creates a static array (in the global variable area) and returns it's address
// Each statement creates its own unique array.
// Using it in a loop or a function called multiple times still
// returns the address of the same array.
// TODO: align the array for speed/compatibility with platforms that need aligned access
#define mem_static(size) ({ static uchar* store[size]; (void*)store; })
