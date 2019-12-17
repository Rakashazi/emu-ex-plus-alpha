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
#include <imagine/util/string.h>

namespace Gfx
{

bool GraphicBufferStorage::testPassed = false;

void GraphicBufferStorage::resetImage(EGLDisplay dpy)
{
	if(eglImg != EGL_NO_IMAGE_KHR)
	{
		eglDestroyImageKHR(dpy, eglImg);
		eglImg = EGL_NO_IMAGE_KHR;
	}
}

void GraphicBufferStorage::reset(EGLDisplay dpy)
{
	resetImage(dpy);
	gBuff = {};
}

GraphicBufferStorage::~GraphicBufferStorage()
{
	resetImage(Base::GLDisplay::getDefault().eglDisplay());
}

Error GraphicBufferStorage::setFormat(Renderer &r, IG::PixmapDesc desc, GLuint tex)
{
	auto dpy = Base::GLDisplay::getDefault().eglDisplay();
	reset(dpy);
	logMsg("setting size:%dx%d format:%s", desc.w(), desc.h(), desc.format().name());
	int androidFormat = Base::pixelFormatToDirectAndroidFormat(desc.format());
	if(!androidFormat)
	{
		return std::runtime_error("pixel format not usable");
	}
	if(!gBuff.reallocate(desc.w(), desc.h(), androidFormat,
		GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE))
	{
		return std::runtime_error("allocation failed");
	}
	logMsg("native buffer:%p with stride:%d", gBuff.handle, gBuff.getStride());
	const EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE, EGL_NONE
	};
	eglImg = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		(EGLClientBuffer)gBuff.getNativeBuffer(), eglImgAttrs);
	if(eglImg == EGL_NO_IMAGE_KHR)
	{
		reset(dpy);
		return std::runtime_error("error creating EGL image");
	}
	bool success = false;
	r.runGLTaskSync(
		[this, tex, &success]()
		{
			glBindTexture(GL_TEXTURE_2D, tex);
			success = runGLCheckedAlways(
				[&]()
				{
					glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
				}, "glEGLImageTargetTexture2DOES()");
		});
	if(!success)
	{
		reset(dpy);
		return std::runtime_error("glEGLImageTargetTexture2DOES() failed");
	}
	bpp = desc.format().bytesPerPixel();
	pitch = gBuff.getStride() * desc.format().bytesPerPixel();
	return {};
}

GraphicBufferStorage::Buffer GraphicBufferStorage::lock(Renderer &, IG::WindowRect *dirtyRect)
{
	assert(gBuff.handle);
	Buffer buff{nullptr, pitch};
	bool success;
	if(dirtyRect)
		success = gBuff.lock(GRALLOC_USAGE_SW_WRITE_OFTEN, *dirtyRect, &buff.data);
	else
		success = gBuff.lock(GRALLOC_USAGE_SW_WRITE_OFTEN, &buff.data);
	if(!success)
	{
		logErr("error locking");
		return {};
	}
	if(dirtyRect)
	{
		// adjust pointer by locked region
		buff.data = (char*)buff.data + (dirtyRect->y * buff.pitch + dirtyRect->x * bpp);
	}
	return buff;
}

void GraphicBufferStorage::unlock(Renderer &, GLuint tex)
{
	assert(gBuff.handle);
	gBuff.unlock();
}

bool GraphicBufferStorage::isRendererWhitelisted(const char *rendererStr)
{
	auto androidSDK = Base::androidSDK();
	if(androidSDK >= 14)
	{
		// TODO: Currently prefer SurfaceTexture on all devices that support it
		// until better synchronization is implemented with multi-threaded renderer
		return false;
	}
	else if(androidSDK >= 11)
	{
		// whitelist known tested devices with Android 3.0+
		auto buildDevice = Base::androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(string_equal(buildDevice.data(), "shamu"))
			{
				// works on Nexus 6 on Android 6.0
				return true;
			}
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

}
