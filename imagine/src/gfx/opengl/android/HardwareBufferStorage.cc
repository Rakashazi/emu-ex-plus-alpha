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

#define LOGTAG "HardwareBuffStorage"
#include "HardwareBufferStorage.hh"
#include "egl.hh"
#include <imagine/gfx/Renderer.hh>
#include <imagine/logger/logger.h>
#include <android/hardware_buffer.h>

namespace Gfx
{

static constexpr uint32_t allocateUsage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
static constexpr uint32_t lockUsage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;

template<class Buffer>
HardwareSingleBufferStorage<Buffer>::HardwareSingleBufferStorage(RendererTask &r, TextureConfig config, IG::ErrorCode *errorPtr):
	TextureBufferStorage{r}
{
	config = baseInit(r, config);
	auto err = setFormat(config.pixmapDesc(), config.colorSpace(), config.compatSampler());
	if(err && errorPtr) [[unlikely]]
	{
		*errorPtr = err;
	}
}

template<class Buffer>
IG::ErrorCode HardwareSingleBufferStorage<Buffer>::setFormat(IG::PixmapDesc desc, ColorSpace colorSpace, const TextureSampler *compatSampler)
{
	buffer = {desc, allocateUsage};
	if(!buffer)
	{
		logMsg("error allocating %dx%d format:%s buffer", desc.w(), desc.h(), desc.format().name());
		return {EINVAL};
	}
	logMsg("allocated buffer:%p size:%dx%d format:%s pitch:%d",
		buffer.nativeObject(), desc.w(), desc.h(), desc.format().name(), buffer.pitch());
	pitchBytes = buffer.pitch() * desc.format().bytesPerPixel();
	auto dpy = renderer().glDisplay();
	auto eglImg = makeAndroidNativeBufferEGLImage(dpy, buffer.eglClientBuffer(), colorSpace == ColorSpace::SRGB);
	if(!eglImg) [[unlikely]]
	{
		logErr("error creating EGL image");
		return {EINVAL};
	}
	initWithEGLImage(desc.size(), eglImg, desc, compatSampler ? compatSampler->samplerParams() : SamplerParams{}, false);
	eglDestroyImageKHR(dpy, eglImg);
	return {};
}

template<class Buffer>
LockedTextureBuffer HardwareSingleBufferStorage<Buffer>::lock(uint32_t bufferFlags)
{
	void *data{};
	if(!buffer.lock(lockUsage, &data)) [[unlikely]]
	{
		logErr("error locking");
		return {};
	}
	return makeLockedBuffer(data, pitchBytes, bufferFlags);
}

template<class Buffer>
void HardwareSingleBufferStorage<Buffer>::unlock(LockedTextureBuffer, uint32_t)
{
	buffer.unlock();
}

template<class Buffer>
HardwareBufferStorage<Buffer>::HardwareBufferStorage(RendererTask &r, TextureConfig config, IG::ErrorCode *errorPtr):
	TextureBufferStorage{r}
{
	config = baseInit(r, config);
	auto err = setFormat(config.pixmapDesc(), config.colorSpace(), config.compatSampler());
	if(err && errorPtr) [[unlikely]]
	{
		*errorPtr = err;
	}
}

template<class Buffer>
IG::ErrorCode HardwareBufferStorage<Buffer>::setFormat(IG::PixmapDesc desc, ColorSpace colorSpace, const TextureSampler *compatSampler)
{
	auto dpy = renderer().glDisplay();
	for(auto &[buff, eglImg, pitchBytes] : bufferInfo)
	{
		buff = {desc, allocateUsage};
		if(!buff)
		{
			logMsg("error allocating %dx%d format:%s buffer", desc.w(), desc.h(), desc.format().name());
			return {EINVAL};
		}
		logMsg("allocated buffer:%p size:%dx%d format:%s pitch:%d",
			buff.nativeObject(), desc.w(), desc.h(), desc.format().name(), buff.pitch());
		pitchBytes = buff.pitch() * desc.format().bytesPerPixel();
		eglImg.reset(makeAndroidNativeBufferEGLImage(dpy, buff.eglClientBuffer(), colorSpace == ColorSpace::SRGB));
		if(!eglImg) [[unlikely]]
		{
			logErr("error creating EGL image");
			return {EINVAL};
		}
	}
	initWithEGLImage(desc.size(), {}, desc, compatSampler ? compatSampler->samplerParams() : SamplerParams{}, true);
	return {};
}

template<class Buffer>
LockedTextureBuffer HardwareBufferStorage<Buffer>::lock(uint32_t bufferFlags)
{
	void *data{};
	auto &[buff, eglImg, pitchBytes] = bufferInfo[bufferIdx];
	if(!buff.lock(lockUsage, &data)) [[unlikely]]
	{
		logErr("error locking");
		return {};
	}
	return makeLockedBuffer(data, pitchBytes, bufferFlags);
}

template<class Buffer>
void HardwareBufferStorage<Buffer>::unlock(LockedTextureBuffer, uint32_t)
{
	bufferInfo[bufferIdx].buffer.unlock();
	swapBuffer();
}

template<class Buffer>
void HardwareBufferStorage<Buffer>::swapBuffer()
{
	updateWithEGLImage(bufferInfo[bufferIdx].eglImg.get());
	bufferIdx = (bufferIdx + 1) % 2;
}

template class HardwareSingleBufferStorage<Base::HardwareBuffer>;
template class HardwareSingleBufferStorage<Base::GraphicBuffer>;
template class HardwareBufferStorage<Base::HardwareBuffer>;
template class HardwareBufferStorage<Base::GraphicBuffer>;

}
