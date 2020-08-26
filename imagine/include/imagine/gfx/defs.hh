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
#include <imagine/time/Time.hh>
#include <imagine/base/Error.hh>
#include <imagine/util/Point2D.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/gfx-globals.hh>
#endif

namespace Gfx
{

using namespace IG;

class RendererTask;
class RendererDrawTask;
class SyncFence;
class Texture;

using GC = TransformCoordinate;
using Coordinate = TransformCoordinate;
using GfxPoint = IG::Point2D<GC>;
using GP = GfxPoint;
using GCRect = IG::CoordinateRect<GC, true, true>;
using GTexC = TextureCoordinate;
using GTexCPoint = IG::Point2D<GTexC>;;
using Error = std::optional<std::runtime_error>;
using DrawDelegate = DelegateFunc<void(Drawable drawable, Base::Window &win, SyncFence fence, RendererDrawTask task)>;
using RenderTaskFuncDelegate = DelegateFunc<void(RendererTask &task)>;

static constexpr GC operator"" _gc (long double n)
{
	return (GC)n;
}

static constexpr GC operator"" _gc (unsigned long long n)
{
	return (GC)n;
}

static constexpr GC operator"" _gtexc (long double n)
{
	return (GTexC)n;
}

static constexpr GC operator"" _gtexc (unsigned long long n)
{
	return (GTexC)n;
}

static GCRect makeGCRectRel(GP p, GP size)
{
	return GCRect::makeRel(p.x, p.y, size.x, size.y);
}

template <class T>
static GTexC pixelToTexC(T pixel, T total) { return (GTexC)pixel / (GTexC)total; }

enum WrapMode
{
	WRAP_REPEAT,
	WRAP_CLAMP
};

enum MipFilterMode
{
	MIP_FILTER_NONE,
	MIP_FILTER_NEAREST,
	MIP_FILTER_LINEAR,
};

enum { BLEND_MODE_OFF = 0, BLEND_MODE_ALPHA, BLEND_MODE_INTENSITY };

enum { IMG_MODE_MODULATE = 0, IMG_MODE_BLEND, IMG_MODE_REPLACE, IMG_MODE_ADD };

enum { BLEND_EQ_ADD, BLEND_EQ_SUB, BLEND_EQ_RSUB };

enum { BOTH_FACES, FRONT_FACES, BACK_FACES };

enum GfxColorEnum { COLOR_WHITE, COLOR_BLACK };

enum TransformTargetEnum { TARGET_WORLD, TARGET_TEXTURE };

enum class CommonProgram
{
	// color replacement shaders
	TEX_REPLACE,
	TEX_ALPHA_REPLACE,
	#ifdef __ANDROID__
	TEX_EXTERNAL_REPLACE,
	#endif

	// color modulation shaders
	TEX,
	TEX_ALPHA,
	#ifdef __ANDROID__
	TEX_EXTERNAL,
	#endif
	NO_TEX
};

enum class CommonTextureSampler
{
	CLAMP,
	NEAREST_MIP_CLAMP,
	NO_MIP_CLAMP,
	NO_LINEAR_NO_MIP_CLAMP,
	REPEAT,
	NEAREST_MIP_REPEAT
};

using TextString = std::u16string;
using TextStringView = std::u16string_view;

class TextureSpan
{
public:
	constexpr TextureSpan(const Texture *tex = {}, IG::Rect2<GTexC> uv = {0., 0., 1., 1.}):
		tex{tex}, uv{uv}
	{}
	const Texture *texture() const { return tex; }
	IG::Rect2<GTexC> uvBounds() const { return uv; }
	explicit operator bool() const { return tex; }

protected:
	const Texture *tex;
	IG::Rect2<GTexC> uv;
};

enum class TextureBufferMode : uint8_t
{
	DEFAULT,
	SYSTEM_MEMORY,
	ANDROID_HARDWARE_BUFFER,
	ANDROID_SURFACE_TEXTURE,
	PBO,
};

}
