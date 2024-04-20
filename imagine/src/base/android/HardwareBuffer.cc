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

#define LOGTAG "HardwareBuff"
#include <imagine/base/android/HardwareBuffer.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/pixmap/PixmapDesc.hh>
#include "android.hh"
#include <imagine/logger/logger.h>
#include <android/hardware_buffer.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace IG
{

static int (*AHardwareBuffer_allocate)(const AHardwareBuffer_Desc* desc, AHardwareBuffer** outBuffer){};
static void (*AHardwareBuffer_release)(AHardwareBuffer* buffer){};
static void (*AHardwareBuffer_describe)(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc){};
static int (*AHardwareBuffer_lock)(AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const ARect* rect, void** outVirtualAddress){};
static int (*AHardwareBuffer_unlock)(AHardwareBuffer* buffer, int32_t* fence){};
static EGLClientBuffer (EGLAPIENTRYP eglGetNativeClientBufferANDROID)(const struct AHardwareBuffer *buffer){};

static void loadAHardwareBufferSymbols()
{
	if(!AHardwareBuffer_allocate) [[unlikely]]
	{
		logMsg("loading AHardwareBuffer functions");
		loadSymbol(AHardwareBuffer_allocate, {}, "AHardwareBuffer_allocate");
		loadSymbol(AHardwareBuffer_release, {}, "AHardwareBuffer_release");
		loadSymbol(AHardwareBuffer_describe, {}, "AHardwareBuffer_describe");
		loadSymbol(AHardwareBuffer_lock, {}, "AHardwareBuffer_lock");
		loadSymbol(AHardwareBuffer_unlock, {}, "AHardwareBuffer_unlock");
		loadSymbol(eglGetNativeClientBufferANDROID, {}, "eglGetNativeClientBufferANDROID");
	}
}

HardwareBuffer::HardwareBuffer()
{
	loadAHardwareBufferSymbols();
}

HardwareBuffer::HardwareBuffer(PixmapDesc desc, uint32_t usage):
	HardwareBuffer(desc.w(), desc.h(), toAHardwareBufferFormat(desc.format), usage)
{}

HardwareBuffer::HardwareBuffer(uint32_t w, uint32_t h, uint32_t format, uint32_t usage):
	HardwareBuffer()
{
	assumeExpr(AHardwareBuffer_allocate);
	AHardwareBuffer_Desc hardwareDesc{.width = w, .height = h, .layers = 1, .format = format, .usage = usage,
		.stride{}, .rfu0{}, .rfu1{}};
	AHardwareBuffer *newBuff;
	if(AHardwareBuffer_allocate(&hardwareDesc, &newBuff) != 0) [[unlikely]]
	{
		logErr("error allocating AHardwareBuffer");
		return;
	}
	AHardwareBuffer_Desc desc;
	AHardwareBuffer_describe(newBuff, &desc);
	stride = desc.stride;
	buff.reset(newBuff);
}

bool HardwareBuffer::lock(uint32_t usage, void **vaddr)
{
	if(AHardwareBuffer_lock(buff.get(), usage, -1, nullptr, vaddr) != 0) [[unlikely]]
	{
		logErr("error locking");
		return false;
	}
	return true;
}

bool HardwareBuffer::lock(uint32_t usage, IG::WindowRect rect, void **vaddr)
{
	if(Config::DEBUG_BUILD)
	{
		AHardwareBuffer_Desc desc;
		AHardwareBuffer_describe(buff.get(), &desc);
		if((rect.x < 0 || rect.x2 > (int)desc.width ||
			rect.y < 0 || rect.y2 > (int)desc.height)) [[unlikely]]
		{
			bug_unreachable("locking pixels:[%d:%d:%d:%d] outside of buffer:%d,%d",
				rect.x, rect.y, rect.x2, rect.y2, desc.width, desc.height);
		}
	}
	ARect aRect{.left = rect.x, .top = rect.y, .right = rect.x2, .bottom = rect.y2};
	if(AHardwareBuffer_lock(buff.get(), usage, -1, &aRect, vaddr) != 0) [[unlikely]]
	{
		logErr("error locking");
		return false;
	}
	return true;
}

void HardwareBuffer::unlock()
{
	AHardwareBuffer_unlock(buff.get(), nullptr);
}

HardwareBuffer::operator bool() const
{
	return (bool)buff;
}

uint32_t HardwareBuffer::pitch()
{
	return stride;
}

AHardwareBuffer *HardwareBuffer::nativeObject()
{
	return buff.get();
}

EGLClientBuffer HardwareBuffer::eglClientBuffer() const
{
	return eglGetNativeClientBufferANDROID(buff.get());
}

void HardwareBuffer::releaseHardwareBuffer(AHardwareBuffer *buff)
{
	if(!buff)
		return;
	AHardwareBuffer_release(buff);
}

}
