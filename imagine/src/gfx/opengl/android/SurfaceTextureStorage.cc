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
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Renderer.hh>
#include "../../../base/android/android.hh"
#include <imagine/util/ScopeGuard.hh>
#include <imagine/logger/logger.h>
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
			if(err && errorPtr) [[unlikely]]
			{
				*errorPtr = err;
			}
		});
	config = baseInit(r, config);
	if(!renderer().support.hasExternalEGLImages) [[unlikely]]
	{
		logErr("can't init without OES_EGL_image_external extension");
		err = {ENOTSUP};
		return;
	}
	SamplerParams samplerParams = config.compatSampler() ? config.compatSampler()->samplerParams() : SamplerParams{};
	task().runSync(
		[=, this](GLTask::TaskContext ctx)
		{
			auto env = task().appContext().thisThreadJniEnv();
			glGenTextures(1, &texName_);
			auto surfaceTex = makeSurfaceTexture(renderer().appContext(), env, texName_, makeSingleBuffered);
			singleBuffered = makeSingleBuffered;
			if(!surfaceTex && makeSingleBuffered)
			{
				// fall back to buffered mode
				surfaceTex = makeSurfaceTexture(renderer().appContext(), env, texName_, false);
				singleBuffered = false;
			}
			if(!surfaceTex) [[unlikely]]
				return;
			updateSurfaceTextureImage(env, surfaceTex); // set the initial display & context
			this->surfaceTex = env->NewGlobalRef(surfaceTex);
			ctx.notifySemaphore();
			setSamplerParamsInGL(renderer(), samplerParams, GL_TEXTURE_EXTERNAL_OES);
		});
	if(!surfaceTex) [[unlikely]]
	{
		logErr("SurfaceTexture ctor failed");
		err = {EINVAL};
		return;
	}
	logMsg("made%sSurfaceTexture with texture:0x%X",
		singleBuffered ? " " : " buffered ", texName_);
	auto env = r.appContext().mainThreadJniEnv();
	auto localSurface = makeSurface(env, surfaceTex);
	if(!localSurface) [[unlikely]]
	{
		logErr("Surface ctor failed");
		err = {EINVAL};
		deinit();
		return;
	}
	surface = env->NewGlobalRef(localSurface);
	nativeWin = ANativeWindow_fromSurface(env, localSurface);
	if(!nativeWin) [[unlikely]]
	{
		logErr("ANativeWindow_fromSurface failed");
		err = {EINVAL};
		deinit();
		return;
	}
	logMsg("native window:%p from Surface:%p%s", nativeWin, localSurface, singleBuffered ? " (single-buffered)" : "");
	err = setFormat(config.pixmapDesc(), config.colorSpace(), config.compatSampler());
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
		ANativeWindow_release(std::exchange(nativeWin, {}));
	}
	auto env = task().appContext().mainThreadJniEnv();
	if(surface)
	{
		releaseSurface(env, surface);
		env->DeleteGlobalRef(std::exchange(surface, {}));
	}
	if(surfaceTex)
	{
		releaseSurfaceTexture(env, surfaceTex);
		env->DeleteGlobalRef(std::exchange(surfaceTex, {}));
	}
}

IG::ErrorCode SurfaceTextureStorage::setFormat(IG::PixmapDesc desc, ColorSpace colorSpace, const TextureSampler *)
{
	logMsg("setting size:%dx%d format:%s", desc.w(), desc.h(), desc.format().name());
	int winFormat = Base::toAHardwareBufferFormat(desc.format());
	if(!winFormat) [[unlikely]]
	{
		logErr("pixel format not usable");
		return {EINVAL};
	}
	if(ANativeWindow_setBuffersGeometry(nativeWin, desc.w(), desc.h(), winFormat) < 0) [[unlikely]]
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
	if(!nativeWin) [[unlikely]]
	{
		logErr("called lock when uninitialized");
		return {};
	}
	if(singleBuffered)
	{
		task().runSync(
			[tex = surfaceTex, app = task().appContext()]()
			{
				releaseSurfaceTextureImage(app.thisThreadJniEnv(), tex);
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
	if(!nativeWin) [[unlikely]]
	{
		logErr("called unlock when uninitialized");
		return;
	}
	ANativeWindow_unlockAndPost(nativeWin);
	task().run(
		[tex = surfaceTex, app = task().appContext()]()
		{
			Base::updateSurfaceTextureImage(app.thisThreadJniEnv(), tex);
		});
}

void SurfaceTextureStorage::setCompatTextureSampler(const TextureSampler &compatSampler)
{
	if(renderer().support.hasSamplerObjects)
		return;
	task().run(
		[&r = std::as_const(renderer()), texName = texName_, params = compatSampler.samplerParams()]()
		{
			GLTextureSampler::setTexParamsInGL(texName, GL_TEXTURE_EXTERNAL_OES, params);
		});
}

}
