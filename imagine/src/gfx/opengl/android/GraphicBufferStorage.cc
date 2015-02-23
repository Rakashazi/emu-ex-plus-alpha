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
#include <imagine/base/GLContext.hh>
#include <imagine/util/ScopeGuard.hh>

namespace Gfx
{

static void setupAndroidNativeBuffer(android_native_buffer_t &eglBuf, int x, int y, int format, int usage)
{
	eglBuf.common.incRef = [](struct android_native_base_t *){ logMsg("called incRef"); };
	eglBuf.common.decRef = [](struct android_native_base_t *){ logMsg("called decRef"); };
	eglBuf.width = x;
	eglBuf.height = y;
	eglBuf.format = format;
	eglBuf.usage = usage;
}

GraphicBufferStorage::~GraphicBufferStorage()
{
	if(eglImg != EGL_NO_IMAGE_KHR)
	{
		eglDestroyImageKHR(Base::GLContext::eglDisplay(), eglImg);
	}
	if(eglBuf.handle)
	{
		logMsg("deinit native buffer:%p", eglBuf.handle);
		if(directTextureConf.freeBuffer(eglBuf) != 0)
		{
			logWarn("error freeing buffer");
		}
	}
}

bool GraphicBufferStorage::testSupport(const char **errorStr)
{
	GLuint ref;
	glGenTextures(1, &ref);
	auto freeTexture = IG::scopeGuard([&](){ glcDeleteTextures(1, &ref); });
	glcBindTexture(GL_TEXTURE_2D, ref);
	IG::PixmapDesc desc{PixelFormatRGB565};
	desc.x = desc.y = 256;
	GraphicBufferStorage directTex;
	if(directTex.init() != OK)
	{
		if(errorStr) *errorStr = "Missing native buffer functions";
		return false;
	}
	if(directTex.setFormat(desc, ref) != OK)
	{
		logMsg("tests passed");
		return true;
	}
	else
	{
		if(errorStr) *errorStr = "Failed native buffer allocation";
		return false;
	}
}

CallResult GraphicBufferStorage::init()
{
	return OK;
}

CallResult GraphicBufferStorage::setFormat(IG::PixmapDesc desc, GLuint tex)
{
	*this = {};
	logMsg("setting size:%dx%d format:%s", desc.x, desc.y, desc.format.name);
	int androidFormat = pixelFormatToDirectAndroidFormat(desc.format);
	if(!androidFormat)
	{
		logErr("pixel format not usable");
		*this = {};
		return INVALID_PARAMETER;
	}
	setupAndroidNativeBuffer(eglBuf, desc.x, desc.y, androidFormat,
		/*GRALLOC_USAGE_SW_READ_OFTEN |*/ GRALLOC_USAGE_SW_WRITE_OFTEN
		/*| GRALLOC_USAGE_HW_RENDER*/ | GRALLOC_USAGE_HW_TEXTURE);
	int err;
	if((err = directTextureConf.allocBuffer(eglBuf)) != 0)
	{
		logErr("alloc buffer failed: %s", strerror(-err));
		*this = {};
		return UNSUPPORTED_OPERATION;
	}
	logMsg("native buffer:%p with stride:%d", eglBuf.handle, eglBuf.stride);
	/*if(grallocMod->registerBuffer(grallocMod, eglBuf.handle) != 0)
	{
		logMsg("error registering");
	}*/
	bool testLock = !directTextureConf.whitelistedEGLImageKHR;
	if(testLock)
	{
		logMsg("testing locking");
		void *data;
		if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, desc.x, desc.y, data) != 0)
		{
			logMsg("error locking");
			*this = {};
			return UNSUPPORTED_OPERATION;
		}
		logMsg("data addr %p", data);
		//memset(data, 0, eglBuf.stride * eglBuf.height * 2);
		logMsg("unlocking");
		if(directTextureConf.unlockBuffer(eglBuf) != 0)
		{
			logErr("error unlocking");
			*this = {};
			return UNSUPPORTED_OPERATION;
		}
	}
	const EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE, EGL_NONE
	};
	eglImg = eglCreateImageKHR(Base::GLContext::eglDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)&eglBuf, eglImgAttrs);
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
	pitch = eglBuf.stride * desc.format.bytesPerPixel;
	return OK;
}

GraphicBufferStorage::Buffer GraphicBufferStorage::lock()
{
	if(unlikely(!eglBuf.handle))
	{
		logErr("called lock when uninitialized");
		return {};
	}
	Buffer buff{nullptr, pitch};
	if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN,
		0, 0, eglBuf.width, eglBuf.height, buff.data) != 0)
	{
		logErr("error locking");
		return {};
	}
	return buff;
}

void GraphicBufferStorage::unlock(GLuint tex)
{
	if(unlikely(!eglBuf.handle))
	{
		logErr("called unlock when uninitialized");
		return;
	}
	directTextureConf.unlockBuffer(eglBuf);
}

}
