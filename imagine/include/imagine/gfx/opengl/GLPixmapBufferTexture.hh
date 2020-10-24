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

#include <imagine/gfx/PixmapTexture.hh>
#include <memory>

namespace Gfx
{

class PixmapBufferTexture;

class TextureBufferStorage : public PixmapTexture
{
public:
	using PixmapTexture::PixmapTexture;
	constexpr TextureBufferStorage() {}
	virtual ~TextureBufferStorage() = 0;
	TextureBufferStorage &operator=(TextureBufferStorage &&o) = default;
	LockedTextureBuffer makeLockedBuffer(void *data, uint32_t pitchBytes, uint32_t bufferFlags);
	virtual IG::ErrorCode setFormat(IG::PixmapDesc desc) = 0;
	virtual void writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags = 0);
	virtual LockedTextureBuffer lock(uint32_t bufferFlags = 0) = 0;
	virtual void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags = 0) = 0;
	bool isExternal() const;

protected:
	using PixmapTexture::setFormat;
	using Texture::clear;
	using Texture::write;
	using Texture::writeAligned;
	using Texture::lock;
	using Texture::generateMipmaps;
};

class GLTextureStorage : public TextureBufferStorage
{
public:
	struct BufferInfo
	{
		void *data{};
		void *bufferOffset{}; // offset into PBO, same as data if no PBO

		constexpr BufferInfo() {}

		constexpr BufferInfo(void *data, void *bufferOffset):
			data{data}, bufferOffset{bufferOffset}
		{}

		constexpr BufferInfo(void *data):
			data{data}, bufferOffset{data}
		{}
	};

	GLTextureStorage(Renderer &r, TextureConfig config, bool usePBO, bool singleBuffer, IG::ErrorCode *errorPtr = {});
	~GLTextureStorage() final;
	GLTextureStorage(GLTextureStorage &&o);
	GLTextureStorage &operator=(GLTextureStorage &&o);
	IG::ErrorCode setFormat(IG::PixmapDesc desc) final;
	void writeAligned(IG::Pixmap pixmap, uint8_t assumeAlign, uint32_t writeFlags = 0) final;
	LockedTextureBuffer lock(uint32_t bufferFlags = 0) final;
	void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags = 0) final;
	bool isSingleBuffered() const;

protected:
	std::array<BufferInfo, 2> buffer{};
	GLuint pbo = 0;
	uint8_t bufferIdx{};
	static constexpr uint8_t SINGLE_BUFFER_VALUE = 2;

	void initPixelBuffer(IG::PixmapDesc desc, bool usePBO, bool singleBuffer);
	BufferInfo swapBuffer();
	void deinit();
};

class GLPixmapBufferTexture
{
public:
	constexpr GLPixmapBufferTexture() {}
	IG::ErrorCode init(Renderer &r, TextureConfig config, TextureBufferMode mode, bool singleBuffer);

protected:
	std::unique_ptr<TextureBufferStorage> directTex{};

	IG::ErrorCode initWithPixelBuffer(Renderer &r, TextureConfig config, bool usePBO = false, bool singleBuffer = false);
	IG::ErrorCode initWithHardwareBuffer(Renderer &r, TextureConfig config, bool singleBuffer = false);
	IG::ErrorCode initWithSurfaceTexture(Renderer &r, TextureConfig config, bool singleBuffer = false);
};

using PixmapBufferTextureImpl = GLPixmapBufferTexture;

}
