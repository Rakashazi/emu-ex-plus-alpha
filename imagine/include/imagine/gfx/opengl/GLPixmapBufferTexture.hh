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

#include "Texture.hh"
#include <memory>

namespace Gfx
{

class PixmapBufferTexture;

class DirectTextureStorage
{
public:
	struct Buffer
	{
		void *data{};
		uint32_t pitchBytes = 0;

		constexpr Buffer() {}
		constexpr Buffer(void *data, uint32_t pitchBytes):
			data{data}, pitchBytes{pitchBytes}
		{}
	};

	GLenum target{GL_TEXTURE_2D};

	constexpr DirectTextureStorage() {}
	constexpr DirectTextureStorage(GLenum target): target{target} {}
	virtual ~DirectTextureStorage() = 0;
	virtual IG::ErrorCode setFormat(Renderer &r, IG::PixmapDesc desc, GLuint &texNameMember) = 0;
	virtual Buffer lock(Renderer &r) = 0;
	virtual void unlock(Renderer &r) = 0;
};

class GLPixmapBufferTexture
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

	IG::ErrorCode init(PixmapBufferTexture &self, TextureConfig config, TextureBufferMode mode, GLuint &texNameMember, bool singleBuffer);

protected:
	std::unique_ptr<DirectTextureStorage> directTex{};
	std::array<BufferInfo, 2> buffer{};
	GLuint pbo = 0;
	uint8_t bufferIdx{};
	static constexpr uint8_t SINGLE_BUFFER_VALUE = 2;

	IG::ErrorCode initWithPixelBuffer(PixmapBufferTexture &self, TextureConfig config, bool usePBO = false, bool singleBuffer = false);
	IG::ErrorCode initWithHardwareBuffer(PixmapBufferTexture &self, TextureConfig config, bool singleBuffer = false);
	IG::ErrorCode initWithSurfaceTexture(PixmapBufferTexture &self, TextureConfig config, GLuint &texNameMember, bool singleBuffer = false);
	void initPixelBuffer(Renderer &r, IG::PixmapDesc desc, bool usePBO, bool singleBuffer);
	BufferInfo swapBuffer();
	bool isSingleBuffered() const;
	void deinit(Renderer *r);
};

using PixmapBufferTextureImpl = GLPixmapBufferTexture;

}
