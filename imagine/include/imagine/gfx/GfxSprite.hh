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
#include <imagine/gfx/Gfx.hh>
#include <imagine/gfx/GfxBufferImage.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GeomQuad.hh>

namespace Gfx
{

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase() {}
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, BufferImage *img);
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2)
	{
		return init(x, y, x2, y2, (BufferImage*)nullptr);
	}
	CallResult init()
	{
		return init(0, 0, 0, 0);
	}
	CallResult init(BufferImage *img)
	{
		return init(0, 0, 0, 0, img);
	}

	void deinit();
	void setImg(BufferImage *img);
	void setImg(BufferImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);
	void mapImg(GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);
	void draw() const;
	bool compileDefaultProgram(uint mode)
	{
		if(img)
			return img->compileDefaultProgram(mode);
		else
			return false;
	}
	void useDefaultProgram(uint mode, const Mat4 *modelMat)
	{
		if(img)
			img->useDefaultProgram(mode, modelMat);
	}
	void useDefaultProgram(uint mode) { useDefaultProgram(mode, nullptr); }
	void useDefaultProgram(uint mode, Mat4 modelMat) { useDefaultProgram(mode, &modelMat); }

	BufferImage *image() { return img; }

private:
	BufferImage *img = nullptr;
	#if defined __ANDROID__ && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	uint flags = 0;
	static constexpr uint HINT_NO_MATRIX_TRANSFORM = bit(0);
	int screenX = 0, screenY = 0, screenX2 = 0, screenY2 = 0;
	#endif
	void setRefImg(BufferImage *img);
};

using Sprite = SpriteBase<TexRect>;
using ShadedSprite = SpriteBase<ColTexQuad>;

}
