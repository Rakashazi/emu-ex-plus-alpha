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
#include <imagine/gfx/Mat4.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GeomQuad.hh>

namespace Gfx
{

class RendererCommands;

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase():
		BaseRect{{}, GTexCRect{0, 0, 1, 1}}
	{}

	constexpr SpriteBase(GCRect pos, TextureSpan span = {}):
		BaseRect{pos, span.uvBounds()},
		img{span.texture()}
	{}

	void setImg(const Texture *img);
	void setImg(TextureSpan span);
	void setUVBounds(GTexCRect uvBounds);
	void draw(RendererCommands &r) const;
	bool compileDefaultProgram(uint32_t mode);
	bool compileDefaultProgramOneShot(uint32_t mode);
	void setCommonProgram(RendererCommands &cmds, uint32_t mode, const Mat4 *modelMat = {}) const;
	void setCommonProgram(RendererCommands &cmds, uint32_t mode, Mat4 modelMat) const;
	const Texture *image() const;

private:
	const Texture *img{};
};

using Sprite = SpriteBase<TexRect>;
using ShadedSprite = SpriteBase<ColTexQuad>;

std::array<TexVertex, 4> makeTexVertArray(GCRect pos, TextureSpan img);

}
