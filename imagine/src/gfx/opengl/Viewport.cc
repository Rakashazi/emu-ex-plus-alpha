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

#include <imagine/gfx/Gfx.hh>
#include <imagine/util/math/Point2D.hh>
#include "private.hh"

namespace Gfx
{

static Viewport currViewport;
static int discardFrameBuffer = 0;

TextureSizeSupport textureSizeSupport;

void setViewport(const Viewport &v)
{
	auto inGLFormat = v.inGLFormat();
	//logMsg("set GL viewport %d:%d:%d:%d", inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	assert(inGLFormat.x2 && inGLFormat.y2);
	glViewport(inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	currViewport = v;
}

const Viewport &viewport()
{
	return currViewport;
}

Viewport Viewport::makeFromRect(const IG::WindowRect &fullRect, const IG::WindowRect &fullRealRect, const IG::WindowRect &rect)
{
	Viewport v;
	v.rect = rect;
	v.w = rect.xSize();
	v.h = rect.ySize();
	float wScaler = v.w / (float)fullRect.xSize();
	float hScaler = v.h / (float)fullRect.ySize();
	v.wMM = 1;
	v.hMM = 1;
	#ifdef __ANDROID__
	v.wSMM = 1;
	v.hSMM = 1;
	#endif
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	v.softOrientation_ = 0;
	#endif
	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {v.realBounds().x, fullRealRect.ySize() - v.realBounds().y2, v.realWidth(), v.realHeight()};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

Viewport Viewport::makeFromWindow(const Base::Window &win, const IG::WindowRect &rect)
{
	Viewport v;
	v.rect = rect;
	v.w = rect.xSize();
	v.h = rect.ySize();
	float wScaler = v.w / (float)win.width();
	float hScaler = v.h / (float)win.height();
	v.wMM = win.widthMM() * wScaler;
	v.hMM = win.heightMM() * hScaler;
	#ifdef __ANDROID__
	v.wSMM = win.widthSMM() * wScaler;
	v.hSMM = win.heightSMM() * hScaler;
	#endif
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	v.softOrientation_ = win.softOrientation();
	#endif
	//logMsg("made viewport %d:%d:%d:%d from window %d:%d",
	//	v.rect.x, v.rect.y, v.rect.x2, v.rect.y2,
	//	win.width(), win.height());

	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {v.realBounds().x, win.realHeight() - v.realBounds().y2, v.realWidth(), v.realHeight()};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

IG::Point2D<int> Viewport::sizesWithRatioBestFitFromViewport(float destAspectRatio) const
{
	return IG::sizesWithRatioBestFit(destAspectRatio, (int)width(), (int)height());
}

}
