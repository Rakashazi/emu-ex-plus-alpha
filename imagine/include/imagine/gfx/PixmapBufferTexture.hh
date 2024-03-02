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

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLPixmapBufferTexture.hh>
#endif

namespace IG::Gfx
{

class Renderer;
class RendererTask;

// A limited 1-level version of Texture with dedicated pixel buffer for frequent data transfer

class PixmapBufferTexture: public PixmapBufferTextureImpl
{
public:
	static constexpr uint32_t MAX_ASSUME_ALIGN = Texture::MAX_ASSUME_ALIGN;

	using PixmapBufferTextureImpl::PixmapBufferTextureImpl;
	PixmapBufferTexture(RendererTask &, TextureConfig config, TextureBufferMode mode = {}, bool singleBuffer = false);
	bool setFormat(PixmapDesc desc, ColorSpace c = {}, TextureSamplerConfig samplerConf = {});
	void write(PixmapView pixmap, TextureWriteFlags writeFlags = {});
	void writeAligned(PixmapView pixmap, int assumedDataAlignment, TextureWriteFlags writeFlags = {});
	void clear();
	LockedTextureBuffer lock(TextureBufferFlags bufferFlags = {});
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags = {});
	WSize size() const;
	PixmapDesc pixmapDesc() const;
	void setSampler(TextureSamplerConfig);
	explicit operator bool() const;
	Renderer &renderer() const;
	operator TextureSpan() const;
	operator const Texture&() const;
	bool isExternal() const;
};

}
