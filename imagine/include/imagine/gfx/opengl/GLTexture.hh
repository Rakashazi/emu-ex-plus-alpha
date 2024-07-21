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
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/used.hh>
#include <imagine/util/memory/UniqueResource.hh>
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace IG::Gfx
{

class LockedTextureBuffer;

class GLLockedTextureBuffer
{
public:
	constexpr GLLockedTextureBuffer() = default;
	constexpr GLLockedTextureBuffer(void *bufferOffset, MutablePixmapView pix, WRect srcDirtyRect,
		int lockedLevel, bool shouldFreeBuffer, GLuint pbo = 0):
		bufferOffset_{bufferOffset}, pix{pix},
		lockedLevel{(int8_t)lockedLevel}, shouldFreeBuffer_{shouldFreeBuffer},
		srcDirtyRect{srcDirtyRect}, pbo_{pbo}
	{}
	int level() const { return lockedLevel; }
	GLuint pbo() const { return pbo_; }
	bool shouldFreeBuffer() const { return shouldFreeBuffer_; }
	void *bufferOffset() const { return bufferOffset_; }

protected:
	void *bufferOffset_{};
	MutablePixmapView pix{};
	int8_t lockedLevel{};
	bool shouldFreeBuffer_{};
	WRect srcDirtyRect{};
	GLuint pbo_ = 0;
};

using LockedTextureBufferImpl = GLLockedTextureBuffer;

void destroyGLTextureRef(RendererTask &, TextureRef);

struct GLTextureRefDeleter
{
	RendererTask *rTaskPtr{};

	void operator()(TextureRef s) const
	{
		destroyGLTextureRef(*rTaskPtr, s);
	}
};
using UniqueGLTextureRef = UniqueResource<TextureRef, GLTextureRefDeleter>;

class GLTexture
{
public:
	constexpr GLTexture() = default;
	constexpr GLTexture(RendererTask &rTask):
		texName_{GLTextureRefDeleter{&rTask}} {}
	GLuint texName() const;
	GLenum target() const;
	TextureType type() const { return type_; }
	TextureBinding binding() const { return {texName(), target()}; }

protected:
	UniqueGLTextureRef texName_{};
	PixmapDesc pixDesc{};
	int8_t levels_{};
	TextureType type_{TextureType::UNSET};

	void init(RendererTask &r, TextureConfig config);
	TextureConfig baseInit(RendererTask &r, TextureConfig config);
	bool canUseMipmaps(const Renderer &r) const;
	void updateFormatInfo(PixmapDesc, int8_t levels, GLenum target = GL_TEXTURE_2D);
	static void setSwizzleForFormatInGL(const Renderer &r, PixelFormatId format, GLuint tex);
	static void setSamplerParamsInGL(SamplerParams params, GLenum target = GL_TEXTURE_2D);
	void updateLevelsForMipmapGeneration();
	#ifdef __ANDROID__
	void initWithEGLImage(EGLImageKHR, PixmapDesc, SamplerParams, bool isMutable);
	void updateWithEGLImage(EGLImageKHR eglImg);
	#endif
	LockedTextureBuffer lockedBuffer(void *data, int pitchBytes, TextureBufferFlags bufferFlags);
	RendererTask *taskPtr() const;
	Renderer &renderer() const;
	RendererTask &task() const;
};

using TextureImpl = GLTexture;

}
