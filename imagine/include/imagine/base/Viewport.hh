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
#include <imagine/base/baseDefs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/used.hh>
#include <imagine/util/2DOrigin.h>

namespace IG
{

class Viewport
{
public:
	constexpr Viewport() = default;
	constexpr Viewport(WRect originRect, WRect rect, Rotation softOrientation = Rotation::UP):
		originRect{originRect}, rect{rect}, softRotation_{softOrientation} {}
	constexpr Viewport(WRect rect, Rotation softOrientation = Rotation::UP):
		Viewport{rect, rect, softOrientation} {}
	constexpr Viewport(WSize size):
		Viewport{{{}, size}} {}
	constexpr WRect realBounds() const { return isSideways() ? bounds().makeInverted() : bounds(); }
	constexpr WRect bounds() const { return rect; }
	constexpr WRect realOriginBounds() const { return isSideways() ? originBounds().makeInverted() : originBounds(); }
	constexpr WRect originBounds() const { return originRect; }
	constexpr int realWidth() const { return isSideways() ? height() : width(); }
	constexpr int realHeight() const { return isSideways() ? width() : height(); }
	constexpr int width() const { return rect.xSize(); }
	constexpr int height() const { return rect.ySize(); }
	constexpr float aspectRatio() const { return (float)width() / (float)height(); }
	constexpr float realAspectRatio() const { return (float)realWidth() / (float)realHeight(); }
	constexpr bool isPortrait() const { return width() < height(); }
	constexpr bool isSideways() const { return IG::isSideways(softRotation_); }
	constexpr bool operator==(Viewport const &) const = default;
	WRect relRect(WPt pos, WSize size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	WRect relRectBestFit(WPt pos, float aspectRatio, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;

	// converts to a relative rectangle in OpenGL coordinate system
	constexpr Rect2<int> asYUpRelRect() const
	{
		return {{realBounds().x, realOriginBounds().ySize() - realBounds().y2}, {realWidth(), realHeight()}};
	}

private:
	WRect originRect{};
	WRect rect{};
	ConditionalMemberOr<!Config::SYSTEM_ROTATES_WINDOWS, Rotation, Rotation::UP> softRotation_{Rotation::UP};
};

}
