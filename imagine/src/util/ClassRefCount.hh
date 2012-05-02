#pragma once

#include <assert.h>

template <class T>
class ClassRefCount
{
public:
	static int refCount;

	static void initClass()
	{
		refCount++;
		if(refCount == 1)
		{
			T::doInitClass();
		}
	}

	static void deinitClass()
	{
		refCount--;
		assert(refCount >= 0);
		if(refCount == 0)
		{
			T::doDeinitClass();
		}
	}
};

template <class T> int ClassRefCount<T>::refCount = 0;
