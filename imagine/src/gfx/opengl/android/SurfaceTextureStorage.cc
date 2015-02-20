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
		surfaceTextureConf.jSurfaceRelease(env, surface);
		env->DeleteGlobalRef(surface);
	}
	if(surfaceTex)
	{
		surfaceTextureConf.jSurfaceTextureRelease(env, surfaceTex);
		env->DeleteGlobalRef(surfaceTex);
	}
}

CallResult SurfaceTextureStorage::init(GLuint tex)
{
	using namespace Base;
	*this = {};
	logMsg("creating SurfaceTexture with texture:0x%X", tex);
	auto env = jEnv();
	surfaceTex = env->NewObject(surfaceTextureConf.jSurfaceTextureCls, surfaceTextureConf.jSurfaceTexture.m, tex);
	if(!surfaceTex)
	{
		logErr("SurfaceTexture ctor failed");
		*this = {};
		return INVALID_PARAMETER;
	}
	surface = env->NewObject(surfaceTextureConf.jSurfaceCls, surfaceTextureConf.jSurface.m, surfaceTex);
	if(!surface)
	{
		logErr("Surface ctor failed");
		*this = {};
		return INVALID_PARAMETER;
	}
	nativeWin = ANativeWindow_fromSurface(env, surface);
	if(!nativeWin)
	{
		logErr("ANativeWindow_fromSurface failed");
		*this = {};
		return INVALID_PARAMETER;
	}
	logMsg("native window:%p from Surface:%p", nativeWin, surface);
	surfaceTex = env->NewGlobalRef(surfaceTex);
	surface = env->NewGlobalRef(surface);
	return OK;
}

CallResult SurfaceTextureStorage::setFormat(IG::PixmapDesc desc, GLuint tex)
{
	logMsg("setting size:%dx%d format:%s", desc.x, desc.y, desc.format.name);
	int winFormat = pixelFormatToDirectAndroidFormat(desc.format);
	if(!winFormat)
	{
		logErr("pixel format not usable");
		return INVALID_PARAMETER;
	}
	if(ANativeWindow_setBuffersGeometry(nativeWin, desc.x, desc.y, winFormat) < 0)
	{
		logErr("ANativeWindow_setBuffersGeometry failed");
		return INVALID_PARAMETER;
	}
	bpp = desc.format.bytesPerPixel;
	return OK;
}

SurfaceTextureStorage::Buffer SurfaceTextureStorage::lock()
{
	if(unlikely(!nativeWin))
	{
		logErr("called lock when uninitialized");
		return {};
	}
	ANativeWindow_Buffer winBuffer;
	//ARect rect{};
	if(ANativeWindow_lock(nativeWin, &winBuffer, 0/*&rect*/) < 0)
	{
		logErr("ANativeWindow_lock failed");
		return {};
	}
	Buffer buff{winBuffer.bits, winBuffer.stride * bpp};
	//logMsg("locked buffer %p with pitch %d", buff.data, buff.pitch);
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
	surfaceTextureConf.jUpdateTexImage(jEnv(), surfaceTex);
	// texture implicitly bound in updateTexImage()
	glState.bindTextureState.GL_TEXTURE_EXTERNAL_OES_state = tex;
}

}
