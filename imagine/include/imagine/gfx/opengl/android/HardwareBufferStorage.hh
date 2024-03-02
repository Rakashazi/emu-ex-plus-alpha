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

#include <imagine/gfx/Texture.hh>
#include <imagine/base/android/HardwareBuffer.hh>
#include <imagine/base/android/GraphicBuffer.hh>
#include "egl.hh"
#include <type_traits>
#include <array>

namespace IG::Gfx
{

template<class Buffer>
class HardwareSingleBufferStorage final: public Texture
{
public:
	HardwareSingleBufferStorage(RendererTask &, TextureConfig config);
	bool setFormat(PixmapDesc, ColorSpace, TextureSamplerConfig);
	LockedTextureBuffer lock(TextureBufferFlags bufferFlags);
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags);

protected:
	Buffer buffer{};
	uint32_t pitchBytes{};
};

template<class Buffer>
class HardwareBufferStorage final: public Texture
{
public:
	HardwareBufferStorage(RendererTask &, TextureConfig config);
	bool setFormat(PixmapDesc, ColorSpace, TextureSamplerConfig);
	LockedTextureBuffer lock(TextureBufferFlags bufferFlags);
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags);

protected:
	struct EGLImageDeleter
	{
		void operator()(EGLImageKHR ptr) const { eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), ptr); }
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

using AHardwareSingleBufferStorage = HardwareSingleBufferStorage<HardwareBuffer>;
using GraphicSingleBufferStorage = HardwareSingleBufferStorage<GraphicBuffer>;
using AHardwareBufferStorage = HardwareBufferStorage<HardwareBuffer>;
using GraphicBufferStorage = HardwareBufferStorage<GraphicBuffer>;

}
