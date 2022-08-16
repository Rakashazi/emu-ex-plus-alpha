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
#include <imagine/base/Viewport.hh>
#include <imagine/util/Point2D.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <optional>
#include <stdexcept>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/gfx-globals.hh>
#endif

namespace IG::Gfx
{

class RendererTask;
class RendererCommands;
class SyncFence;
class Texture;

using GCRect = IG::CoordinateRect<float, true, true>;

static GCRect makeGCRectRel(FP p, FP size)
{
	return GCRect::makeRel(p, size);
}

enum class WrapMode: uint8_t { REPEAT, CLAMP };

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

enum class CommonTextureSampler: uint8_t
{
	CLAMP,
	NEAREST_MIP_CLAMP,
	NO_MIP_CLAMP,
	NO_LINEAR_NO_MIP_CLAMP,
	REPEAT,
	NEAREST_MIP_REPEAT
};

enum class TextureType : uint8_t
{
	UNSET, T2D_1, T2D_2, T2D_4, T2D_EXTERNAL
};

class TextureSpan
{
public:
	constexpr TextureSpan(const Texture *tex = {}, FRect uv = {{}, {1., 1.}}):
		tex{tex}, uv{uv}
	{}
	constexpr const Texture *texture() const { return tex; }
	constexpr auto uvBounds() const { return uv; }
	explicit operator bool() const;

protected:
	const Texture *tex;
	FRect uv;
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

constexpr Color color(float r, float g, float b, float a = 1.f)
{
	if constexpr(std::is_floating_point_v<ColorComp>)
	{
		return {(ColorComp)r, (ColorComp)g, (ColorComp)b, (ColorComp)a};
	}
	else
	{
		return {255.f * r, 255.f * g, 255.f * b, 255.f * a};
	}
}

constexpr Color color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	if constexpr(std::is_floating_point_v<ColorComp>)
	{
		return {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
	}
	else
	{
		return {(ColorComp)r, (ColorComp)g, (ColorComp)b, (ColorComp)a};
	}
}

constexpr Color color(ColorName c)
{
	switch(c)
	{
		case ColorName::RED: return color(1.f, 0.f, 0.f);
		case ColorName::GREEN: return color(0.f, 1.f, 0.f);
		case ColorName::BLUE: return color(0.f, 0.f, 1.f);
		case ColorName::CYAN: return color(0.f, 1.f, 1.f);
		case ColorName::YELLOW: return color(1.f, 1.f, 0.f);
		case ColorName::MAGENTA: return color(1.f, 0.f, 1.f);
		case ColorName::WHITE: return color(1.f, 1.f, 1.f);
		case ColorName::BLACK: return color(0.f, 0.f, 0.f);
		default: return color(0.f, 0.f, 0.f, 0.f);
	}
}

// converts to a relative rectangle in OpenGL coordinate system
constexpr Rect2<int> asYUpRelRect(Viewport v)
{
	return {{v.realBounds().x, v.realOriginBounds().ySize() - v.realBounds().y2}, {v.realWidth(), v.realHeight()}};
}

}
