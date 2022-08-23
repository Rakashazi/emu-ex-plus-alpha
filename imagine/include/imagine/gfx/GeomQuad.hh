#pragma once

#include <imagine/config/defs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/edge.h>
#include <imagine/util/math/math.hh>
#include <imagine/util/concepts.hh>
#include <span>

namespace IG::Gfx
{

template<VertexLayout V>
constexpr auto mapQuadUV(std::array<V, 4> v, FRect rect, Rotation r = Rotation::UP)
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
	using TexCoord = decltype(V::texCoord.x);
	Rect2<TexCoord> nRect{};
	if constexpr(std::is_same_v<TexCoord, float>)
	{
		nRect = rect;
	}
	else
	{
		nRect.x = remapClamp(rect.x, -1.f, 1.f, std::numeric_limits<TexCoord>{});
		nRect.x2 = remapClamp(rect.x2, -1.f, 1.f, std::numeric_limits<TexCoord>{});
		nRect.y = remapClamp(rect.y, -1.f, 1.f, std::numeric_limits<TexCoord>{});
		nRect.y2 = remapClamp(rect.y2, -1.f, 1.f, std::numeric_limits<TexCoord>{});
	}
	if(rotate)
	{
		v[0].texCoord.x = nRect.x;  v[0].texCoord.y = nRect.y;  //BL
		v[1].texCoord.x = nRect.x2; v[1].texCoord.y = nRect.y;  //TL
		v[2].texCoord.x = nRect.x;  v[2].texCoord.y = nRect.y2; //BR
		v[3].texCoord.x = nRect.x2; v[3].texCoord.y = nRect.y2; //TR
	}
	else
	{
		v[0].texCoord.x = nRect.x;  v[0].texCoord.y = nRect.y2; //BL
		v[1].texCoord.x = nRect.x;  v[1].texCoord.y = nRect.y;  //TL
		v[2].texCoord.x = nRect.x2; v[2].texCoord.y = nRect.y2; //BR
		v[3].texCoord.x = nRect.x2; v[3].texCoord.y = nRect.y;  //TR
	}
	return v;
}

template<VertexLayout V>
constexpr std::array<V, 4> mapQuadPos(std::array<V, 4> v, FP bl, FP tl, FP tr, FP br)
{
	v[0].pos.x = bl.x; v[0].pos.y = bl.y;
	v[1].pos.x = tl.x; v[1].pos.y = tl.y;
	v[2].pos.x = br.x; v[2].pos.y = br.y;
	v[3].pos.x = tr.x; v[3].pos.y = tr.y;
	return v;
}

template<VertexLayout V>
constexpr std::array<V, 4> mapQuadPos(std::array<V, 4> v, GCRect rect)
{
	return mapQuadPos(v, {rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
}

template<VertexLayout V>
class QuadGeneric
{
public:
	constexpr QuadGeneric() = default;

	constexpr QuadGeneric(FP bl, FP tl, FP tr, FP br)
	{
		setPos(bl, tl, tr, br);
	}

	constexpr QuadGeneric(GCRect posRect)
	{
		setPos(posRect);
	}

	constexpr QuadGeneric(GCRect posRect, VertexColor col):
		QuadGeneric{posRect}
	{
		if constexpr(requires {V::color;})
		{
			for(auto &vtx : v) { vtx.color = col; }
		}
	}

	constexpr QuadGeneric(GCRect posRect, FRect uvRect):
		QuadGeneric{posRect}
	{
		setUV(uvRect);
	}

	constexpr QuadGeneric(GCRect posRect, TextureSpan texSpan):
		QuadGeneric{posRect, texSpan.uvBounds()} {}

	constexpr void setPos(FP bl, FP tl, FP tr, FP br)
	{
		v = mapQuadPos(v, bl, tl, tr, br);
	}

	constexpr void setPos(QuadGeneric quad)
	{
		setPos(
			{quad.v[0].pos.x, quad.v[0].pos.y},
			{quad.v[1].pos.x, quad.v[1].pos.y},
			{quad.v[3].pos.x, quad.v[3].pos.y},
			{quad.v[2].pos.x, quad.v[2].pos.y});
	}

	constexpr void setPos(GCRect rect)
	{
		setPos({rect.x, rect.y}, {rect.x, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y});
	}

	void setPos(IG::WindowRect b, ProjectionPlane proj)
	{
		setPos(proj.unProjectRect(b));
	}

	constexpr void setPosRel(float x, float y, float xSize, float ySize)
	{
		setPos({{x, y}, {x+xSize, y+ySize}});
	}

	constexpr void setUV(FRect rect, Rotation r = Rotation::UP)
	{
		if constexpr(requires {V::texCoord;})
		{
			v = mapQuadUV(v, rect, r);
		}
	}

	void draw(RendererCommands &cmds) const
	{
		cmds.bindTempVertexBuffer();
		cmds.vertexBufferData(v.data(), sizeof(v));
		cmds.setVertexAttribs(v.data());
		cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
	}

	static void draw(RendererCommands &cmds, IG::WindowRect b, ProjectionPlane proj)
	{
		draw(cmds, proj.unProjectRect(b));
	}

	static void draw(RendererCommands &cmds, GCRect d)
	{
		QuadGeneric rect{d};
		rect.draw(cmds);
	}

	constexpr auto &operator[](size_t idx) { return v[idx]; }
	constexpr auto &bl() { return v[0]; }
	constexpr auto &tl() { return v[1]; }
	constexpr auto &tr() { return v[3]; }
	constexpr auto &br() { return v[2]; }
	constexpr operator std::array<V, 4>&() { return v; }
	constexpr auto data() const { return v.data(); }
	constexpr auto size() const { return v.size(); }

protected:
	std::array<V, 4> v{};
};

using Quad = QuadGeneric<Vertex2P>;
using TexQuad = QuadGeneric<Vertex2PTex>;
using ColQuad = QuadGeneric<Vertex2PCol>;
using ColTexQuad = QuadGeneric<Vertex2PTexCol>;
using TexRect = TexQuad;
using GeomRect = Quad;

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

inline void drawQuads(RendererCommands &cmds, IG::Container auto &quads,
	std::span<const std::array<VertexIndex, 6>> quadIdxs)
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(quads.data(), sizeof(quads[0]) * quads.size());
	cmds.setVertexAttribs(quads[0].data());
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, {quadIdxs[0].data(), quadIdxs.size() * 6});
}

}
