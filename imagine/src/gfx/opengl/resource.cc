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

#include <imagine/gfx/Gfx.hh>

namespace Gfx
{

Texture Renderer::makeTexture(TextureConfig config)
{
	Texture t;
	t.init2(*this, config);
	return t;
}

Texture Renderer::makeTexture(GfxImageSource &img, bool makeMipmaps)
{
	Texture t;
	t.init2(*this, img, makeMipmaps);
	return t;
}

PixmapTexture Renderer::makePixmapTexture(TextureConfig config)
{
	PixmapTexture t;
	t.init2(*this, config);
	return t;
}

PixmapTexture Renderer::makePixmapTexture(GfxImageSource &img, bool makeMipmaps)
{
	PixmapTexture t;
	t.init2(*this, img, makeMipmaps);
	return t;
}

TextureSampler Renderer::makeTextureSampler(TextureSamplerConfig config)
{
	TextureSampler s;
	s.init2(*this, config);
	return s;
}

}
