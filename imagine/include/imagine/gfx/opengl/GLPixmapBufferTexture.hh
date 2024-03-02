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

#ifdef __ANDROID__
#include <imagine/gfx/opengl/android/HardwareBufferStorage.hh>
#include <imagine/gfx/opengl/android/SurfaceTextureStorage.hh>
#endif
#include <imagine/gfx/Texture.hh>
#include <memory>
#include <variant>
#include <array>

namespace IG::Gfx
{

class RendererTask;
class PixmapBufferTexture;

template<class Impl, class BufferInfo>
class GLTextureStorage: public Texture
{
public:
	constexpr GLTextureStorage() = default;

	GLTextureStorage(RendererTask &rTask, TextureConfig config, bool singleBuffer):
		Texture{rTask, config},
		bufferIdx{singleBuffer ? SINGLE_BUFFER_VALUE : (int8_t)0} {}

	bool setFormat(PixmapDesc, ColorSpace, TextureSamplerConfig);
	void writeAligned(PixmapView pixmap, int assumeAlign, TextureWriteFlags writeFlags = {});
	LockedTextureBuffer lock(TextureBufferFlags bufferFlags = {});
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags = {});
	bool isSingleBuffered() const { return bufferIdx == SINGLE_BUFFER_VALUE; }

protected:
	int8_t bufferIdx{};
	std::array<BufferInfo, 2> info{};
	static constexpr int8_t SINGLE_BUFFER_VALUE = 2;

	BufferInfo currentBuffer() const
	{
		return isSingleBuffered() ? info[0] : info[bufferIdx];
	}

	void swapBuffer()
	{
		if(isSingleBuffered())
			return;
		bufferIdx = (bufferIdx + 1) % 2;
	}
};

struct GLSystemMemoryBufferInfo
{
	void *data{};

	void *dataStoreOffset() const { return data; }
};

class GLSystemMemoryStorage final: public GLTextureStorage<GLSystemMemoryStorage, GLSystemMemoryBufferInfo>
{
public:
	constexpr GLSystemMemoryStorage() = default;
	GLSystemMemoryStorage(RendererTask &rTask, TextureConfig config, bool singleBuffer);
	void initBuffer(PixmapDesc desc, bool singleBuffer);

private:
	std::unique_ptr<char[]> storage;
};

struct GLPixelBufferInfo
{
	void *data{};
	void *pboDataOffset{};

	void *dataStoreOffset() const { return pboDataOffset; }
};

class GLPixelBufferStorage final: public GLTextureStorage<GLPixelBufferStorage, GLPixelBufferInfo>
{
public:
	constexpr GLPixelBufferStorage() = default;
	GLPixelBufferStorage(RendererTask &rTask, TextureConfig config, bool singleBuffer);
	void initBuffer(PixmapDesc desc, bool singleBuffer);
	GLuint pbo() const { return pixelBuff.get(); }

private:
	UniqueGLBuffer pixelBuff{};
};

using GLPixmapBufferTextureVariant = std::variant<
	GLSystemMemoryStorage,
	GLPixelBufferStorage
	#ifdef __ANDROID__
	, AHardwareSingleBufferStorage
	, GraphicSingleBufferStorage
	, AHardwareBufferStorage
	, GraphicBufferStorage
	#endif
	#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
	, SurfaceTextureStorage
	#endif
	>;

class GLPixmapBufferTexture
{
public:
	constexpr GLPixmapBufferTexture() = default;

protected:
	GLPixmapBufferTextureVariant directTex{};

	void initWithSystemMemory(RendererTask &rTask, TextureConfig config, bool singleBuffer = false);
	void initWithPixelBuffer(RendererTask &rTask, TextureConfig config,  bool singleBuffer = false);
	void initWithHardwareBuffer(RendererTask &rTask, TextureConfig config, bool singleBuffer = false);
	void initWithSurfaceTexture(RendererTask &rTask, TextureConfig config, bool singleBuffer = false);
};

using PixmapBufferTextureImpl = GLPixmapBufferTexture;

}
