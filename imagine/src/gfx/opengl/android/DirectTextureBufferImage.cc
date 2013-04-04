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

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE

#include "DirectTextureBufferImage.hh"
#include "../glStateCache.h"
#include "../utils.h"

/*#include <unistd.h>

static void printPrivHnd(buffer_handle_t handle)
{
	private_handle_t* pHnd = (private_handle_t*)handle;
	logMsg("app pid %d version %d numFds %d numInts %d", (int)getpid(), pHnd->version, pHnd->numFds, pHnd->numInts);
	logMsg("fd %d magic %d flags %x size %d offset %d gpu_fd %d base %d lockState %x writeOwner %d gpuaddr %d pid %d",
			pHnd->fd, pHnd->magic, pHnd->flags, pHnd->size, pHnd->offset, pHnd->gpu_fd, pHnd->base, pHnd->lockState, pHnd->writeOwner, pHnd->gpuaddr, pHnd->pid);

}*/

namespace Gfx
{

static void dummyIncRef(struct android_native_base_t* base)
{
	logMsg("called incRef");
}

static void dummyDecRef(struct android_native_base_t* base)
{
	logMsg("called decRef");
}

static void setupAndroidNativeBuffer(android_native_buffer_t &eglBuf, int x, int y, int format, int usage)
{
	mem_zero(eglBuf);
	eglBuf.common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
	eglBuf.common.version = sizeof(android_native_buffer_t);
	//memset(eglBuf.common.reserved, 0, sizeof(eglBuf.common.reserved));
	eglBuf.common.incRef = dummyIncRef;
	eglBuf.common.decRef = dummyDecRef;
	//memset(eglBuf.reserved, 0, sizeof(eglBuf.reserved));
	//memset(eglBuf.reserved_proc, 0, sizeof(eglBuf.reserved_proc));
	eglBuf.width = x;
	eglBuf.height = y;
	//eglBuf.stride = 0;
	eglBuf.format = format;
	//eglBuf.handle = 0;
	eglBuf.usage = usage;
}

bool DirectTextureBufferImage::testSupport(const char **errorStr)
{
	GLuint ref;
	glGenTextures(1, &ref);
	glcBindTexture(GL_TEXTURE_2D, ref);

	Pixmap pix(PixelFormatRGB565);
	pix.init(nullptr, 256, 256);
	DirectTextureBufferImage directTex;
	if(directTex.init(pix, ref, 256, 256, errorStr))
	{
		logMsg("tests passed");
		directTex.deinit();
		return 1;
	}
	else
	{
		glcDeleteTextures(1, &ref);
		return 0;
	}
}

bool DirectTextureBufferImage::initTexture(Pixmap &pix, uint usedX, uint usedY, bool testLock, const char **errorStr)
{
	int androidFormat = pixelFormatToDirectAndroidFormat(pix.format);
	if(androidFormat == GGL_PIXEL_FORMAT_NONE)
	{
		logMsg("format cannot be used");
		if(errorStr) *errorStr = "Unsupported pixel format";
		goto CLEANUP;
	}

	setupAndroidNativeBuffer(eglBuf, usedX, usedY, androidFormat,
		/*GRALLOC_USAGE_SW_READ_OFTEN |*/ GRALLOC_USAGE_SW_WRITE_OFTEN
		/*| GRALLOC_USAGE_HW_RENDER*/ | GRALLOC_USAGE_HW_TEXTURE);
	int err;
	if((err = directTextureConf.allocBuffer(eglBuf)) != 0)
	{
		logMsg("error in alloc buffer: %s", strerror(-err));
		if(errorStr) *errorStr = "Allocation failed";
		goto CLEANUP;
	}
	logMsg("got buffer %p with stride %d", eglBuf.handle, eglBuf.stride);
	//printPrivHnd(eglBuf.handle);

	/*if(grallocMod->registerBuffer(grallocMod, eglBuf.handle) != 0)
	{
		logMsg("error registering");
	}*/
	if(testLock)
	{
		logMsg("testing locking");
		void *data;
		if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, usedX, usedY, data) != 0)
		{
			logMsg("error locking");
			if(errorStr) *errorStr = "Lock failed";
			goto CLEANUP;
		}
		logMsg("data addr %p", data);
		//printPrivHnd(eglBuf.handle);
		//memset(data, 0, eglBuf.stride * eglBuf.height * 2);
		logMsg("unlocking");
		if(directTextureConf.unlockBuffer(eglBuf) != 0)
		{
			logMsg("error unlocking");
			if(errorStr) *errorStr = "Unlock failed";
			goto CLEANUP;
		}
	}
	//printPrivHnd(eglBuf.handle);

	static const EGLint eglImgAttrs[] { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
	eglImg = eglCreateImageKHR(Base::getAndroidEGLDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)&eglBuf, eglImgAttrs);
	if(eglImg == EGL_NO_IMAGE_KHR)
	{
		logMsg("error creating EGL image");
		if(errorStr) *errorStr = "eglCreateImageKHR failed";
		goto CLEANUP;
	}

	handleGLErrors();
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)eglImg);
	if(handleGLErrors([](GLenum, const char *err) { logErr("%s in glEGLImageTargetTexture2DOES", err); }))
	{
		if(errorStr) *errorStr = "glEGLImageTargetTexture2DOES failed";
		goto CLEANUP;
	}

	new(&eglPixmap) Pixmap(pix.format);
	eglPixmap.init(0, usedX, usedY, 0);
	eglPixmap.pitch = eglBuf.stride * eglPixmap.format.bytesPerPixel;

	return 1;

	CLEANUP:

	if(eglImg != EGL_NO_IMAGE_KHR)
	{
		logMsg("calling eglDestroyImageKHR");
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
	}
	if(eglBuf.handle)
	{
		directTextureConf.freeBuffer(eglBuf);
	}
	return 0;
}

bool DirectTextureBufferImage::init(Pixmap &pix, uint texRef, uint usedX, uint usedY, const char **errorStr)
{
	return initTexture(pix, usedX, usedY, !directTextureConf.whitelistedEGLImageKHR, errorStr);
}

/*void DirectTextureBufferImage::replace(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);
	if(eglBuf.handle)
	{
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
		if(directTextureConf.freeBuffer(eglBuf) != 0)
		{
			logWarn("error freeing buffer");
		}
	}
	if(!initTexture(p, usedX, usedY, 0))
	{
		logErr("error redefining EGL texture");
	}
}*/

void DirectTextureBufferImage::write(Pixmap &p, uint hints)
{
	glcBindTexture(GL_TEXTURE_2D, tid);

	//logMsg("updating EGL image");
	void *data;
	Pixmap *texturePix = lock(0, 0, p.x, p.y);
	if(!texturePix)
	{
		return;
	}
	p.copy(0, 0, 0, 0, texturePix, 0, 0);
	unlock();
}

void DirectTextureBufferImage::write(Pixmap &p, uint hints, uint alignment)
{
	write(p, hints);
}

Pixmap *DirectTextureBufferImage::lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback)
{
	void *data;
	if(directTextureConf.lockBuffer(eglBuf, GRALLOC_USAGE_SW_WRITE_OFTEN, x, y, xlen, ylen, data) != 0)
	{
		logMsg("error locking");
		return 0;
	}
	eglPixmap.data = (uchar*)data;
	return &eglPixmap;
}

void DirectTextureBufferImage::unlock(Pixmap *pix, uint hints)
{
	directTextureConf.unlockBuffer(eglBuf);
}

void DirectTextureBufferImage::deinit()
{
	logMsg("freeing EGL image");
	if(eglBuf.handle)
	{
		eglDestroyImageKHR(Base::getAndroidEGLDisplay(), eglImg);
		if(directTextureConf.freeBuffer(eglBuf) != 0)
		{
			logWarn("error freeing buffer");
		}
	}
	TextureBufferImage::deinit();
	freeTexRef(tid);
	tid = 0;
}

}

#endif
