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

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/GeomQuad.hh>

namespace IG::Gfx
{

template<VertexLayout V>
class SpriteBase : public QuadGeneric<V>
{
public:
	using BaseQuad = QuadGeneric<V>;
	using PosRect = typename BaseQuad::PosRect;
	using TexCoord = typename BaseQuad::TexCoord;
	using TexCoordRect = typename BaseQuad::TexCoordRect;
	using RectInitParams = BaseQuad::RectInitParams;

	constexpr SpriteBase():
		BaseQuad{{.textureBounds = remapTexCoordRect<TexCoordRect>({{}, {1.f, 1.f}})}} {}

	constexpr SpriteBase(PosRect pos, TextureSpan span = {}):
		BaseQuad{{.bounds = pos, .textureBounds = remapTexCoordRect<TexCoordRect>(span.bounds)}} {}

	constexpr void set(TextureSpan span, Rotation r = Rotation::UP)
	{
		set(span.texturePtr);
		setUVBounds(BaseQuad::remapTexCoordRect(span.bounds), r);
	}

	constexpr void setUVBounds(TexCoordRect uvBounds, Rotation r = Rotation::UP)
	{
		BaseQuad::setUV(uvBounds, r);
	}

	static void write(Buffer<V, BufferType::vertex> &buff, ssize_t offset, RectInitParams rectParams, TextureSpan texSpan = {})
	{
		rectParams.textureBounds = texSpan ? BaseQuad::remapTexCoordRect(texSpan.bounds) : BaseQuad::unitTexCoordRect();
		BaseQuad::write(buff, offset, rectParams);
	}

	constexpr static void write(std::span<V> span, ssize_t offset, RectInitParams rectParams, TextureSpan texSpan = {})
	{
		rectParams.textureBounds = texSpan ? BaseQuad::remapTexCoordRect(texSpan.bounds) : BaseQuad::unitTexCoordRect();
		BaseQuad::write(span, offset, rectParams);
	}

	void write(Buffer<V, BufferType::vertex> &buff, ssize_t offset) const
	{
		buff.task().write(buff, BaseQuad::v, offset * 4);
	}

	constexpr void write(std::span<V> span, ssize_t offset) const
	{
		std::ranges::copy(BaseQuad::v, span.begin() + offset * 4);
	}
};

using Sprite = SpriteBase<Vertex2ITexI>;
using ShadedSprite = SpriteBase<Vertex2ITexIColI>;
using LitSprite = SpriteBase<Vertex2ITexIColF>;

}
