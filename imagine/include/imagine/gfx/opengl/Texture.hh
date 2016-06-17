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

namespace Gfx
{

class GLTextureSampler
{
protected:
	GLuint name_ = 0;
	GLenum minFilter = 0;
	GLenum magFilter = 0;
	GLenum xWrapMode_ = 0;
	GLenum yWrapMode_ = 0;

public:
	constexpr GLTextureSampler() {}
	void setTexParams(GLenum target);
	GLuint name() const { return name_; }
};

using TextureSamplerImpl = GLTextureSampler;

class DirectTextureStorage
{
public:
	struct Buffer
	{
		void *data{};
		uint pitch = 0;

		constexpr Buffer() {}
		constexpr Buffer(void *data, uint pitch): data{data}, pitch{pitch} {}
	};

	virtual ~DirectTextureStorage() = 0;
	virtual CallResult setFormat(IG::PixmapDesc desc, GLuint tex) = 0;
	virtual Buffer lock(IG::WindowRect *dirtyRect) = 0;
	virtual void unlock(GLuint tex) = 0;
};

class GLLockedTextureBuffer
{
protected:
	IG::Pixmap pix;
	IG::WindowRect srcDirtyRect;
	uint lockedLevel = 0;

public:
	constexpr GLLockedTextureBuffer() {}
	void set(IG::Pixmap pix, IG::WindowRect srcDirtyRect, uint lockedLevel);
	uint level() const { return lockedLevel; }
};

using LockedTextureBufferImpl = GLLockedTextureBuffer;

class GLTexture
{
public:
	#ifdef __ANDROID__
	enum AndroidStorageImpl : int
	{
		ANDROID_AUTO,
		ANDROID_NONE,
		ANDROID_GRAPHIC_BUFFER,
		ANDROID_SURFACE_TEXTURE
	};
	#endif

protected:
	DirectTextureStorage *directTex{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint type_ = TEX_UNSET;
	#else
	static constexpr uint type_ = TEX_2D_4;
	#endif
	#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	GLenum target = GL_TEXTURE_2D;
	#else
	static constexpr GLenum target = GL_TEXTURE_2D;
	#endif
	TextureRef texName_ = 0;
	IG::PixmapDesc pixDesc;
	GLuint sampler = 0; // used when separate sampler objects not supported
	uint levels_ = 0;
	GLuint ownPBO = 0;
	#ifdef __ANDROID__
	static AndroidStorageImpl androidStorageImpl_;
	#endif

	static void setSwizzleForFormat(IG::PixelFormatID format, GLuint tex, GLenum target);

public:
	constexpr GLTexture() {}
	GLuint texName() const;
	#ifdef __ANDROID__
	static bool setAndroidStorageImpl(AndroidStorageImpl impl);
	static AndroidStorageImpl androidStorageImpl();
	static bool isAndroidGraphicBufferStorageWhitelisted();
	bool isExternal();
	static const char *androidStorageImplStr(AndroidStorageImpl);
	static const char *androidStorageImplStr();
	#endif
};

using TextureImpl = GLTexture;

}
