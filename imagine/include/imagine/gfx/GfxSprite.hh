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
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GeomQuad.hh>

namespace Gfx
{

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase() {}
	CallResult init(GCRect pos, Texture *img, IG::Rect2<GTexC> uvBounds);

	CallResult init(GCRect pos)
	{
		return init(pos, (Texture*)nullptr, {});
	}

	CallResult init(GCRect pos, PixmapTexture &img)
	{
		return init(pos, &img, img.uvBounds());
	}

	void deinit();
	void setImg(Texture *img);

	void setImg(Texture *img, IG::Rect2<GTexC> uvBounds)
	{
		setImg(img);
		setUVBounds(uvBounds);
	}

	void setImg(PixmapTexture &img)
	{
		setImg(&img, img.uvBounds());
	}

	void setUVBounds(IG::Rect2<GTexC> uvBounds);
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
	Texture *image() { return img; }

private:
	Texture *img{};
};

using Sprite = SpriteBase<TexRect>;
using ShadedSprite = SpriteBase<ColTexQuad>;

}
