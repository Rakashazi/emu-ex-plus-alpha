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

#include <imagine/gfx/Quads.hh>
#include <numeric>

namespace IG::Gfx
{

// A quad split into 4 triangles with a center vertex

template<VertexLayout V>
class BaseFanQuad : public BaseQuad<V>
{
public:
	using Base = BaseQuad<V>;
	using Pos = Base::Pos;
	using PosRect = Base::PosRect;
	using PosPoint = Base::PosPoint;
	using InitParams = Base::InitParams;
	using TexCoordRect = Base::TexCoordRect;
	using Base::v;

	static constexpr size_t vertexCount = 5;
	V centerV;

	constexpr BaseFanQuad() = default;

	constexpr BaseFanQuad(InitParams params)
	{
		setPos(params.bounds);
		if constexpr(requires {V::color;})
		{
			for(auto &vtx : *this) { vtx.color = params.color; }
		}
		if constexpr(requires {V::texCoord;})
		{
			if(params.textureSpan)
				params.textureBounds = Base::remapTexCoordRect(params.textureSpan.bounds);
			setUV(params.textureBounds, params.rotation);
		}
	}

	constexpr void setPos(QuadPoints<Pos> p)
	{
		v = mapQuadPos(v, p.bl, p.tl, p.tr, p.br);
		centerV.pos = {std::midpoint(p.bl.x, p.tr.x), std::midpoint(p.bl.y, p.tr.y)};
	}

	constexpr void setUV(TexCoordRect rect, Rotation r = Rotation::UP)
	{
		if constexpr(requires {V::texCoord;})
		{
			v = mapQuadUV(v, rect, r);
			centerV.texCoord = {std::midpoint(v[0].texCoord.x, v[3].texCoord.x), std::midpoint(v[0].texCoord.y, v[3].texCoord.y)};
		}
	}

	constexpr std::array<V, vertexCount> toArray() const
	{
		std::array<V, vertexCount> arr;
		std::ranges::copy(v, arr.begin());
		arr[4] = centerV;
		return arr;
	}

	void write(Buffer<V, BufferType::vertex> &buff, ssize_t offset) const
	{
		buff.task().write(buff, toArray(), offset * vertexCount);
	}

	constexpr void write(std::span<V> span, ssize_t offset) const
	{
		std::ranges::copy(toArray(), span.begin() + offset * vertexCount);
	}

	constexpr auto size() const { return v.size() + 1; }
	constexpr auto end() { return v.end() + 1; }
	constexpr auto end() const { return v.end() + 1; }
};

using ILitTexFanQuad = BaseFanQuad<Vertex2ITexIColF>;

using ILitTexFanQuads = ObjectVertexArray<ILitTexFanQuad>;

template<class T>
constexpr std::array<T, 12> mapFanQuadIndices(T baseIdx)
{
	baseIdx *= 5;
	static constexpr T tl = 0, bl = 1, tr = 2, br = 3, cen = 4;
	return
	{{
		T(baseIdx + tl),
		T(baseIdx + bl),
		T(baseIdx + cen),
		T(baseIdx + tl),
		T(baseIdx + cen),
		T(baseIdx + tr),
		T(baseIdx + tr),
		T(baseIdx + br),
		T(baseIdx + cen),
		T(baseIdx + br),
		T(baseIdx + bl),
		T(baseIdx + cen),
	}};
}

}
