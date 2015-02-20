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

#include <imagine/engine-globals.h>
#include <imagine/util/number.h>
#include <imagine/util/Point2D.hh>
#include <imagine/util/rectangle2.h>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/gfx-globals.hh>
#endif

namespace Gfx
{

using GC = TransformCoordinate;
using Coordinate = TransformCoordinate;
using GTexC = TextureCoordinate;
using GfxPoint = IG::Point2D<GC>;
using GP = GfxPoint;

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

using GCRect = IG::CoordinateRect<GC, true, true>;

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

}
