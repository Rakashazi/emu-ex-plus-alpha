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
#include <imagine/gfx/Texture.hh>

namespace Gfx
{

class RendererTask;
class PixmapTexture;

class GLPixmapTexture : public Texture
{
public:
	using Texture::Texture;
	constexpr GLPixmapTexture() {}

protected:
	IG::ErrorCode init(RendererTask &rTask, TextureConfig config);
	void updateUsedPixmapSize(IG::WP usedSize, IG::WP fullSize);
	void updateFormatInfo(IG::WP usedSize, IG::PixmapDesc, uint8_t levels, GLenum target = GL_TEXTURE_2D);
	#ifdef __ANDROID__
	void initWithEGLImage(IG::WP usedSize, EGLImageKHR, IG::PixmapDesc, SamplerParams, bool isMutable);
	#endif

	GTexCPoint uv{};
	IG::WP usedSize{};
};

using PixmapTextureImpl = GLPixmapTexture;

}
