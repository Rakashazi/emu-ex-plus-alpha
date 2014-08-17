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
#include <imagine/gfx/defs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/operators.hh>
#include <imagine/base/Window.hh>

namespace Gfx
{

class Viewport : public NotEquals<Viewport>
{
private:
	IG::WindowRect rect;
	int w = 0, h = 0;
	float wMM = 0, hMM = 0;
	#ifdef __ANDROID__
	float wSMM = 0, hSMM = 0;
	#endif
	IG::Rect2<int> relYFlipViewport;
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint softOrientation_ = Base::VIEW_ROTATE_0;
	#else
	static constexpr uint softOrientation_ = Base::VIEW_ROTATE_0;
	#endif

public:

	constexpr Viewport() {}
	IG::WindowRect realBounds() const { return Base::orientationIsSideways(softOrientation_) ? IG::WindowRect{rect.y, rect.x, rect.y2, rect.x2} : rect; }
	IG::WindowRect bounds() const { return rect; }
	int realWidth() const { return Base::orientationIsSideways(softOrientation_) ? h : w; }
	int realHeight() const { return Base::orientationIsSideways(softOrientation_) ? w : h; }
	int width() const { return w; }
	int height() const { return h; }
	float aspectRatio() const { return (float)width() / (float)height(); }
	float realAspectRatio() const { return (float)realWidth() / (float)realHeight(); }
	float widthMM() const { return wMM; }
	float heightMM() const { return hMM; }
	#ifdef __ANDROID__
	float widthSMM() const { return wSMM; }
	float heightSMM() const { return hSMM; }
	#else
	float widthSMM() const { return widthMM(); }
	float heightSMM() const { return heightMM(); }
	#endif

	bool isPortrait() const
	{
		return width() < height();
	}

	IG::WindowRect relRectFromViewport(int newX, int newY, int xSize, int ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
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

	IG::WindowRect relRectFromViewport(int newX, int newY, int size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
	{
		return relRectFromViewport(newX, newY, size, size, posOrigin, screenOrigin);
	}

	IG::WindowRect relRectFromViewport(int newX, int newY, IG::Point2D<int> size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
	{
		return relRectFromViewport(newX, newY, size.x, size.y, posOrigin, screenOrigin);
	}

	IG::Point2D<int> sizesWithRatioBestFitFromViewport(float destAspectRatio) const
	{
		return IG::sizesWithRatioBestFit(destAspectRatio, (int)width(), (int)height());
	}

	IG::WindowRect rectWithRatioBestFitFromViewport(int newX, int newY, float aR, _2DOrigin posOrigin, _2DOrigin screenOrigin) const
	{
		return relRectFromViewport(newX, newY, sizesWithRatioBestFitFromViewport(aR), posOrigin, screenOrigin);
	}

	IG::Rect2<int> inGLFormat() const
	{
		return relYFlipViewport;
	}

	static Viewport makeFromWindow(const Base::Window &win, const IG::WindowRect &rect);
	static Viewport makeFromWindow(const Base::Window &win)
	{
		return makeFromWindow(win, win.contentBounds());
	}

	static Viewport makeFromRect(const IG::WindowRect &fullRect, const IG::WindowRect &fullRealRect, const IG::WindowRect &subRect);

	static Viewport makeFromRect(const IG::WindowRect &fullRect, const IG::WindowRect &subRect)
	{
		return makeFromRect(fullRect, fullRect, subRect);
	}

	static Viewport makeFromRect(const IG::WindowRect &fullRect)
	{
		return makeFromRect(fullRect, fullRect, fullRect);
	}

	bool operator ==(Viewport const& rhs) const
	{
		return rect == rhs.rect;
	}
};

}
