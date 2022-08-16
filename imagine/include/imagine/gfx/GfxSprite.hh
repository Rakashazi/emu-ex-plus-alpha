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
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/gfx/Vertex.hh>

namespace IG::Gfx
{

class Texture;
class RendererCommands;
class Mat4;
class BasicEffect;

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase():
		BaseRect{{}, FRect{{}, {1., 1.}}} {}

	SpriteBase(GCRect pos, TextureSpan span = {});
	void set(const Texture *);
	void set(TextureSpan, Rotation r = Rotation::UP);
	void setUVBounds(FRect uvBounds, Rotation r = Rotation::UP);
	void draw(RendererCommands &) const;
	void draw(RendererCommands &, BasicEffect &) const;
	TextureBinding textureBinding() const { return texBinding; }
	bool hasTexture() const { return texBinding.name; }

private:
	TextureBinding texBinding{};
};

using Sprite = SpriteBase<TexRect>;
using ShadedSprite = SpriteBase<ColTexQuad>;

std::array<TexVertex, 4> makeTexVertArray(GCRect pos, TextureSpan img);

}
