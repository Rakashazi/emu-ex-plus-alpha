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

#define LOGTAG "AHardwareBuffStorage"
#include "AHardwareBufferStorage.hh"
#include "../private.hh"
#include "../utils.h"
#include "../../../base/android/android.hh"
#include <imagine/base/GLContext.hh>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/hardware_buffer.h>
#include <dlfcn.h>

namespace Gfx
{

static constexpr uint32_t allocateUsage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
static constexpr uint32_t lockUsage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;

static int (*AHardwareBuffer_allocate)(const AHardwareBuffer_Desc* desc, AHardwareBuffer** outBuffer);
static void (*AHardwareBuffer_release)(AHardwareBuffer* buffer);
static void (*AHardwareBuffer_describe)(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
static int (*AHardwareBuffer_lock)(AHardwareBuffer* buffer, uint64_t usage, int32_t fence, const ARect* rect, void** outVirtualAddress);
static int (*AHardwareBuffer_unlock)(AHardwareBuffer* buffer, int32_t* fence);
static EGLClientBuffer (EGLAPIENTRYP eglGetNativeClientBufferANDROID)(const struct AHardwareBuffer *buffer);

template<class T>
static void dlsymFunc(T &funcPtr, const char *funcName)
{
	funcPtr = (T)dlsym(RTLD_DEFAULT, funcName);
}

AHardwareBufferStorage::AHardwareBufferStorage(Renderer &r, TextureConfig config, IG::ErrorCode *errorPtr):
	TextureBufferStorage{r}
{
	if(!AHardwareBuffer_allocate)
	{
		logMsg("loading AHardwareBuffer functions");
		dlsymFunc(AHardwareBuffer_allocate, "AHardwareBuffer_allocate");
		dlsymFunc(AHardwareBuffer_release, "AHardwareBuffer_release");
		dlsymFunc(AHardwareBuffer_describe, "AHardwareBuffer_describe");
		dlsymFunc(AHardwareBuffer_lock, "AHardwareBuffer_lock");
		dlsymFunc(AHardwareBuffer_unlock, "AHardwareBuffer_unlock");
		dlsymFunc(eglGetNativeClientBufferANDROID, "eglGetNativeClientBufferANDROID");
	}
	config = baseInit(r, config);
	auto err = setFormat(config.pixmapDesc());
	if(unlikely(err && errorPtr))
	{
		*errorPtr = err;
	}
}

AHardwareBufferStorage::AHardwareBufferStorage(AHardwareBufferStorage &&o)
{
	*this = std::move(o);
}

AHardwareBufferStorage::~AHardwareBufferStorage()
{
	deinit();
}

void AHardwareBufferStorage::deinit()
{
	if(!hBuff)
		return;
	logMsg("releasing AHardwareBuffer:%p", hBuff);
	AHardwareBuffer_release(std::exchange(hBuff, {}));
}

AHardwareBufferStorage &AHardwareBufferStorage::operator=(AHardwareBufferStorage &&o)
{
	deinit();
	TextureBufferStorage::operator=(std::move(o));
	hBuff = std::exchange(o.hBuff, {});
	pitchBytes = std::exchange(o.pitchBytes, {});
	return *this;
}

static AHardwareBuffer *makeAHardwareBuffer(IG::PixmapDesc desc, uint32_t usage)
{
	auto androidFormat = Base::toAHardwareBufferFormat(desc.format());
	assert(androidFormat);
	AHardwareBuffer_Desc hardwareDesc{desc.w(), desc.h(), 1, androidFormat, usage};
	AHardwareBuffer *newBuff;
	if(unlikely(AHardwareBuffer_allocate(&hardwareDesc, &newBuff) != 0))
	{
		logErr("Error allocating AHardwareBuffer with format:%s", desc.format().name());
		return nullptr;
	}
	return newBuff;
}

IG::ErrorCode AHardwareBufferStorage::setFormat(IG::PixmapDesc desc)
{
	deinit();
	if(auto newBuff = makeAHardwareBuffer(desc, allocateUsage);
		unlikely(!newBuff))
	{
		logMsg("error allocating %dx%d format:%s buffer", desc.w(), desc.h(), desc.format().name());
		return {EINVAL};
	}
	else
	{
		assert(!hBuff);
		hBuff = newBuff;
	}
	AHardwareBuffer_Desc hardwareDesc;
	AHardwareBuffer_describe(hBuff, &hardwareDesc);
	logMsg("allocated buffer:%p size:%dx%d format:%s stride:%d",
		hBuff, desc.w(), desc.h(), desc.format().name(), hardwareDesc.stride);
	auto dpy = Base::GLDisplay::getDefault().eglDisplay();
	auto eglImg = makeAndroidNativeBufferEGLImage(dpy, eglGetNativeClientBufferANDROID(hBuff));
	if(unlikely(eglImg == EGL_NO_IMAGE_KHR))
	{
		logErr("error creating EGL image");
		return {EINVAL};
	}
	setFromEGLImage(desc.size(), eglImg, desc);
	eglDestroyImageKHR(dpy, eglImg);
	pitchBytes = hardwareDesc.stride * desc.format().bytesPerPixel();
	return {};
}

LockedTextureBuffer AHardwareBufferStorage::lock(uint32_t bufferFlags)
{
	assert(hBuff);
	void *buff{};
	if(unlikely(AHardwareBuffer_lock(hBuff, lockUsage, -1, nullptr, &buff) != 0))
	{
		logErr("error locking");
		return {};
	}
	return makeLockedBuffer(buff, pitchBytes, bufferFlags);
}

void AHardwareBufferStorage::unlock(LockedTextureBuffer, uint32_t)
{
	assert(hBuff);
	AHardwareBuffer_unlock(hBuff, nullptr);
}

}
