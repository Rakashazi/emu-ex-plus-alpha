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

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLPixmapTexture.hh>
#endif

class GfxImageSource;

namespace IG
{
class PixmapDesc;
}

namespace Gfx
{

class RendererTask;
class RendererCommands;

class PixmapTexture: public PixmapTextureImpl
{
public:
	using PixmapTextureImpl::PixmapTextureImpl;
	PixmapTexture(RendererTask &, TextureConfig config, IG::ErrorCode *errorPtr = nullptr);
	PixmapTexture(RendererTask &, GfxImageSource &img, const TextureSampler *compatSampler, bool makeMipmaps, IG::ErrorCode *errorPtr = nullptr);
	IG::ErrorCode setFormat(IG::PixmapDesc desc, uint8_t levels, ColorSpace c = {}, const TextureSampler *compatSampler = {});
	GTexCRect uvBounds() const;
	IG::PixmapDesc usedPixmapDesc() const;
	operator TextureSpan() const;
};

}
