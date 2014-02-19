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

#include <engine-globals.h>
#include <util/number.h>
#include <util/Point2D.hh>
#include <util/rectangle2.h>
#include <gfx/common/TextureSizeSupport.hh>

#ifdef CONFIG_GFX_OPENGL
#include <gfx/opengl/gfx-globals.hh>
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

using GCRect = IG::CoordinateRect<GC, true, true>;

static GCRect makeGCRectRel(GP p, GP size)
{
	return GCRect::makeRel(p.x, p.y, size.x, size.y);
}

template <class T>
static GTexC pixelToTexC(T pixel, T total) { return (GTexC)pixel / (GTexC)total; }

extern TextureSizeSupport textureSizeSupport;

}
