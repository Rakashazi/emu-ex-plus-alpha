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
#include <imagine/gfx/defs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/typeTraits.hh>
#include <compare>

namespace Base
{

class Window;

}

namespace Gfx
{

class Viewport
{
public:
	constexpr Viewport() {}
	IG::WindowRect realBounds() const { return Base::orientationIsSideways(softOrientation_) ? rect.makeInverted() : rect; }
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

	IG::Rect2<int> inGLFormat() const
	{
		return relYFlipViewport;
	}

	IG::WindowRect relRectFromViewport(int newX, int newY, int xSize, int ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	IG::WindowRect relRectFromViewport(int newX, int newY, int size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	IG::WindowRect relRectFromViewport(int newX, int newY, IG::Point2D<int> size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	IG::WindowRect rectWithRatioBestFitFromViewport(int newX, int newY, float aR, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	static Viewport makeFromRect(IG::WindowRect fullRect, IG::WindowRect fullRealRect, IG::WindowRect subRect);
	static Viewport makeFromWindow(const Base::Window &win, IG::WindowRect rect);
	static Viewport makeFromWindow(const Base::Window &win);
	IG::Point2D<int> sizesWithRatioBestFitFromViewport(float destAspectRatio) const;

	static Viewport makeFromRect(IG::WindowRect fullRect, IG::WindowRect subRect)
	{
		return makeFromRect(fullRect, fullRect, subRect);
	}

	static Viewport makeFromRect(IG::WindowRect fullRect)
	{
		return makeFromRect(fullRect, fullRect, fullRect);
	}

	bool operator ==(Viewport const& rhs) const
	{
		return rect == rhs.rect;
	}

private:
	IG::WindowRect rect;
	int w = 0, h = 0;
	float wMM = 0, hMM = 0;
	#ifdef __ANDROID__
	float wSMM = 0, hSMM = 0;
	#endif
	IG::Rect2<int> relYFlipViewport;
	IG_enableMemberIfOrConstant(!Config::SYSTEM_ROTATES_WINDOWS, Base::Orientation,
		Base::VIEW_ROTATE_0, softOrientation_){Base::VIEW_ROTATE_0};
};

}
