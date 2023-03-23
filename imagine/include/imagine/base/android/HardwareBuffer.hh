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
#include <EGL/egl.h>
#include <memory>

struct AHardwareBuffer;

namespace IG
{
struct PixmapDesc;
}

namespace IG
{

class HardwareBuffer
{
public:
	HardwareBuffer();
	HardwareBuffer(PixmapDesc desc, uint32_t usage);
	HardwareBuffer(uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
	bool lock(uint32_t usage, void **outAddr);
	bool lock(uint32_t usage, WRect rect, void **outAddr);
	void unlock();
	explicit operator bool() const;
	uint32_t pitch();
	AHardwareBuffer *nativeObject();
	EGLClientBuffer eglClientBuffer() const;

private:
	struct HardwareBufferDeleter
	{
		void operator()(AHardwareBuffer *ptr) const
		{
			releaseHardwareBuffer(ptr);
		}
	};
	using UniqueHardwareBuffer = std::unique_ptr<AHardwareBuffer, HardwareBufferDeleter>;

	static void releaseHardwareBuffer(AHardwareBuffer *);

	UniqueHardwareBuffer buff{};
	uint32_t stride{};
};

}
