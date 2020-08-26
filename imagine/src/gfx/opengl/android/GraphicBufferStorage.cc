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

#define LOGTAG "GraphicBuffStorage"
#include "GraphicBufferStorage.hh"
#include "../private.hh"
#include "../utils.h"
#include "../../../base/android/android.hh"
#include <imagine/base/GLContext.hh>

namespace Gfx
{

static constexpr uint32_t allocateUsage = GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE;
static constexpr uint32_t lockUsage = GRALLOC_USAGE_SW_WRITE_OFTEN;

bool GraphicBufferStorage::testPassed_ = false;

GraphicBufferStorage::GraphicBufferStorage() {}

GraphicBufferStorage::GraphicBufferStorage(GraphicBufferStorage &&o)
{
	*this = std::move(o);
}

GraphicBufferStorage &GraphicBufferStorage::operator=(GraphicBufferStorage &&o)
{
	DirectTextureStorage::operator=(o);
	gBuff = std::move(o.gBuff);
	pitchBytes = std::exchange(o.pitchBytes, {});
	return *this;
}

IG::ErrorCode GraphicBufferStorage::setFormat(Renderer &r, IG::PixmapDesc desc, GLuint &tex)
{
	auto androidFormat = Base::toAHardwareBufferFormat(desc.format());
	assert(androidFormat);
	if(unlikely(!gBuff.reallocate(desc.w(), desc.h(), androidFormat, allocateUsage)))
	{
		logMsg("error allocating %dx%d format:%s buffer", desc.w(), desc.h(), desc.format().name());
		return {EINVAL};
	}
	logMsg("allocated buffer:%p size:%dx%d format:%s stride:%d",
		gBuff.handle, desc.w(), desc.h(), desc.format().name(), gBuff.getStride());
	auto dpy = Base::GLDisplay::getDefault().eglDisplay();
	auto eglImg = makeAndroidNativeBufferEGLImage(dpy, (EGLClientBuffer)gBuff.getNativeBuffer());
	if(unlikely(eglImg == EGL_NO_IMAGE_KHR))
	{
		logErr("error creating EGL image");
		return {EINVAL};
	}
	tex = defineEGLImageTexture(r, eglImg, tex);
	eglDestroyImageKHR(dpy, eglImg);
	pitchBytes = gBuff.getStride() * desc.format().bytesPerPixel();
	return {};
}

GraphicBufferStorage::Buffer GraphicBufferStorage::lock(Renderer &)
{
	assert(gBuff.handle);
	Buffer buff{nullptr, pitchBytes};
	if(unlikely(!gBuff.lock(lockUsage, &buff.data)))
	{
		logErr("error locking");
		return {};
	}
	//buff.data = (char*)buff.data + (rect->y * buff.pitchBytes + rect->x * bpp);
	return buff;
}

void GraphicBufferStorage::unlock(Renderer &)
{
	assert(gBuff.handle);
	gBuff.unlock();
}

bool GraphicBufferStorage::canSupport(const char *rendererStr)
{
	auto androidSDK = Base::androidSDK();
	if(androidSDK >= 24)
	{
		// non-NDK library loading is blocked by the OS
		return false;
	}
	else if(androidSDK >= 11)
	{
		// known tested devices with Android 3.0+
		auto buildDevice = Base::androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(androidSDK >= 20 &&
				string_equal(buildDevice.data(), "mako"))
			{
				// only Adreno 320 drivers on the Nexus 4 (mako) are confirmed to work,
				// other devices like the HTC One M7 will crash using GraphicBuffers
				return true;
			}
			if(androidSDK >= 19 &&
				string_equal(buildDevice.data(), "ha3g"))
			{
				// works on Galaxy Note 3 (SM-N900) with Mali-T628
				// but not on all devices with this GPU
				return true;
			}
		}
		else if(Config::MACHINE_IS_GENERIC_X86)
		{
			if(androidSDK >= 19 &&
				string_equal(buildDevice.data(), "ducati2fhd"))
			{
				// Works on Acer Iconia Tab 8 (A1-840FHD)
				return true;
			}
		}
	}
	else
	{
		// general rules for Android 2.3 devices
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(string_equal(rendererStr, "PowerVR SGX 530"))
				return true;
			if(string_equal(rendererStr, "PowerVR SGX 540"))
				return true;
			if(string_equal(rendererStr, "Mali-400 MP"))
				return true;
		}
	}
	return false;
}

bool GraphicBufferStorage::testSupport()
{
	// test API functions work
	Base::GraphicBuffer gb{};
	if(!gb.hasBufferMapper())
	{
		logErr("failed GraphicBuffer mapper initialization");
		return false;
	}
	if(!gb.reallocate(256, 256, HAL_PIXEL_FORMAT_RGB_565, allocateUsage))
	{
		logErr("failed GraphicBuffer allocation test");
		return false;
	}
	void *addr;
	if(!gb.lock(lockUsage, &addr))
	{
		logErr("failed GraphicBuffer lock test");
		return false;
	}
	gb.unlock();
	testPassed_ = true;
	logMsg("Android GraphicBuffer test passed");
	return true;
}

bool GraphicBufferStorage::isSupported()
{
	return testPassed_;
}

}
