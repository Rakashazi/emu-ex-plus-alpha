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
#include <imagine/glm/ext/vector_int2_sized.hpp>

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
};

enum class VertexLayoutAttribMask : uint8_t
{
	Position = bit(0),
	TextureCoordinate = bit(1),
	Color = bit(2),
};

IG_DEFINE_ENUM_BIT_FLAG_FUNCTIONS(VertexLayoutAttribMask);

template <VertexLayout V>
constexpr VertexLayoutAttribMask vertexLayoutEnableMask()
{
	using enum VertexLayoutAttribMask;
	if constexpr(requires {V::pos; V::texCoord; V::color;})
		return Position | TextureCoordinate | Color;
	else if constexpr(requires {V::pos; V::color;})
		return Position | Color;
	else if constexpr(requires {V::pos; V::texCoord;})
		return Position | TextureCoordinate;
	else
		return Position;
}

template <VertexLayout V>
constexpr VertexLayoutAttribMask vertexLayoutIntNormalizeMask()
{
	using enum VertexLayoutAttribMask;
	if constexpr(requires {V::intNormalizeMask;})
		return V::intNormalizeMask;
	else
		return TextureCoordinate | Color;
}

template <VertexLayout V>
constexpr bool shouldNormalize(AttribType type, VertexLayoutAttribMask attribMask)
{
	return type != AttribType::Float && to_underlying(vertexLayoutIntNormalizeMask<V>() & attribMask);
}

template <VertexLayout V>
constexpr AttribDesc posAttribDesc()
{
	using T = decltype(V::pos.x);
	auto type = attribType<T>;
	return {offsetof(V, pos), sizeof(V::pos) / sizeof(T), type, shouldNormalize<V>(type, VertexLayoutAttribMask::Position)};
}

template <VertexLayout V>
constexpr AttribDesc colorAttribDesc()
{
	if constexpr(requires {V::color;})
	{
		using T = decltype(V::color.r);
		auto type = attribType<T>;
		return {offsetof(V, color), sizeof(V::color) / sizeof(T), type, shouldNormalize<V>(type, VertexLayoutAttribMask::Color)};
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
		auto type = attribType<T>;
		return {offsetof(V, texCoord), sizeof(V::texCoord) / sizeof(T), type, shouldNormalize<V>(type, VertexLayoutAttribMask::TextureCoordinate)};
	}
	else
	{
		return {};
	}
}

struct Vertex2F
{
	glm::vec2 pos;
};

struct Vertex2FColI
{
	glm::vec2 pos;
	PackedColor color;
};

struct Vertex2FTexF
{
	glm::vec2 pos;
	glm::vec2 texCoord;
};

struct Vertex2FTexFColI
{
	glm::vec2 pos;
	glm::vec2 texCoord;
	PackedColor color;
};

struct Vertex2I
{
	glm::i16vec2 pos;
};

struct Vertex2IColI
{
	glm::i16vec2 pos;
	PackedColor color;
};

struct Vertex2ITexI
{
	glm::i16vec2 pos;
	glm::i16vec2 texCoord;
};

struct Vertex2ITexIColI
{
	glm::i16vec2 pos;
	glm::i16vec2 texCoord;
	PackedColor color;
};

}
