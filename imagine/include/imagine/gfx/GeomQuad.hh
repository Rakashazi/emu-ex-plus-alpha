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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/edge.h>
#include <imagine/util/math/math.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/used.hh>
#include <span>
#include <array>
#include <utility>
#include <type_traits>

namespace IG::Gfx
{

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
class QuadGeneric
{
public:
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
		TexCoordRect textureBounds{};
	};

	static constexpr int tlIdx = 0, blIdx = 1, trIdx = 2, brIdx = 3;

	std::array<V, 4> v;

	constexpr QuadGeneric() = default;

	constexpr QuadGeneric(PosPoint bl, PosPoint tl, PosPoint tr, PosPoint br)
	{
		setPos(bl, tl, tr, br);
	}

	constexpr QuadGeneric(RectInitParams params)
	{
		setPos(params.bounds);
		if constexpr(requires {V::color;})
		{
			for(auto &vtx : v) { vtx.color = params.color; }
		}
		if constexpr(requires {V::texCoord;})
		{
			setUV(params.textureBounds);
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
		if(texCoordAttribDesc<V>().normalize)
			return Gfx::remapTexCoordRect<TexCoordRect>(rect);
		return rect.as<TexCoord>();
	}

	static constexpr TexCoordRect unitTexCoordRect() { return remapTexCoordRect({{}, {1.f, 1.f}}); }

	void draw(RendererCommands &cmds) const
	{
		cmds.bindTempVertexBuffer();
		cmds.vertexBufferData(v.data(), sizeof(v));
		cmds.setVertexAttribs(v.data());
		cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
	}

	static void draw(RendererCommands &cmds, RectInitParams params)
	{
		QuadGeneric rect{params};
		rect.draw(cmds);
	}

	static void draw(RendererCommands &cmds, PosRect bounds)
	{
		draw(cmds, RectInitParams{.bounds = bounds});
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

using Quad = QuadGeneric<Vertex2F>;
using TexQuad = QuadGeneric<Vertex2FTexF>;
using ColQuad = QuadGeneric<Vertex2FColI>;
using ColTexQuad = QuadGeneric<Vertex2FTexFColI>;
using IQuad = QuadGeneric<Vertex2I>;
using ITexQuad = QuadGeneric<Vertex2ITexI>;
using IColQuad = QuadGeneric<Vertex2IColI>;
using IColTexQuad = QuadGeneric<Vertex2ITexIColI>;

constexpr std::array<VertexIndex, 6> makeRectIndexArray(VertexIndex baseIdx)
{
	baseIdx *= 4;
	return
	{{
		baseIdx,
		VertexIndex(baseIdx+1),
		VertexIndex(baseIdx+3),
		baseIdx,
		VertexIndex(baseIdx+3),
		VertexIndex(baseIdx+2),
	}};
}

inline void drawQuads(RendererCommands &cmds, Container auto &quads,
	std::span<const std::array<VertexIndex, 6>> quadIdxs)
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(quads.data(), sizeof(quads[0]) * quads.size());
	cmds.setVertexAttribs(quads[0].data());
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, {quadIdxs[0].data(), quadIdxs.size() * 6});
}

}
