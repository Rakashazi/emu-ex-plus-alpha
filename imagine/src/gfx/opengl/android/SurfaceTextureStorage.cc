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
#include <imagine/gfx/RendererTask.hh>
#include "SurfaceTextureStorage.hh"
#include "../private.hh"
#include "../../../base/android/android.hh"
#include <imagine/util/ScopeGuard.hh>
#include <android/native_window_jni.h>

namespace Gfx
{

SurfaceTextureStorage::SurfaceTextureStorage(RendererTask &r, TextureConfig config, bool makeSingleBuffered, IG::ErrorCode *errorPtr):
	TextureBufferStorage{r}
{
	using namespace Base;
	IG::ErrorCode err{};
	auto setErrorPtr = IG::scopeGuard(
		[&]()
		{
			if(unlikely(err && errorPtr))
			{
				*errorPtr = err;
			}
		});
	config = baseInit(r, config);
	if(unlikely(!renderer().support.hasExternalEGLImages))
	{
		logErr("can't init without OES_EGL_image_external extension");
		err = {ENOTSUP};
		return;
	}
	task().runSync(
		[=, this]()
		{
			auto env = jEnvForThread();
			glGenTextures(1, &texName_);
			auto surfaceTex = makeSurfaceTexture(jEnvForThread(), texName_, makeSingleBuffered);
			singleBuffered = makeSingleBuffered;
			if(!surfaceTex && makeSingleBuffered)
			{
				// fall back to buffered mode
				surfaceTex = makeSurfaceTexture(jEnvForThread(), texName_, false);
				singleBuffered = false;
			}
			if(unlikely(!surfaceTex))
				return;
			updateSurfaceTextureImage(env, surfaceTex); // set the initial display & context
			this->surfaceTex = env->NewGlobalRef(surfaceTex);
		});
	if(unlikely(!surfaceTex))
	{
		logErr("SurfaceTexture ctor failed");
		err = {EINVAL};
		return;
	}
	logMsg("made%sSurfaceTexture with texture:0x%X",
		singleBuffered ? " " : " buffered ", texName_);
	auto env = jEnvForThread();
	auto localSurface = makeSurface(env, surfaceTex);
	if(unlikely(!localSurface))
	{
		logErr("Surface ctor failed");
		err = {EINVAL};
		deinit();
		return;
	}
	surface = env->NewGlobalRef(localSurface);
	nativeWin = ANativeWindow_fromSurface(env, localSurface);
	if(unlikely(!nativeWin))
	{
		logErr("ANativeWindow_fromSurface failed");
		err = {EINVAL};
		deinit();
		return;
	}
	logMsg("native window:%p from Surface:%p%s", nativeWin, localSurface, singleBuffered ? " (single-buffered)" : "");
	err = setFormat(config.pixmapDesc());
}

SurfaceTextureStorage::SurfaceTextureStorage(SurfaceTextureStorage &&o)
{
	*this = std::move(o);
}

SurfaceTextureStorage &SurfaceTextureStorage::operator=(SurfaceTextureStorage &&o)
{
	deinit();
	TextureBufferStorage::operator=(std::move(o));
	surfaceTex = std::exchange(o.surfaceTex, {});
	surface = std::exchange(o.surface, {});
	nativeWin = std::exchange(o.nativeWin, {});
	bpp = o.bpp;
	singleBuffered = o.singleBuffered;
	return *this;
}

SurfaceTextureStorage::~SurfaceTextureStorage()
{
	deinit();
}

void SurfaceTextureStorage::deinit()
{
	using namespace Base;
	if(nativeWin)
	{
		logMsg("deinit SurfaceTexture, releasing window:%p", nativeWin);
		ANativeWindow_release(nativeWin);
		nativeWin = {};
	}
	auto env = jEnvForThread();
	if(surface)
	{
		releaseSurface(env, surface);
		env->DeleteGlobalRef(surface);
		surface = {};
	}
	if(surfaceTex)
	{
		releaseSurfaceTexture(env, surfaceTex);
		env->DeleteGlobalRef(surfaceTex);
		surfaceTex = {};
	}
}

IG::ErrorCode SurfaceTextureStorage::setFormat(IG::PixmapDesc desc)
{
	logMsg("setting size:%dx%d format:%s", desc.w(), desc.h(), desc.format().name());
	int winFormat = Base::toAHardwareBufferFormat(desc.format());
	if(unlikely(!winFormat))
	{
		logErr("pixel format not usable");
		return {EINVAL};
	}
	if(unlikely(ANativeWindow_setBuffersGeometry(nativeWin, desc.w(), desc.h(), winFormat) < 0))
	{
		logErr("ANativeWindow_setBuffersGeometry failed");
		return {EINVAL};
	}
	updateFormatInfo(desc.size(), desc, 1, GL_TEXTURE_EXTERNAL_OES);
	bpp = desc.format().bytesPerPixel();
	return {};
}

LockedTextureBuffer SurfaceTextureStorage::lock(uint32_t bufferFlags)
{
	using namespace Base;
	if(unlikely(!nativeWin))
	{
		logErr("called lock when uninitialized");
		return {};
	}
	if(singleBuffered)
	{
		task().runSync(
			[tex = surfaceTex]()
			{
				releaseSurfaceTextureImage(jEnvForThread(), tex);
			});
	}
	ANativeWindow_Buffer winBuffer;
	// setup the dirty rectangle, not currently needed by our use case
	/*ARect aRect;
	aRect.left = rect.x;
	aRect.top = rect.y;
	aRect.right = rect.x2;
	aRect.bottom = rect.y2;*/
	if(ANativeWindow_lock(nativeWin, &winBuffer, nullptr) < 0)
	{
		logErr("ANativeWindow_lock failed");
		return {};
	}
	/*rect.x = aRect.left;
	rect.y = aRect.top;
	rect.x2 = aRect.right;
	rect.y2 = aRect.bottom;*/
	//buff.data = (char*)winBuffer.bits + (aRect.top * buff.pitch + aRect.left * bpp);
	//logMsg("locked buffer %p with pitch %d", winBuffer.bits, winBuffer.stride * bpp);
	return makeLockedBuffer(winBuffer.bits, (uint32_t)winBuffer.stride * bpp, bufferFlags);
}

void SurfaceTextureStorage::unlock(LockedTextureBuffer, uint32_t)
{
	using namespace Base;
	if(unlikely(!nativeWin))
	{
		logErr("called unlock when uninitialized");
		return;
	}
	ANativeWindow_unlockAndPost(nativeWin);
	task().run(
		[tex = surfaceTex]()
		{
			Base::updateSurfaceTextureImage(Base::jEnvForThread(), tex);
		});
}

}
