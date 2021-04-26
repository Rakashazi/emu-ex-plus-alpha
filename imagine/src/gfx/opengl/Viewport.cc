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

#include <imagine/gfx/Viewport.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/math/Point2D.hh>

namespace Gfx
{

IG::WindowRect Viewport::relRectFromViewport(int newX, int newY, int xSize, int ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	// adjust to the requested origin on the screen
	//logMsg("relRectFromViewport %d,%d %d,%d", newX, newY, xSize, ySize);
	newX = LT2DO.adjustX(newX, (int)width(), screenOrigin.invertYIfCartesian());
	newY = LT2DO.adjustY(newY, (int)height(), screenOrigin.invertYIfCartesian());
	//logMsg("translated to %d,%d", newX, newY);
	IG::WindowRect rect;
	rect.setPosRel({newX, newY}, {xSize, ySize}, posOrigin);
	return rect;
}

IG::WindowRect Viewport::relRectFromViewport(int newX, int newY, int size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	return relRectFromViewport(newX, newY, size, size, posOrigin, screenOrigin);
}

IG::WindowRect Viewport::relRectFromViewport(int newX, int newY, IG::Point2D<int> size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	return relRectFromViewport(newX, newY, size.x, size.y, posOrigin, screenOrigin);
}

IG::WindowRect Viewport::rectWithRatioBestFitFromViewport(int newX, int newY, float aR, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
{
	return relRectFromViewport(newX, newY, sizesWithRatioBestFitFromViewport(aR), posOrigin, screenOrigin);
}

Viewport Viewport::makeFromRect(IG::WindowRect fullRect, IG::WindowRect fullRealRect, IG::WindowRect rect)
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
	v.softOrientation_ = 0;
	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {{v.realBounds().x, fullRealRect.ySize() - v.realBounds().y2}, {v.realWidth(), v.realHeight()}};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

Viewport Viewport::makeFromWindow(const Base::Window &win, IG::WindowRect rect)
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
	v.softOrientation_ = win.softOrientation();
	//logMsg("made viewport %d:%d:%d:%d from window %d:%d",
	//	v.rect.x, v.rect.y, v.rect.x2, v.rect.y2,
	//	win.width(), win.height());

	// glViewport() needs flipped Y and relative size
	v.relYFlipViewport = {{v.realBounds().x, win.realHeight() - v.realBounds().y2}, {v.realWidth(), v.realHeight()}};
	//logMsg("transformed for GL %d:%d:%d:%d", v.relYFlipViewport.x, v.relYFlipViewport.y, v.relYFlipViewport.x2, v.relYFlipViewport.y2);
	return v;
}

Viewport Viewport::makeFromWindow(const Base::Window &win)
{
	return makeFromWindow(win, win.contentBounds());
}

IG::Point2D<int> Viewport::sizesWithRatioBestFitFromViewport(float destAspectRatio) const
{
	return IG::sizesWithRatioBestFit(destAspectRatio, (int)width(), (int)height());
}

}
