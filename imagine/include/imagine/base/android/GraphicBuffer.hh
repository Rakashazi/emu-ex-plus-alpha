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
#include <imagine/pixmap/PixmapDesc.hh>
#include "gralloc.h"
#include <EGL/egl.h>
#include <string_view>

// Wrapper for ANativeWindowBuffer (android_native_buffer_t)
// similar to GraphicBuffer class in Android frameworks

struct android_native_buffer_t;

namespace IG
{

class ApplicationContext;
struct PixmapDesc;

class GraphicBuffer : public android_native_buffer_t
{
public:
	GraphicBuffer();
	GraphicBuffer(PixmapDesc desc, uint32_t usage);
	GraphicBuffer(uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
	GraphicBuffer(GraphicBuffer &&o) noexcept;
	GraphicBuffer &operator=(GraphicBuffer &&o) noexcept;
	~GraphicBuffer();
	bool lock(uint32_t usage, void **outAddr);
	bool lock(uint32_t usage, WindowRect rect, void **outAddr);
	void unlock();
	explicit operator bool() const;
	uint32_t pitch();
	android_native_buffer_t *nativeObject();
	EGLClientBuffer eglClientBuffer();
	static bool hasBufferMapper();
	static bool canSupport(ApplicationContext, std::string_view rendererStr);
	static bool testSupport();
	static bool isSupported();

private:
	void deinit();
};

}
