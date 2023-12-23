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

#include <imagine/util/utility.h>
#include <imagine/util/math.hh>
#include <imagine/util/enum.hh>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>

// Origins are based on Cartesian coordinates,
// min:negative x = left
// max:positive x = right
// min:negative y = bottom
// max:positive y = top

namespace IG
{

WISE_ENUM_CLASS((Origin, uint8_t),
	center,
	min,
	max,
	centerInverted,
	minInverted,
	maxInverted);

class _2DOrigin
{
public:
	using PackedType = uint8_t;

	Origin x{}, y{};

	constexpr _2DOrigin() = default;
	constexpr _2DOrigin(Origin x, Origin y): x{x}, y{y} {}

	static constexpr _2DOrigin unpack(PackedType val)
	{
		int maxVal = to_underlying(lastEnum<Origin>);
		return {Origin(std::min(val & 0xF, maxVal)), Origin(std::min(val >> 4, maxVal))};
	}

	constexpr PackedType pack() const
	{
		return to_underlying(x) | (to_underlying(y) << 4);
	}

	static constexpr int scaler(Origin originValue)
	{
		switch(originValue)
		{
			case Origin::min:
			case Origin::minInverted: return -1;
			case Origin::center:
			case Origin::centerInverted: return 0;
			case Origin::max:
			case Origin::maxInverted: return 1;
			default: bug_unreachable("invalid Origin");
		}
	}

	constexpr int xScaler() const
	{
		return scaler(x);
	}

	constexpr int yScaler() const
	{
		return scaler(y);
	}

	constexpr bool isYCartesian() const
	{
		return isCartesian(y);
	}

	static constexpr int isCartesian(Origin type)
	{
		return type == Origin::min || type ==Origin::max || type ==Origin::center;
	}

	constexpr bool isXCentered() { return scaler(x) == 0; }
	constexpr bool onYCenter() { return scaler(y) == 0; }
	constexpr bool onRight() { return scaler(x) == 1; }
	constexpr bool onLeft() { return scaler(x) == -1; }
	constexpr bool onTop() { return scaler(y) == 1; }
	constexpr bool onBottom() { return scaler(y) == -1; }

	static constexpr uint8_t inverted(Origin inputType, Origin outputType)
	{
		int inputInverted;
		if(isCartesian(inputType))
			inputInverted = 0;
		else inputInverted = 1;

		int outputInverted;
		if(isCartesian(outputType))
			outputInverted = 0;
		else outputInverted = 1;

		return lxor(inputInverted, outputInverted);
	}

	constexpr uint8_t xInverted(_2DOrigin outputType) const
	{
		return inverted(x, outputType.x);
	}

	constexpr uint8_t yInverted(_2DOrigin outputType) const
	{
		return inverted(y, outputType.y);
	}

	static constexpr Origin invert(Origin originValue)
	{
		switch(originValue)
		{
			case Origin::min: return Origin::maxInverted;
			case Origin::minInverted: return Origin::max;
			case Origin::center: return Origin::centerInverted;
			case Origin::centerInverted: return Origin::center;
			case Origin::max: return Origin::minInverted;
			case Origin::maxInverted: return Origin::min;
			default: bug_unreachable("invalid Origin");
		}
	}

	constexpr _2DOrigin invertX() const { return {invert(x), y}; }

	constexpr _2DOrigin invertY() const { return {x, invert(y)}; }

	constexpr _2DOrigin invertYIfCartesian() const
	{
		if(isCartesian(y))
		{
			return invertY();
		}
		return *this;
	}

	static constexpr Origin flip(Origin originValue)
	{
		switch(originValue)
		{
			case Origin::min: return Origin::max;
			case Origin::minInverted: return Origin::maxInverted;
			case Origin::center: return Origin::center;
			case Origin::centerInverted: return Origin::centerInverted;
			case Origin::max: return Origin::min;
			case Origin::maxInverted: return Origin::minInverted;
			default: bug_unreachable("invalid Origin");
		}
	}

	constexpr _2DOrigin flipX() const { return {flip(x), y}; }

	constexpr _2DOrigin flipY() const { return {x, flip(y)}; }

	static constexpr auto adjust(auto pos, auto halfSize, auto fullSize, int inputScale, int outputScale)
	{
		int scaleDiff = inputScale - outputScale;
		switch(std::abs(scaleDiff))
		{
			case 1: return pos - halfSize * sign(scaleDiff);
			case 2: return pos - fullSize * sign(scaleDiff);
		}
		return pos;
	}

	constexpr auto adjustX(auto pos, auto halfSize, auto fullSize, _2DOrigin outputType) const
	{
		pos = xInverted(outputType) ? (fullSize) - pos : pos;
		return adjust(pos, halfSize, fullSize, xScaler(), outputType.xScaler());
	}

	constexpr auto adjustX(auto pos, auto fullSize, _2DOrigin outputType) const
	{
		return adjustX(pos, fullSize / (decltype(fullSize))2, fullSize, outputType);
	}

	constexpr auto adjustY(auto pos, auto halfSize, auto fullSize, _2DOrigin outputType) const
	{
		pos = yInverted(outputType) ? (fullSize) - pos : pos;
		return adjust(pos, halfSize, fullSize, yScaler(), outputType.yScaler());
	}

	constexpr auto adjustY(auto pos, auto fullSize, _2DOrigin outputType) const
	{
		return adjustY(pos, fullSize / (decltype(fullSize))2, fullSize, outputType);
	}

	constexpr bool operator==(_2DOrigin const &rhs) const = default;

	static constexpr int lxor(int a, int b)
	{
		return !a != !b;
	}
};

// cartesian origin shortcuts sorted clockwise
constexpr _2DOrigin CT2DO(Origin::center, Origin::max);
constexpr _2DOrigin RT2DO(Origin::max, Origin::max);
constexpr _2DOrigin RC2DO(Origin::max, Origin::center);
constexpr _2DOrigin RB2DO(Origin::max, Origin::min);
constexpr _2DOrigin CB2DO(Origin::center, Origin::min);
constexpr _2DOrigin LB2DO(Origin::min, Origin::min);
constexpr _2DOrigin LC2DO(Origin::min, Origin::center);
constexpr _2DOrigin LT2DO(Origin::min, Origin::max);
constexpr _2DOrigin C2DO(Origin::center, Origin::center);

}
