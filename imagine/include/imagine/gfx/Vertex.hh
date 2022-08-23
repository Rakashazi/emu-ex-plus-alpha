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
#include <imagine/glm/ext/vector_float2.hpp>

namespace IG::Gfx
{

class RendererCommands;

template <class T> constexpr inline AttribType attribType{};
template <> constexpr inline AttribType attribType<uint8_t> = AttribType::UByte;
template <> constexpr inline AttribType attribType<int16_t> = AttribType::Short;
template <> constexpr inline AttribType attribType<uint16_t> = AttribType::UShort;
template <> constexpr inline AttribType attribType<float> = AttribType::Float;

template <class T>
concept VertexLayout = requires
{
    T::pos;
    T::ID;
};

template <VertexLayout V>
constexpr AttribDesc posAttribDesc()
{
	using T = decltype(V::pos.x);
	return {offsetof(V, pos), sizeof(V::pos) / sizeof(T), attribType<T>};
}

template <VertexLayout V>
constexpr AttribDesc colorAttribDesc()
{
	if constexpr(requires {V::color;})
	{
		using T = decltype(V::color.r);
		return {offsetof(V, color), sizeof(V::color) / sizeof(T), attribType<T>};
	}
	else
	{
		return {};
	}
}

template <VertexLayout V>
constexpr AttribDesc texCoordAttribDesc()
{
	if constexpr(requires {V::texCoord;})
	{
		using T = decltype(V::texCoord.x);
		return {offsetof(V, texCoord), sizeof(V::texCoord) / sizeof(T), attribType<T>};
	}
	else
	{
		return {};
	}
}

struct Vertex2P
{
	glm::vec2 pos{};
	static constexpr unsigned ID = 1;
};

struct Vertex2PCol
{
	glm::vec2 pos{};
	VertexColor color{};
	static constexpr unsigned ID = 2;
};

struct Vertex2PTex
{
	glm::vec2 pos{};
	glm::vec2 texCoord{};
	static constexpr unsigned ID = 3;
};

struct Vertex2PTexCol
{
	glm::vec2 pos{};
	glm::vec2 texCoord{};
	VertexColor color{};
	static constexpr unsigned ID = 4;
};

}
