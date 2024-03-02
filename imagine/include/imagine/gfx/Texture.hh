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

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureConfig.hh>
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <utility>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLTexture.hh>
#endif

namespace IG::Data
{
class PixmapSource;
}

namespace IG::Gfx
{

class LockedTextureBuffer: public LockedTextureBufferImpl
{
public:
	using LockedTextureBufferImpl::LockedTextureBufferImpl;
	MutablePixmapView pixmap() const;
	WRect sourceDirtyRect() const;
	explicit operator bool() const;
};

struct TextureWriteFlags
{
	uint8_t
	async:1{},
	makeMipmaps:1{};
};

struct TextureBufferFlags
{
	uint8_t
	clear:1{};
};

class Texture: public TextureImpl
{
public:
	static constexpr uint32_t MAX_ASSUME_ALIGN = 8;

	using TextureImpl::TextureImpl;
	Texture(RendererTask &, TextureConfig);
	Texture(RendererTask &, Data::PixmapSource, TextureSamplerConfig, bool makeMipmaps);
	static int bestAlignment(PixmapView pixmap);
	bool canUseMipmaps() const;
	bool generateMipmaps();
	int levels() const;
	bool setFormat(PixmapDesc, int levels, ColorSpace c = {}, TextureSamplerConfig samplerConf = {});
	void write(int level, PixmapView pixmap, WPt destPos, TextureWriteFlags writeFlags = {});
	void writeAligned(int level, PixmapView pixmap, WPt destPos, int assumedDataAlignment, TextureWriteFlags writeFlags = {});
	void clear(int level);
	LockedTextureBuffer lock(int level, TextureBufferFlags bufferFlags = {});
	LockedTextureBuffer lock(int level, WRect rect, TextureBufferFlags bufferFlags = {});
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags = {});
	WSize size(int level) const;
	PixmapDesc pixmapDesc() const;
	void setSampler(TextureSamplerConfig);
	explicit operator bool() const;
	Renderer &renderer() const;
	RendererTask &task() const;
	operator TextureSpan() const;
};

}
