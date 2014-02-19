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

#include <engine-globals.h>
#include <io/mmap/IoMmap.hh>
#include <mem/interface.h>

class IoMmapGeneric : public IoMmap
{
public:
	static Io* open(const char * buffer, size_t size);
	~IoMmapGeneric() { close(); }
	void close() override;

	// optional function to call on close, <ptr> is the buffer passed during open()
	typedef void (*FreeFunc)(void *ptr);
	void memFreeFunc(FreeFunc free);

private:
	FreeFunc free = nullptr;
};
