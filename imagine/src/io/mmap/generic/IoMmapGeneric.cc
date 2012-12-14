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

#define thisModuleName "io:mmap:generic"
#include <logger/interface.h>

#include "IoMmapGeneric.hh"

Io* IoMmapGeneric::open(const uchar *buffer, size_t size)
{
	IoMmapGeneric *inst = new IoMmapGeneric;
	if(inst == NULL)
	{
		logErr("out of memory");
		return NULL;
	}

	inst->init(buffer, size);
	inst->free = NULL;
	return inst;
}

void IoMmapGeneric::memFreeFunc(FreeFunc free)
{
	this->free = free;
}

void IoMmapGeneric::close()
{
	if(free)
		free((void*)data);
}
