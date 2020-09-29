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

#include <cstddef>

namespace IG
{

void *allocVMem(size_t bytes);
void freeVMem(void *vMemPtr, size_t bytes);
size_t adjustVMemAllocSize(size_t bytes);
void *allocMirroredBuffer(size_t bytes);
void freeMirroredBuffer(void *vMemPtr, size_t bytes);

template<class T>
T *allocVMemObjects(size_t size)
{
	return (T*)allocVMem(adjustVMemAllocSize(sizeof(T) * size));
}

template<class T>
void freeVMemObjects(T *vMemPtr, size_t size)
{
	freeVMem(vMemPtr, adjustVMemAllocSize(sizeof(T) * size));
}

}
