#pragma once

#include <engine-globals.h>
#include <util/number.h>
#include <util/Matrix4x4.hh>
#include <util/Point2D.hh>
#include <gfx/common/TextureSizeSupport.hh>

#ifdef CONFIG_GFX_OPENGL
#include <gfx/opengl/gfx-globals.hh>
#endif

#ifndef CONFIG_BASE_ANDROID
#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

typedef TransformCoordinate GC;
typedef TransformCoordinate Coordinate;
typedef TextureCoordinate GTexC;

typedef struct IG::Point2D<GC> GfxPoint;

namespace Gfx
{

typedef GfxPoint GP;

template <class T>
static GTexC pixelToTexC(T pixel, T total) { return (GTexC)pixel / (GTexC)total; }

extern TextureSizeSupport textureSizeSupport;

#if defined CONFIG_BASE_IOS
typedef double FrameTimeBase;

constexpr static double decimalFrameTimeBaseFromSec(double sec)
{
	return sec;
}

constexpr static FrameTimeBase frameTimeBaseFromSec(double sec)
{
	return sec;
}
#else
typedef int64 FrameTimeBase;

constexpr static double decimalFrameTimeBaseFromSec(double sec)
{
	return sec * (double)1000000000.;
}

constexpr static FrameTimeBase frameTimeBaseFromSec(double sec)
{
	return decimalFrameTimeBaseFromSec(sec);
}
#endif

}
