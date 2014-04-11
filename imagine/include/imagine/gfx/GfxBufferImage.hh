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

#include <imagine/engine-globals.h>
#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include <imagine/util/RefCount.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/BufferImage.hh>
#endif

namespace Gfx
{

class BufferImage: private BufferImageImpl, public RefCount<BufferImage>
{
public:
	static constexpr uint NEAREST = 0, LINEAR = 1;
	static constexpr uint HINT_STREAM = IG::bit(0), HINT_NO_MINIFY = IG::bit(1);
	static constexpr uint MAX_ASSUME_ALIGN = 8;

	constexpr BufferImage() {}
	CallResult init(IG::Pixmap &pix, bool upload, uint filter, uint hints, bool textured);
	CallResult init(IG::Pixmap &pix, bool upload, uint filter = LINEAR, uint hints = HINT_STREAM)
	{
		return init(pix, upload, filter, hints, 0);
	}
	CallResult init(GfxImageSource &img, uint filter = LINEAR, uint hints = 0, bool textured = 0);
	static uint bestAlignment(const IG::Pixmap &p);
	bool hasMipmaps();
	static bool isFilterValid(uint v) { return v <= 1; }
	void setFilter(uint filter);
	void setRepeatMode(uint xMode, uint yMode);
	void deinit();
	void write(IG::Pixmap &p);
	void write(IG::Pixmap &p, uint assumeAlign);
	void replace(IG::Pixmap &p);
	void unlock(IG::Pixmap *p);
	const TextureDesc &textureDesc() const { return BufferImageImpl::textureDesc(); };
	TextureDesc &textureDesc() { return BufferImageImpl::textureDesc(); };
	uint type() const { return type_; }
	bool compileDefaultProgram(uint mode);
	void useDefaultProgram(uint mode, const Mat4 *modelMat);
	void useDefaultProgram(uint mode) { useDefaultProgram(mode, nullptr); }
	void useDefaultProgram(uint mode, Mat4 modelMat) { useDefaultProgram(mode, &modelMat); }

	void free()
	{
		logMsg("BufferImage %p has no more references", this);
		deinit();
	};

	operator bool() const
	{
		return isInit();
	}

private:
	uint hints = 0;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint type_ = TEX_UNSET;
	#else
	static constexpr uint type_ = TEX_2D_4;
	#endif
	bool hasMipmaps_ = false;
	#if defined __ANDROID__ && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	uint xSize = 0, ySize = 0; // the actual x,y size of the image content
	#endif

	void testMipmapSupport(uint x, uint y);
	void generateMipmaps();
	bool setupTexture(IG::Pixmap &pix, bool upload, uint internalFormat, int xWrapType, int yWrapType,
			uint usedX, uint usedY, uint hints, uint filter);
};

}
