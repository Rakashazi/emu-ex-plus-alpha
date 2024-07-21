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
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/rectangle2.h>
#include <array>

#ifndef CONFIG_GFX_OPENGL
#define CONFIG_GFX_OPENGL 1
#endif

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/defs.hh>
#endif

namespace IG::Gfx
{

class Renderer;
class RendererTask;
class RendererCommands;
class SyncFence;
struct TextureBufferFlags;
struct TextureWriteFlags;
class TextureConfig;
class Texture;
struct TextureBinding;
class PixmapBufferTexture;
class TextureSamplerConfig;
class TextureSampler;
class Mat4;
class Vec3;
class Vec4;
class Shader;
class Program;
class BasicEffect;
class GlyphTextureSet;
class ProjectionPlane;
struct DrawableConfig;
struct Color4B;
enum class BufferType : uint8_t;
template<class T, BufferType type>
class Buffer;
template<class T>
class ObjectVertexBuffer;
template<class T>
class ObjectVertexArray;
template<class T>
class QuadIndexArray;

using GCRect = CoordinateRect<float, true, true>;

enum class WrapMode: uint8_t { REPEAT, MIRROR_REPEAT, CLAMP };

enum class MipFilter: uint8_t { NONE, NEAREST, LINEAR };

enum class BlendMode: uint8_t { OFF, ALPHA, PREMULT_ALPHA, INTENSITY };

enum class EnvMode: uint8_t { MODULATE, BLEND, REPLACE, ADD };

enum class BlendEquation: uint8_t { ADD, SUB, RSUB };

enum class Faces: uint8_t { BOTH, FRONT, BACK };

enum class ColorName: uint8_t
{
	RED,
	GREEN,
	BLUE,
	CYAN,
	YELLOW,
	MAGENTA,
	WHITE,
	BLACK
};

enum class TextureType : uint8_t
{
	UNSET, T2D_1, T2D_2, T2D_4, T2D_EXTERNAL
};

class TextureSpan
{
public:
	const Texture *texturePtr{};
	FRect bounds{{}, {1.f, 1.f}};

	explicit operator bool() const;
	operator TextureBinding() const;
};

enum class TextureBufferMode : uint8_t
{
	DEFAULT,
	SYSTEM_MEMORY,
	ANDROID_HARDWARE_BUFFER,
	ANDROID_SURFACE_TEXTURE,
	PBO,
};

enum class DrawAsyncMode : uint8_t
{
	AUTO, NONE, PRESENT, FULL
};

struct DrawParams
{
	DrawAsyncMode asyncMode{DrawAsyncMode::AUTO};
};

struct Color4F
{
	union
	{
		std::array<float, 4> rgba{};
		struct
		{
			float r, g, b, a;
		};
	};

	constexpr Color4F() = default;
	constexpr Color4F(float r, float g, float b, float a = 1.f):
		r{r}, g{g}, b{b}, a{a} {}
	constexpr Color4F(std::array<float, 4> rgba): rgba{rgba} {}
	constexpr Color4F(float i): Color4F{i, i, i, i} {}

	constexpr Color4F(ColorName c):
		rgba
		{
			[&] -> std::array<float, 4>
			{
				switch(c)
				{
					case ColorName::RED:     return {1.f, 0.f, 0.f, 1.f};
					case ColorName::GREEN:   return {0.f, 1.f, 0.f, 1.f};
					case ColorName::BLUE:    return {0.f, 0.f, 1.f, 1.f};
					case ColorName::CYAN:    return {0.f, 1.f, 1.f, 1.f};
					case ColorName::YELLOW:  return {1.f, 1.f, 0.f, 1.f};
					case ColorName::MAGENTA: return {1.f, 0.f, 1.f, 1.f};
					case ColorName::WHITE:   return {1.f, 1.f, 1.f, 1.f};
					case ColorName::BLACK:   return {0.f, 0.f, 0.f, 1.f};
				}
				return {};
			}()
		} {}

	[[nodiscard]]
	constexpr Color4F multiplyAlpha() const { return {r * a, g * a, b * a, a}; }
	[[nodiscard]]
	constexpr Color4F multiplyRGB(float l) const { return {r * l, g * l, b * l, a}; }
	constexpr operator Color4B() const;
	constexpr operator std::array<float, 4>() const { return rgba; }
	constexpr bool operator ==(Color4F const &rhs) const { return rgba == rhs.rgba; }
};

struct Color4B
{
	union
	{
		uint32_t rgba{};
		struct
		{
			uint8_t r, g, b, a;
		};
	};
	static constexpr auto format = PixelDescRGBA8888Native;

	constexpr Color4B() = default;
	constexpr Color4B(uint32_t rgba): rgba{rgba} {}

	constexpr Color4B(ColorName c):
		rgba
		{
			[&] -> uint32_t
			{
				switch(c)
				{
					case ColorName::RED:     return format.build(1.f, 0.f, 0.f);
					case ColorName::GREEN:   return format.build(0.f, 1.f, 0.f);
					case ColorName::BLUE:    return format.build(0.f, 0.f, 1.f);
					case ColorName::CYAN:    return format.build(0.f, 1.f, 1.f);
					case ColorName::YELLOW:  return format.build(1.f, 1.f, 0.f);
					case ColorName::MAGENTA: return format.build(1.f, 0.f, 1.f);
					case ColorName::WHITE:   return format.build(1.f, 1.f, 1.f);
					case ColorName::BLACK:   return format.build(0.f, 0.f, 0.f);
				}
				return {};
			}()
		} {}

	constexpr operator Color4F() const { return format.rgbaNorm(rgba); }
	constexpr operator uint32_t() const { return rgba; }
	constexpr bool operator ==(Color4B const &rhs) const { return rgba == rhs.rgba; }
};

constexpr Color4F::operator Color4B() const { return Color4B::format.build(r, g, b, a); }

using PackedColor = Color4B;
using Color = Color4F;

enum class AttribType : uint8_t
{
	UByte = 1,
	Short,
	UShort,
	Float,
};

struct AttribDesc
{
	size_t offset{};
	int size{};
	AttribType type{};
	bool normalize{};

	constexpr bool operator==(AttribDesc const&) const = default;
};

constexpr bool supportsPresentModes = Config::envIsLinux || Config::envIsAndroid;
constexpr bool supportsPresentationTime = Config::envIsAndroid;

struct GlyphSetMetrics
{
	int16_t nominalHeight{};
	int16_t spaceSize{};
	int16_t yLineStart{};
};

enum class BufferType : uint8_t
{
	vertex,
	index,
};

enum class BufferMapMode
{
	unset, direct, indirect
};

}
