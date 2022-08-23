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
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/BasicEffect.hh>

namespace IG::Gfx
{

class Texture;
class RendererCommands;
class Mat4;

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase():
		BaseRect{{}, FRect{{}, {1., 1.}}} {}

	constexpr SpriteBase(GCRect pos, TextureSpan span = {}):
		BaseRect{pos, span.uvBounds()},
		texBinding{span.texture() ? span.texture()->binding() : TextureBinding{}} {}

	constexpr void set(const Texture *tex)
	{
		if(tex)
			texBinding = tex->binding();
		else
			texBinding = {};
	}

	constexpr void set(TextureSpan span, Rotation r = Rotation::UP)
	{
		set(span.texture());
		setUVBounds(span.uvBounds(), r);
	}

	constexpr void setUVBounds(FRect uvBounds, Rotation r = Rotation::UP)
	{
		assert(uvBounds.xSize());
		BaseRect::setUV(uvBounds, r);
	}

	void draw(RendererCommands &cmds) const
	{
		if(!texBinding.name) [[unlikely]]
			return;
		cmds.set(texBinding);
		BaseRect::draw(cmds);
	}

	void draw(RendererCommands &cmds, BasicEffect &prog) const
	{
		if(!texBinding.name) [[unlikely]]
			return;
		prog.enableTexture(cmds, texBinding);
		BaseRect::draw(cmds);
	}

	constexpr TextureBinding textureBinding() const { return texBinding; }
	constexpr bool hasTexture() const { return texBinding.name; }

private:
	TextureBinding texBinding{};
};

using Sprite = SpriteBase<TexRect>;
using ShadedSprite = SpriteBase<ColTexQuad>;

}
