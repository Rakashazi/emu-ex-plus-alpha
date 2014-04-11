#pragma once
#include <string.h>
#include <imagine/util/ansiTypes.h>
#include <imagine/util/algorithm.h>

namespace IG
{

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
}

template <class T, size_t S>
static T* mem_findFirstZeroValue(T (&a)[S])
{
	return mem_findFirstZeroValue(a, S);
}

}

static const void* mem_locate(const void *voidData, const void *endAddr, const void *voidSeq, uint seqLen)
{
	const char *data = (const char*)voidData, *seq = (const char*)voidSeq;
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
	return mem_locate(data, (char*)data + size, seqStr, strlen(seqStr));
}

static const void* mem_locate(const void *data, size_t size, const void *voidSeq, uint seqLen)
{
	return mem_locate(data, (char*)data + size, voidSeq, seqLen);
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
	const void *offset = mem_locate(data, (char*)data + size, voidSeq, seqLen);
	if(!offset)
		return 0;
	return (ptrsize)offset - (ptrsize)data;
}
#endif


// Find the offset of first different byte in 2 buffers
static size_t mem_locateDiff(const void *vdata1, const void *vdata2, size_t size)
{
	const char *data1 = (const char*)vdata1, *data2 = (const char*)vdata2;
	iterateTimes(size, i)
	{
		if(data1[i] != data2[i])
			return i;
	}
	return size;
}
