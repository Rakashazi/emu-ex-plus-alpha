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

#define LOGTAG "SurfaceTexStorage"
#include "SurfaceTextureStorage.hh"
#include "../GLStateCache.hh"
#include "../private.hh"
#include "../../../base/android/android.hh"

namespace Gfx
{

static void updateTexImage(JNIEnv *env, jobject surfaceTex, GLuint tex)
{
	Base::updateSurfaceTextureImage(env, surfaceTex);
	// texture implicitly bound in updateTexImage()
	glState.bindTextureState.GL_TEXTURE_EXTERNAL_OES_state = tex;
}

SurfaceTextureStorage::~SurfaceTextureStorage()
{
	using namespace Base;
	if(nativeWin)
	{
		logMsg("deinit SurfaceTexture, releasing window:%p", nativeWin);
		ANativeWindow_release(nativeWin);
	}
	auto env = jEnv();
	if(surface)
	{
		releaseSurface(env, surface);
		env->DeleteGlobalRef(surface);
	}
	if(surfaceTex)
	{
		releaseSurfaceTexture(env, surfaceTex);
		env->DeleteGlobalRef(surfaceTex);
	}
}

CallResult SurfaceTextureStorage::init(GLuint tex)
{
	using namespace Base;
	if(!useExternalEGLImages)
	{
		logMsg("can't init without OES_EGL_image_external extension");
		return UNSUPPORTED_OPERATION;
	}
	*this = {};
	auto env = jEnv();
	auto localSurfaceTex = makeSurfaceTexture(env, tex, true);
	if(!localSurfaceTex)
	{
		// fallback to buffered mode
		localSurfaceTex = makeSurfaceTexture(env, tex);
		singleBuffered = false;
	}
	else
	{
		updateTexImage(env, localSurfaceTex, tex);
		singleBuffered = true;
	}
	if(!localSurfaceTex)
	{
		logErr("SurfaceTexture ctor failed with texture:0x%X", tex);
		*this = {};
		return INVALID_PARAMETER;
	}
	logMsg("created%sSurfaceTexture with texture:0x%X",
		singleBuffered ? " " : " buffered ", tex);
	auto localSurface = makeSurface(env, localSurfaceTex);
	if(!localSurface)
	{
		logErr("Surface ctor failed");
		*this = {};
		return INVALID_PARAMETER;
	}
	nativeWin = ANativeWindow_fromSurface(env, localSurface);
	if(!nativeWin)
	{
		logErr("ANativeWindow_fromSurface failed");
		*this = {};
		return INVALID_PARAMETER;
	}
	logMsg("native window:%p from Surface:%p", nativeWin, localSurface);
	surfaceTex = env->NewGlobalRef(localSurfaceTex);
	surface = env->NewGlobalRef(localSurface);
	return OK;
}

CallResult SurfaceTextureStorage::setFormat(IG::PixmapDesc desc, GLuint tex)
{
	logMsg("setting size:%dx%d format:%s", desc.w(), desc.h(), desc.format().name());
	int winFormat = Base::pixelFormatToDirectAndroidFormat(desc.format());
	if(!winFormat)
	{
		logErr("pixel format not usable");
		return INVALID_PARAMETER;
	}
	if(ANativeWindow_setBuffersGeometry(nativeWin, desc.w(), desc.h(), winFormat) < 0)
	{
		logErr("ANativeWindow_setBuffersGeometry failed");
		return INVALID_PARAMETER;
	}
	bpp = desc.format().bytesPerPixel();
	return OK;
}

SurfaceTextureStorage::Buffer SurfaceTextureStorage::lock(IG::WindowRect *dirtyRect)
{
	using namespace Base;
	if(unlikely(!nativeWin))
	{
		logErr("called lock when uninitialized");
		return {};
	}
	if(singleBuffered)
	{
		releaseSurfaceTextureImage(jEnv(), surfaceTex);
	}
	ANativeWindow_Buffer winBuffer;
	ARect aRect;
	if(dirtyRect)
	{
		aRect.left = dirtyRect->x;
		aRect.top = dirtyRect->y;
		aRect.right = dirtyRect->x2;
		aRect.bottom = dirtyRect->y2;
	}
	if(ANativeWindow_lock(nativeWin, &winBuffer, dirtyRect ? &aRect : nullptr) < 0)
	{
		logErr("ANativeWindow_lock failed");
		return {};
	}
	Buffer buff{winBuffer.bits, winBuffer.stride * bpp};
	//logMsg("locked buffer %p with pitch %d", buff.data, buff.pitch);
	if(dirtyRect)
	{
		// update dirty rectangle & adjust pointer by locked region
		dirtyRect->x = aRect.left;
		dirtyRect->y = aRect.top;
		dirtyRect->x2 = aRect.right;
		dirtyRect->y2 = aRect.bottom;
		buff.data = (char*)buff.data + (aRect.top * buff.pitch + aRect.left * bpp);
	}
	return buff;
}

void SurfaceTextureStorage::unlock(GLuint tex)
{
	using namespace Base;
	if(unlikely(!nativeWin))
	{
		logErr("called unlock when uninitialized");
		return;
	}
	ANativeWindow_unlockAndPost(nativeWin);
	updateTexImage(jEnv(), surfaceTex, tex);
}

bool SurfaceTextureStorage::isRendererBlacklisted(const char *rendererStr)
{
	return false;
	// TODO: these need to be tested on an ES 2.0 context,
	// so assume nothing is blacklisted for now
	if(strstr(rendererStr, "GC1000")) // Texture binding issues on Samsung Galaxy Tab 3 7.0 on Android 4.1
	{
		logWarn("buggy SurfaceTexture implementation on Vivante GC1000, blacklisted");
		return true;
	}
	else if(strstr(rendererStr, "Adreno"))
	{
		if(strstr(rendererStr, "200")) // Textures may stop updating on HTC EVO 4G (supersonic) on Android 4.1
		{
			logWarn("buggy SurfaceTexture implementation on Adreno 200, blacklisted");
			return true;
		}
	}
	return false;
}


}
