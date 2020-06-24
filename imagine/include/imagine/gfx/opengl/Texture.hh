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

class GfxImageSource;

namespace Gfx
{

class Renderer;
class TextureSampler;

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
	GLenum minFilter = 0;
	GLenum magFilter = 0;
	GLenum xWrapMode_ = 0;
	GLenum yWrapMode_ = 0;
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, const char *> debugLabel{};
};

using TextureSamplerImpl = GLTextureSampler;

class DirectTextureStorage
{
public:
	struct Buffer
	{
		void *data{};
		uint32_t pitch = 0;

		constexpr Buffer() {}
		constexpr Buffer(void *data, uint32_t pitch): data{data}, pitch{pitch} {}
	};

	virtual ~DirectTextureStorage() = 0;
	virtual Error setFormat(Renderer &r, IG::PixmapDesc desc, GLuint tex) = 0;
	virtual Buffer lock(Renderer &r, IG::WindowRect *dirtyRect) = 0;
	virtual void unlock(Renderer &r, GLuint tex) = 0;
};

class GLLockedTextureBuffer
{
protected:
	IG::Pixmap pix;
	IG::WindowRect srcDirtyRect;
	uint32_t lockedLevel = 0;
	GLuint pbo_ = 0;

public:
	constexpr GLLockedTextureBuffer() {}
	void set(IG::Pixmap pix, IG::WindowRect srcDirtyRect, uint32_t lockedLevel, GLuint pbo);
	uint32_t level() const { return lockedLevel; }
	GLuint pbo() const { return pbo_; };
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

	constexpr GLTexture() {}
	GLTexture(Renderer &r, TextureConfig config, Error *errorPtr = nullptr);
	GLTexture(Renderer &r, GfxImageSource &img, bool makeMipmaps, Error *errorPtr = nullptr);
	~GLTexture();
	Error init(Renderer &r, TextureConfig config);
	GLuint texName() const;
	#ifdef __ANDROID__
	static bool setAndroidStorageImpl(Renderer &r, AndroidStorageImpl impl);
	static AndroidStorageImpl androidStorageImpl(Renderer &r);
	static bool isAndroidGraphicBufferStorageWhitelisted(Renderer &r);
	bool isExternal();
	static const char *androidStorageImplStr(AndroidStorageImpl);
	static const char *androidStorageImplStr(Renderer &r);
	#endif
	void bindTex(RendererCommands &cmds, const TextureSampler &sampler);
	bool canUseMipmaps(Renderer &r) const;

protected:
	Renderer *r{};
	DirectTextureStorage *directTex{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint32_t type_ = TEX_UNSET;
	#else
	static constexpr uint32_t type_ = TEX_2D_4;
	#endif
	#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	GLenum target = GL_TEXTURE_2D;
	#else
	static constexpr GLenum target = GL_TEXTURE_2D;
	#endif
	TextureRef texName_ = 0;
	IG::PixmapDesc pixDesc;
	GLuint sampler = 0; // used when separate sampler objects not supported
	uint32_t levels_ = 0;
	#ifdef __ANDROID__
	static AndroidStorageImpl androidStorageImpl_;
	#endif

	void deinit();
	static void setSwizzleForFormat(Renderer &r, IG::PixelFormatID format, GLuint tex, GLenum target);
};

using TextureImpl = GLTexture;

}
