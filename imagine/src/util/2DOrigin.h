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

#include <cmath>
#include <cstdlib>
#include <util/memory.h>
#include <util/branch.h>

// Origins are based on Cartesian coordinates,
// min:negative x = left
// max:positive x = right
// min:negative y = bottom
// max:positive y = top

enum { _2DORIGIN_NONE = 0, _2DORIGIN_MIN, _2DORIGIN_MIN_INVERSE_CARTESIAN, _2DORIGIN_CENTER, _2DORIGIN_CENTER_INVERSE_CARTESIAN, _2DORIGIN_MAX, _2DORIGIN_MAX_INVERSE_CARTESIAN };
class _2DOrigin
{
public:
	uchar x = _2DORIGIN_NONE, y = _2DORIGIN_NONE;
	constexpr _2DOrigin() { }
	constexpr _2DOrigin(uchar x, uchar y): x(x & 7), y(y & 7) { }
	constexpr _2DOrigin(uchar val): x(val & 7), y(val >> 3) { }

	static const char *toString(uint value)
	{
		switch(value)
		{
			case _2DORIGIN_MIN: return "Min";
			case _2DORIGIN_MIN_INVERSE_CARTESIAN: return "Min (Inverted)";
			case _2DORIGIN_CENTER: return "Center";
			case _2DORIGIN_CENTER_INVERSE_CARTESIAN: return "Center (Inverted)";
			case _2DORIGIN_MAX: return "Max";
			case _2DORIGIN_MAX_INVERSE_CARTESIAN: return "Max (Inverted)";
			default: bug_branch("%d", value); return 0;
		}
	}

	static int scaler(uint originValue)
	{
		switch(originValue)
		{
			case _2DORIGIN_MIN:
			case _2DORIGIN_MIN_INVERSE_CARTESIAN: return -1;
			case _2DORIGIN_CENTER:
			case _2DORIGIN_CENTER_INVERSE_CARTESIAN: return 0;
			case _2DORIGIN_MAX:
			case _2DORIGIN_MAX_INVERSE_CARTESIAN: return 1;
			default: bug_branch("%d", originValue); return 0;
		}
	}

	int xScaler() const
	{
		return scaler(x);
	}

	int yScaler() const
	{
		return scaler(y);
	}

	bool isYCartesian() const
	{
		return isCartesian(y);
	}

	static int isCartesian(int type)
	{
		return type == _2DORIGIN_MIN || type ==_2DORIGIN_MAX || type ==_2DORIGIN_CENTER;
	}

	static bool valIsValid(uint originValue)
	{
		switch(originValue)
		{
			case 0:
			case _2DORIGIN_MIN:
			case _2DORIGIN_CENTER:
			case _2DORIGIN_MAX:
				return 1;
		}
		return 0;
	}

	bool isValid()
	{
		return valIsValid(x) && valIsValid(y);
	}

	bool isXCentered() { return scaler(x) == 0; }
	bool onYCenter() { return scaler(y) == 0; }
	bool onRight() { return scaler(x) == 1; }
	bool onLeft() { return scaler(x) == -1; }
	bool onTop() { return scaler(y) == 1; }
	bool onBottom() { return scaler(y) == -1; }

	static int inverted(int inputType, int outputType)
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

	int xInverted(_2DOrigin outputType) const
	{
		return inverted(x, outputType.x);
	}

	int yInverted(_2DOrigin outputType) const
	{
		return inverted(y, outputType.y);
	}

	static int invert(uint originValue)
	{
		switch(originValue)
		{
			case _2DORIGIN_MIN: return _2DORIGIN_MAX_INVERSE_CARTESIAN;
			case _2DORIGIN_MIN_INVERSE_CARTESIAN: return _2DORIGIN_MAX;
			case _2DORIGIN_CENTER: return _2DORIGIN_CENTER_INVERSE_CARTESIAN;
			case _2DORIGIN_CENTER_INVERSE_CARTESIAN: return _2DORIGIN_CENTER;
			case _2DORIGIN_MAX: return _2DORIGIN_MIN_INVERSE_CARTESIAN;
			case _2DORIGIN_MAX_INVERSE_CARTESIAN: return _2DORIGIN_MIN;
			default: bug_branch("%d", originValue); return 0;
		}
	}

	_2DOrigin invertX() const
	{
		_2DOrigin o(x, y);
		o.x = invert(x);
		return o;
	}

	_2DOrigin invertY() const
	{
		_2DOrigin o(x, y);
		o.y = invert(y);
		return o;
	}

	_2DOrigin invertYIfCartesian() const
	{
		_2DOrigin o(x, y);
		if(isCartesian(y))
		{
			o.y = invert(y);
			//logDMsg("inverted");
		}
		return o;
	}

	static int flip(uint originValue)
	{
		switch(originValue)
		{
			case _2DORIGIN_MIN: return _2DORIGIN_MAX;
			case _2DORIGIN_MIN_INVERSE_CARTESIAN: return _2DORIGIN_MAX_INVERSE_CARTESIAN;
			case _2DORIGIN_CENTER: return _2DORIGIN_CENTER;
			case _2DORIGIN_CENTER_INVERSE_CARTESIAN: return _2DORIGIN_CENTER_INVERSE_CARTESIAN;
			case _2DORIGIN_MAX: return _2DORIGIN_MIN;
			case _2DORIGIN_MAX_INVERSE_CARTESIAN: return _2DORIGIN_MIN_INVERSE_CARTESIAN;
			default: bug_branch("%d", originValue); return 0;
		}
	}

	_2DOrigin flipX() const
	{
		_2DOrigin o(x, y);
		o.x = flip(x);
		return o;
	}

	_2DOrigin flipY() const
	{
		_2DOrigin o(x, y);
		o.y = flip(y);
		return o;
	}

	template<class T>
	static T adjust(T pos, T halfSize, T fullSize, int inputScale, int outputScale)
	{
		int scaleDiff = inputScale - outputScale;
		if(std::abs(scaleDiff) == 1)
			return pos - IG::multBySignOf(halfSize, (T)scaleDiff);
		else if(std::abs(scaleDiff) == 2)
			return pos - IG::multBySignOf(fullSize, (T)scaleDiff);
		else
			return pos;
	}

	template<class T>
	T adjustX(T pos, T halfSize, T fullSize, _2DOrigin outputType) const
	{
		pos = xInverted(outputType) ? (fullSize) - pos : pos;
		return adjust(pos, halfSize, fullSize, xScaler(), outputType.xScaler());
	}

	template<class T>
	T adjustX(T pos, T fullSize, _2DOrigin outputType) const
	{
		return adjustX(pos, fullSize/(T)2, fullSize, outputType);
	}

	template<class T>
	T adjustY(T pos, T halfSize, T fullSize, _2DOrigin outputType) const
	{
		pos = yInverted(outputType) ? (fullSize) - pos : pos;
		return adjust(pos, halfSize, fullSize, yScaler(), outputType.yScaler());
	}

	template<class T>
	T adjustY(T pos, T fullSize, _2DOrigin outputType) const
	{
		return adjustY(pos, fullSize/(T)2, fullSize, outputType);
	}

	template<class T>
	T adjustYInv(T pos, T halfSize, T fullSize, _2DOrigin outputType) const
	{
		_2DOrigin o = invertY();
		return o.adjustY(pos, halfSize, fullSize, outputType.invertY());
	}

	template<class T>
	T adjustYInv(T pos, T fullSize, _2DOrigin outputType) const
	{
		return adjustYInv(pos, fullSize/(T)2, fullSize, outputType);
	}

	template<class T>
	T adjustXExtent(T pos, T halfSize, _2DOrigin outputType) const
	{
		assert(!xInverted(outputType));
		return adjust(pos, halfSize, halfSize+halfSize, xScaler(), outputType.xScaler());
	}

	template<class T>
	T adjustYExtent(T pos, T halfSize, _2DOrigin outputType) const
	{
		assert(!yInverted(outputType));
		return adjust(pos, halfSize, halfSize+halfSize, yScaler(), outputType.yScaler());
	}

	bool operator ==(_2DOrigin const& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	bool operator !=(_2DOrigin const& rhs) const
	{
		return !(*this == rhs);
	}

	operator unsigned int() const
	{
		//logMsg("converting 0x%X 0x%X to 0x%X", x, y, x | (y << 3));
		return x | (y << 3);
	}
};

// cartesian origin shortcuts sorted clockwise
static constexpr _2DOrigin CenterTop2DOrigin(_2DORIGIN_CENTER, _2DORIGIN_MAX);
#define CT2DO CenterTop2DOrigin
static constexpr _2DOrigin RightTop2DOrigin(_2DORIGIN_MAX, _2DORIGIN_MAX);
#define RT2DO RightTop2DOrigin
static constexpr _2DOrigin RightCenter2DOrigin(_2DORIGIN_MAX, _2DORIGIN_CENTER);
#define RC2DO RightCenter2DOrigin
static constexpr _2DOrigin RightBottom2DOrigin(_2DORIGIN_MAX, _2DORIGIN_MIN);
#define RB2DO RightBottom2DOrigin
static constexpr _2DOrigin CenterBottom2DOrigin(_2DORIGIN_CENTER, _2DORIGIN_MIN);
#define CB2DO CenterBottom2DOrigin
static constexpr _2DOrigin LeftBottom2DOrigin(_2DORIGIN_MIN, _2DORIGIN_MIN);
#define LB2DO LeftBottom2DOrigin
static constexpr _2DOrigin LeftCenter2DOrigin(_2DORIGIN_MIN, _2DORIGIN_CENTER);
#define LC2DO LeftCenter2DOrigin
static constexpr _2DOrigin LeftTop2DOrigin(_2DORIGIN_MIN, _2DORIGIN_MAX);
#define LT2DO LeftTop2DOrigin

static constexpr _2DOrigin Center2DOrigin(_2DORIGIN_CENTER, _2DORIGIN_CENTER);
#define C2DO Center2DOrigin

static constexpr _2DOrigin LeftBottomInvCart2DOrigin(_2DORIGIN_MIN, _2DORIGIN_MAX_INVERSE_CARTESIAN);
#define LBIC2DO LeftBottomInvCart2DOrigin
static constexpr _2DOrigin LeftTopInvCart2DOrigin(_2DORIGIN_MIN, _2DORIGIN_MIN_INVERSE_CARTESIAN);
#define LTIC2DO LeftTopInvCart2DOrigin
static constexpr _2DOrigin CenterInvCart2DOrigin(_2DORIGIN_CENTER, _2DORIGIN_CENTER_INVERSE_CARTESIAN);
#define CIC2DO CenterInvCart2DOrigin

static constexpr _2DOrigin Null2DOrigin(_2DORIGIN_NONE, _2DORIGIN_NONE);
#define NULL2DO Null2DOrigin
