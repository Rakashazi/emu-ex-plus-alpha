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
#include "../GLStateCache.hh"
#include "../private.hh"
#include "../utils.h"
#include "../../../base/android/android.hh"
#include <imagine/base/GLContext.hh>
#include <imagine/util/ScopeGuard.hh>

namespace Gfx
{

GraphicBufferStorage::~GraphicBufferStorage()
{
	if(eglImg != EGL_NO_IMAGE_KHR)
	{
		eglDestroyImageKHR(Base::GLContext::eglDisplay(), eglImg);
	}
}

CallResult GraphicBufferStorage::init()
{
	return OK;
}

CallResult GraphicBufferStorage::setFormat(IG::PixmapDesc desc, GLuint tex)
{
	*this = {};
	logMsg("setting size:%dx%d format:%s", desc.w(), desc.h(), desc.format().name());
	int androidFormat = Base::pixelFormatToDirectAndroidFormat(desc.format());
	if(!androidFormat)
	{
		logErr("pixel format not usable");
		return INVALID_PARAMETER;
	}
	if(!gBuff.reallocate(desc.w(), desc.h(), androidFormat,
		GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE))
	{
		logErr("allocation failed");
		return UNSUPPORTED_OPERATION;
	}
	logMsg("native buffer:%p with stride:%d", gBuff.handle, gBuff.getStride());
	const EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE, EGL_NONE
	};
	eglImg = eglCreateImageKHR(Base::GLContext::eglDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		(EGLClientBuffer)gBuff.getNativeBuffer(), eglImgAttrs);
	if(eglImg == EGL_NO_IMAGE_KHR)
	{
		logErr("error creating EGL image");
		*this = {};
		return UNSUPPORTED_OPERATION;
	}
	glcBindTexture(GL_TEXTURE_2D, tex);
	handleGLErrors();
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
	if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glEGLImageTargetTexture2DOES", err); }))
	{
		*this = {};
		return UNSUPPORTED_OPERATION;
	}
	bpp = desc.format().bytesPerPixel();
	pitch = gBuff.getStride() * desc.format().bytesPerPixel();
	return OK;
}

GraphicBufferStorage::Buffer GraphicBufferStorage::lock(IG::WindowRect *dirtyRect)
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

void GraphicBufferStorage::unlock(GLuint tex)
{
	assert(gBuff.handle);
	gBuff.unlock();
}

bool GraphicBufferStorage::isRendererWhitelisted(const char *rendererStr)
{
	if(Config::MACHINE_IS_GENERIC_ARMV7)
	{
		if(string_equal(rendererStr, "PowerVR SGX 530"))
			return true;
		if(string_equal(rendererStr, "PowerVR SGX 540"))
			return true;
		if(strstr(rendererStr, "Mali"))
			return true;
		if(Base::androidSDK() >= 20 &&
			string_equal(rendererStr, "Adreno (TM) 320"))
			return true;
	}
	else if(Config::MACHINE_IS_GENERIC_X86)
	{
		if(strstr(rendererStr, "BayTrail"))
			return true;
	}
	return false;
}

}
