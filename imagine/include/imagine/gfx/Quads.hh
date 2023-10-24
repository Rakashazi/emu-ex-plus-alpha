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
#include <imagine/gfx/Vertex.hh>
#include <imagine/gfx/Buffer.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/ranges.hh>
#include <span>
#include <array>
#include <utility>
#include <type_traits>

namespace IG::Gfx
{

template <class T>
class QuadIndexArray : public IndexBuffer<T>
{
public:
	constexpr QuadIndexArray() = default;

	QuadIndexArray(RendererTask &rTask, size_t maxQuads, BufferUsageHint usageHint = BufferUsageHint::constant):
		IndexBuffer<T>{rTask, {.size = maxQuads * 6, .usageHint = usageHint}}
	{
		auto indices = this->map();
		for(auto i : iotaCount(maxQuads))
		{
			std::ranges::copy(makeRectIndexArray(i), indices.begin() + (i * 6));
		}
	}
};

template<VertexLayout V>
constexpr auto mapQuadUV(std::array<V, 4> v, Rectangle auto rect, Rotation r = Rotation::UP)
{
	bool rotate = false;
	if(r == Rotation::DOWN)
	{
		std::swap(rect.x, rect.x2);
		std::swap(rect.y, rect.y2);
	}
	else if(r == Rotation::RIGHT)
	{
		rotate = true;
	}
	else if(r == Rotation::LEFT)
	{
		std::swap(rect.x, rect.x2);
		std::swap(rect.y, rect.y2);
		rotate = true;
	}
	if(rotate)
	{
		v[0].texCoord = {rect.x2, rect.y};  //BL
		v[1].texCoord = {rect.x,  rect.y};  //TL
		v[2].texCoord = {rect.x2, rect.y2}; //BR
		v[3].texCoord = {rect.x,  rect.y2}; //TR
	}
	else
	{
		v[0].texCoord = {rect.x,  rect.y};  //BL
		v[1].texCoord = {rect.x,  rect.y2}; //TL
		v[2].texCoord = {rect.x2, rect.y};  //BR
		v[3].texCoord = {rect.x2, rect.y2}; //TR
	}
	return v;
}

template<VertexLayout V>
constexpr std::array<V, 4> mapQuadPos(std::array<V, 4> v, Point auto bl, Point auto tl, Point auto tr, Point auto br)
{
	v[0].pos = {bl.x, bl.y};
	v[1].pos = {tl.x, tl.y};
	v[2].pos = {br.x, br.y};
	v[3].pos = {tr.x, tr.y};
	return v;
}

template<VertexLayout V>
constexpr std::array<V, 4> mapQuadPos(std::array<V, 4> v, Rectangle auto rect)
{
	return mapQuadPos(v, {rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
}

template<Rectangle OutRect>
constexpr OutRect remapTexCoordRect(FRect rect)
{
	if constexpr(std::is_same_v<OutRect, FRect>)
		return rect;
	// normalize integers
	using OutValue = decltype(OutRect::x);
	auto outLimits = std::numeric_limits<OutValue>{};
	float min = std::is_signed_v<OutValue> ? -1.f : 0.f;
	return
	{
		{OutValue(remap(rect.x, min, 1.f, outLimits)), OutValue(remap(rect.y, min, 1.f, outLimits))},
		{OutValue(remap(rect.x2, min, 1.f, outLimits)), OutValue(remap(rect.y2, min, 1.f, outLimits))}
	};
}

template<VertexLayout V>
class BaseQuad
{
public:
	using Vertex = V;
	using Pos = decltype(V::pos.x);
	using PosRect = Rect2<Pos>;
	using PosPoint = Point2D<Pos>;
	using Color = IG_GetValueTypeOr(V::color, UnusedType<Color>);
	using TexCoord = IG_GetValueTypeOr(V::texCoord.x, UnusedType<float>);
	using TexCoordRect = std::conditional_t<used(TexCoord{}), Rect2<TexCoord>, UnusedType<FRect>>;

	struct RectInitParams
	{
		PosRect bounds{};
		Color color{};
		TexCoordRect textureBounds = unitTexCoordRect();
		TextureSpan textureSpan{};
		Rotation rotation = Rotation::UP;
	};

	static constexpr int tlIdx = 0, blIdx = 1, trIdx = 2, brIdx = 3;

	std::array<V, 4> v;

	constexpr BaseQuad() = default;

	constexpr BaseQuad(RectInitParams params)
	{
		setPos(params.bounds);
		if constexpr(requires {V::color;})
		{
			for(auto &vtx : v) { vtx.color = params.color; }
		}
		if constexpr(requires {V::texCoord;})
		{
			if(params.textureSpan)
				params.textureBounds = remapTexCoordRect(params.textureSpan.bounds);
			setUV(params.textureBounds, params.rotation);
		}
	}

	constexpr void setPos(PosPoint bl, PosPoint tl, PosPoint tr, PosPoint br)
	{
		v = mapQuadPos(v, bl, tl, tr, br);
	}

	constexpr void setPos(const auto &quad)
	{
		setPos(
			{quad.v[0].pos.x, quad.v[0].pos.y},
			{quad.v[1].pos.x, quad.v[1].pos.y},
			{quad.v[3].pos.x, quad.v[3].pos.y},
			{quad.v[2].pos.x, quad.v[2].pos.y});
	}

	constexpr void setPos(PosRect rect)
	{
		setPos({rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
	}

	constexpr void setPos(WRect rect) { setPos(rect.as<Pos>()); }

	constexpr void setUV(TexCoordRect rect, Rotation r = Rotation::UP)
	{
		if constexpr(requires {V::texCoord;})
		{
			v = mapQuadUV(v, rect, r);
		}
	}

	static constexpr TexCoordRect remapTexCoordRect(FRect rect)
	{
		if constexpr(requires {V::texCoord;})
		{
			if(texCoordAttribDesc<V>().normalize)
				return Gfx::remapTexCoordRect<TexCoordRect>(rect);
			return rect.as<TexCoord>();
		}
		else
		{
			return {};
		}
	}

	static constexpr TexCoordRect unitTexCoordRect() { return remapTexCoordRect({{}, {1.f, 1.f}}); }

	void write(Buffer<V, BufferType::vertex> &buff, ssize_t offset) const
	{
		buff.task().write(buff, v, offset * 4);
	}

	constexpr void write(std::span<V> span, ssize_t offset) const
	{
		std::ranges::copy(v, span.begin() + offset * 4);
	}

	constexpr auto &operator[](size_t idx) { return v[idx]; }
	constexpr auto &bl() { return v[blIdx]; }
	constexpr auto &tl() { return v[tlIdx]; }
	constexpr auto &tr() { return v[trIdx]; }
	constexpr auto &br() { return v[brIdx]; }
	constexpr operator std::array<V, 4>&() { return v; }
	constexpr auto data() const { return v.data(); }
	constexpr auto size() const { return v.size(); }
	constexpr auto begin() { return v.begin(); }
	constexpr auto begin() const { return v.begin(); }
	constexpr auto end() { return v.end(); }
	constexpr auto end() const { return v.end(); }
};

using Quad = BaseQuad<Vertex2F>;
using TexQuad = BaseQuad<Vertex2FTexF>;
using ColQuad = BaseQuad<Vertex2FColI>;
using ColTexQuad = BaseQuad<Vertex2FTexFColI>;
using IQuad = BaseQuad<Vertex2I>;
using ITexQuad = BaseQuad<Vertex2ITexI>;
using IColQuad = BaseQuad<Vertex2IColI>;
using IColTexQuad = BaseQuad<Vertex2ITexIColI>;
using ILitTexQuad = BaseQuad<Vertex2ITexIColF>;

template<class T>
class QuadsConfig
{
public:
	size_t size;
	BufferUsageHint usageHint{BufferUsageHint::dynamic};

	constexpr BufferConfig<T> toBufferConfig() const
	{
		return
		{
			.size = size * 4,
			.usageHint = usageHint,
		};
	}
};

template<VertexLayout V>
class BaseQuads : public VertexBuffer<V>
{
public:
	using Quad = BaseQuad<V>;

	constexpr BaseQuads() = default;
	BaseQuads(RendererTask &rTask, QuadsConfig<V> config): VertexBuffer<V>{rTask, config.toBufferConfig()} {}
	void reset(QuadsConfig<V> config) { VertexBuffer<V>::reset(config.toBufferConfig()); }
	void write(ssize_t offset, Quad quad) { quad.write(*this, offset); }
	void write(ssize_t offset, Quad::RectInitParams params) { write(offset, Quad{params}); }
};

using Quads = BaseQuads<Vertex2F>;
using TexQuads = BaseQuads<Vertex2FTexF>;
using ColQuads = BaseQuads<Vertex2FColI>;
using ColTexQuads = BaseQuads<Vertex2FTexFColI>;
using IQuads = BaseQuads<Vertex2I>;
using ITexQuads = BaseQuads<Vertex2ITexI>;
using IColQuads = BaseQuads<Vertex2IColI>;
using IColTexQuads = BaseQuads<Vertex2ITexIColI>;
using ILitTexQuads = BaseQuads<Vertex2ITexIColF>;

}
