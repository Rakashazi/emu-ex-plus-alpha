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

#include <imagine/gfx/Texture.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLPixmapBufferTexture.hh>
#endif

namespace Gfx
{

// A limited 1-level version of PixmapTexture with dedicated pixel buffer for frequent data transfer

class PixmapBufferTexture: public PixmapTexture, public PixmapBufferTextureImpl
{
public:
	constexpr PixmapBufferTexture() {}
	PixmapBufferTexture(Renderer &r, TextureConfig config, TextureBufferMode mode = {}, bool singleBuffer = false, IG::ErrorCode *errorPtr = nullptr);
	PixmapBufferTexture(PixmapBufferTexture &&o);
	PixmapBufferTexture &operator=(PixmapBufferTexture &&o);
	~PixmapBufferTexture();
	IG::ErrorCode setFormat(IG::PixmapDesc desc);
	void write(IG::Pixmap pixmap, uint32_t writeFlags = 0);
	void writeAligned(IG::Pixmap pixmap, uint8_t assumedDataAlignment, uint32_t writeFlags = 0);
	void clear();
	LockedTextureBuffer lock(uint32_t bufferFlags = 0);
	void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags = 0);
	IG::WP size() const;

protected:
	using PixmapTexture::setFormat;
	using Texture::clear;
	using Texture::write;
	using Texture::writeAligned;
	using Texture::lock;
	using Texture::generateMipmaps;
};

}
