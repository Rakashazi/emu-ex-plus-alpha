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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureConfig.hh>
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/util/typeTraits.hh>
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

class GfxImageSource;

namespace Gfx
{

class Renderer;
class RendererCommands;
class TextureSampler;

enum { TEX_UNSET, TEX_2D_1, TEX_2D_2, TEX_2D_4, TEX_2D_EXTERNAL };

class GLTextureSampler
{
public:
	constexpr GLTextureSampler() {}
	GLTextureSampler(Renderer &r, TextureSamplerConfig config);
	~GLTextureSampler();
	void setTexParams(GLenum target) const;
	void deinit();
	GLuint name() const;
	const char *label() const;

protected:
	Renderer *r{};
	GLuint name_ = 0;
	uint16_t minFilter = 0;
	uint16_t magFilter = 0;
	uint16_t xWrapMode_ = 0;
	uint16_t yWrapMode_ = 0;
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, const char *> debugLabel{};
};

using TextureSamplerImpl = GLTextureSampler;

class GLLockedTextureBuffer
{
public:
	constexpr GLLockedTextureBuffer() {}
	constexpr GLLockedTextureBuffer(void *bufferOffset, IG::Pixmap pix, IG::WindowRect srcDirtyRect,
		uint16_t lockedLevel, bool shouldFreeBuffer, GLuint pbo = 0):
		bufferOffset_{bufferOffset}, pix{pix}, srcDirtyRect{srcDirtyRect}, pbo_{pbo},
		lockedLevel{lockedLevel}, shouldFreeBuffer_{shouldFreeBuffer}
	{}
	uint16_t level() const { return lockedLevel; }
	GLuint pbo() const { return pbo_; }
	bool shouldFreeBuffer() const { return shouldFreeBuffer_; }
	void *bufferOffset() const { return bufferOffset_; }

protected:
	void *bufferOffset_{};
	IG::Pixmap pix{};
	IG::WindowRect srcDirtyRect{};
	GLuint pbo_ = 0;
	uint16_t lockedLevel = 0;
	bool shouldFreeBuffer_ = false;
};

using LockedTextureBufferImpl = GLLockedTextureBuffer;

class GLTexture
{
public:
	constexpr GLTexture() {}
	constexpr GLTexture(Renderer &r):r{&r} {}
	~GLTexture();
	GLuint texName() const;
	void bindTex(RendererCommands &cmds, const TextureSampler &sampler) const;

protected:
	Renderer *r{};
	TextureRef texName_ = 0;
	mutable GLuint sampler = 0; // used when separate sampler objects not supported
	IG::PixmapDesc pixDesc;
	uint8_t levels_ = 0;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint8_t type_ = TEX_UNSET;
	#else
	static constexpr uint8_t type_ = TEX_2D_4;
	#endif

	IG::ErrorCode init(Renderer &r, TextureConfig config);
	TextureConfig baseInit(Renderer &r, TextureConfig config);
	void deinit();
	bool canUseMipmaps(const Renderer &r) const;
	void updateFormatInfo(IG::PixmapDesc desc, uint8_t levels, GLenum target = GL_TEXTURE_2D);
	static void setSwizzleForFormatInGLTask(const Renderer &r, IG::PixelFormatID format, GLuint tex);
	void updateLevelsForMipmapGeneration();
	GLenum target() const;
	#ifdef __ANDROID__
	void setFromEGLImage(EGLImageKHR eglImg, IG::PixmapDesc desc);
	#endif
};

using TextureImpl = GLTexture;

}
