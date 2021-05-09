#pragma once

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

#include <imagine/gfx/PixmapBufferTexture.hh>
#include "../../../base/android/HardwareBuffer.hh"
#include "../../../base/android/privateApi/GraphicBuffer.hh"
#include "egl.hh"

namespace Gfx
{

template<class Buffer>
class HardwareSingleBufferStorage final: public TextureBufferStorage
{
public:
	HardwareSingleBufferStorage(RendererTask &, TextureConfig config, IG::ErrorCode *errorPtr);
	IG::ErrorCode setFormat(IG::PixmapDesc desc, ColorSpace colorSpace, const TextureSampler *compatSampler) final;
	LockedTextureBuffer lock(uint32_t bufferFlags) final;
	void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags) final;

protected:
	Buffer buffer{};
	uint32_t pitchBytes{};
};

template<class Buffer>
class HardwareBufferStorage final: public TextureBufferStorage
{
public:
	HardwareBufferStorage(RendererTask &, TextureConfig config, IG::ErrorCode *errorPtr);
	IG::ErrorCode setFormat(IG::PixmapDesc desc, ColorSpace colorSpace, const TextureSampler *compatSampler) final;
	LockedTextureBuffer lock(uint32_t bufferFlags) final;
	void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags) final;

protected:
	struct EGLImageDeleter
	{
		void operator()(EGLImageKHR ptr) const
		{
			if(!ptr)
				return;
			eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), ptr);
		}
	};
	using UniqueEGLImage = std::unique_ptr<std::remove_pointer_t<EGLImageKHR>, EGLImageDeleter>;
	struct BufferInfo
	{
		Buffer buffer{};
		UniqueEGLImage eglImg{};
		uint32_t pitchBytes{};
	};
	std::array<BufferInfo, 2> bufferInfo{};
	uint8_t bufferIdx{};

	void swapBuffer();
};

using AHardwareSingleBufferStorage = HardwareSingleBufferStorage<Base::HardwareBuffer>;
using GraphicSingleBufferStorage = HardwareSingleBufferStorage<Base::GraphicBuffer>;
using AHardwareBufferStorage = HardwareBufferStorage<Base::HardwareBuffer>;
using GraphicBufferStorage = HardwareBufferStorage<Base::GraphicBuffer>;

}
