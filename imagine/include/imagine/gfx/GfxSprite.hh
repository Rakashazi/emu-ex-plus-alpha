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

template<VertexLayout V>
class SpriteBase : public QuadGeneric<V>
{
public:
	using BaseQuad = QuadGeneric<V>;
	using PosRect = typename BaseQuad::PosRect;
	using TexCoord = typename BaseQuad::TexCoord;
	using TexCoordRect = typename BaseQuad::TexCoordRect;

	constexpr SpriteBase():
		BaseQuad{{.textureBounds = remapTexCoordRect<TexCoordRect>({{}, {1.f, 1.f}})}} {}

	constexpr SpriteBase(PosRect pos, TextureSpan span = {}):
		BaseQuad{{.bounds = pos, .textureBounds = remapTexCoordRect<TexCoordRect>(span.bounds)}},
		texBinding{span.texturePtr ? span.texturePtr->binding() : TextureBinding{}} {}

	constexpr void set(const Texture *tex)
	{
		if(tex)
			texBinding = tex->binding();
		else
			texBinding = {};
	}

	constexpr void set(TextureSpan span, Rotation r = Rotation::UP)
	{
		set(span.texturePtr);
		setUVBounds(BaseQuad::remapTexCoordRect(span.bounds), r);
	}

	constexpr void setUVBounds(TexCoordRect uvBounds, Rotation r = Rotation::UP)
	{
		BaseQuad::setUV(uvBounds, r);
	}

	void draw(RendererCommands &cmds) const
	{
		if(!texBinding.name) [[unlikely]]
			return;
		cmds.set(texBinding);
		BaseQuad::draw(cmds);
	}

	void draw(RendererCommands &cmds, BasicEffect &prog) const
	{
		if(!texBinding.name) [[unlikely]]
			return;
		prog.enableTexture(cmds, texBinding);
		BaseQuad::draw(cmds);
	}

	constexpr TextureBinding textureBinding() const { return texBinding; }
	constexpr bool hasTexture() const { return texBinding.name; }

private:
	TextureBinding texBinding{};
};

using Sprite = SpriteBase<Vertex2ITexI>;
using ShadedSprite = SpriteBase<Vertex2ITexIColI>;
using LitSprite = SpriteBase<Vertex2ITexIColF>;

}
