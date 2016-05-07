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

#include <imagine/util/rectangle2.h>
#include "gralloc.h"

// Wrapper for ANativeWindowBuffer (android_native_buffer_t)
// similar to GraphicBuffer class in Android frameworks

namespace Base
{

class GraphicBuffer : public android_native_buffer_t
{
public:
	GraphicBuffer();
	~GraphicBuffer();
	bool reallocate(uint w, uint h, uint format, uint usage);
	bool lock(uint usage, void **vaddr);
	bool lock(uint usage, IG::WindowRect rect, void **vaddr);
	void unlock();
	uint getWidth();
	uint getHeight();
	uint getStride();
	android_native_buffer_t *getNativeBuffer();
	static bool hasBufferMapper();

private:
	bool initSize(uint w, uint h, uint format, uint usage);
};

}
