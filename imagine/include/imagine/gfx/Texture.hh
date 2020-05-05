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

#include <utility>
#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureConfig.hh>
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/pixmap/Pixmap.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/Texture.hh>
#endif

class GfxImageSource;

namespace Gfx
{

class Renderer;
class RendererCommands;

class TextureSampler: public TextureSamplerImpl
{
public:
	using TextureSamplerImpl::TextureSamplerImpl;
	TextureSampler(TextureSampler &&o);
	TextureSampler &operator=(TextureSampler &&o);
	explicit operator bool() const;
};

class LockedTextureBuffer: public LockedTextureBufferImpl
{
public:
	constexpr LockedTextureBuffer() {}
	IG::Pixmap pixmap() const;
	IG::WindowRect sourceDirtyRect() const;
	explicit operator bool() const;
};

class Texture: public TextureImpl
{
public:
	static constexpr uint32_t MAX_ASSUME_ALIGN = 8;
	static constexpr uint32_t COMMIT_FLAG_ASYNC = IG::bit(0);

	using TextureImpl::TextureImpl;
	Texture(Texture &&o);
	Texture &operator=(Texture &&o);
	static uint32_t bestAlignment(const IG::Pixmap &pixmap);
	bool canUseMipmaps() const;
	bool generateMipmaps();
	uint32_t levels() const;
	Error setFormat(IG::PixmapDesc desc, uint32_t levels);
	void write(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t commitFlags = 0);
	void writeAligned(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t assumedDataAlignment, uint32_t commitFlags = 0);
	void clear(uint32_t level);
	LockedTextureBuffer lock(uint32_t level);
	LockedTextureBuffer lock(uint32_t level, IG::WindowRect rect);
	void unlock(LockedTextureBuffer lockBuff);
	IG::WP size(uint32_t level) const;
	IG::PixmapDesc pixmapDesc() const;
	bool compileDefaultProgram(uint32_t mode);
	bool compileDefaultProgramOneShot(uint32_t mode);
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode, const Mat4 *modelMat) const;
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode) const { useDefaultProgram(cmds, mode, nullptr); }
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode, Mat4 modelMat) const { useDefaultProgram(cmds, mode, &modelMat); }
	explicit operator bool() const;
	Renderer &renderer();
};

class PixmapTexture: public Texture
{
public:
	constexpr PixmapTexture() {}
	PixmapTexture(Renderer &r, TextureConfig config, Error *errorPtr = nullptr);
	PixmapTexture(Renderer &r, GfxImageSource &img, bool makeMipmaps, Error *errorPtr = nullptr);
	Error init(Renderer &r, TextureConfig config);
	Error setFormat(IG::PixmapDesc desc, uint32_t levels);
	IG::Rect2<GTexC> uvBounds() const;
	IG::PixmapDesc usedPixmapDesc() const;

protected:
	IG::Rect2<GTexC> uv{};
	IG::WP usedSize{};

	void updateUV(IG::WP pixPos, IG::WP pixSize);
};

}
