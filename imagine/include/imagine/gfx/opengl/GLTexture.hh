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
#include "GLTextureSampler.hh"
#include <imagine/util/typeTraits.hh>
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

class GfxImageSource;

namespace Gfx
{

class Renderer;
class RendererTask;
class RendererCommands;
class TextureSampler;

enum class TextureType : uint8_t
{
	UNSET, T2D_1, T2D_2, T2D_4, T2D_EXTERNAL
};

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
	constexpr GLTexture(RendererTask &rTask):rTask{&rTask} {}
	~GLTexture();
	GLuint texName() const;
	void bindTex(RendererCommands &cmds) const;

protected:
	RendererTask *rTask{};
	TextureRef texName_{};
	IG::PixmapDesc pixDesc{};
	uint8_t levels_{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	TextureType type_ = TextureType::UNSET;
	#else
	static constexpr TextureType type_ = TextureType::T2D_4;
	#endif

	IG::ErrorCode init(RendererTask &r, TextureConfig config);
	TextureConfig baseInit(RendererTask &r, TextureConfig config);
	void deinit();
	bool canUseMipmaps(const Renderer &r) const;
	void updateFormatInfo(IG::PixmapDesc, uint8_t levels, GLenum target = GL_TEXTURE_2D);
	static void setSwizzleForFormatInGL(const Renderer &r, IG::PixelFormatID format, GLuint tex);
	static void setSamplerParamsInGL(const Renderer &r, SamplerParams params, GLenum target = GL_TEXTURE_2D);
	void updateLevelsForMipmapGeneration();
	GLenum target() const;
	#ifdef __ANDROID__
	void initWithEGLImage(EGLImageKHR, IG::PixmapDesc, SamplerParams, bool isMutable);
	void updateWithEGLImage(EGLImageKHR eglImg);
	#endif
};

using TextureImpl = GLTexture;

}
