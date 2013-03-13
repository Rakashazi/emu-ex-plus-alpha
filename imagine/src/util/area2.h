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

#include <gfx/defs.hh>
#include <util/2DOrigin.h>
#include <util/number.h>
#include <util/Rational.hh>
#include <util/rectangle2.h>
#include <logger/interface.h>
#include <gfx/Gfx.hh>

// TODO: merge/utilize Rect class
#define AREA_INTERNAL_2DO LB2DO

class Area
{
public:
	constexpr Area() { }

	void init(int aRatioNumerator, int aRatioDenominator)
	{
		if(aRatioDenominator)
			aRatio_ = Rational(aRatioNumerator, aRatioDenominator);//(GC)aRatioNumerator / (GC)aRatioDenominator;
		else
			aRatio_ = Rational(0,1);
		//if(aRatio_) logMsg("area init with %d/%d aspect", aRatio_.numer, aRatio_.denom);
	}

	void init() { init(0,0); }

	void initFullscreen()
	{
		init(0, 0);
		setSizeViewSpaceBestFit();
		setPosOrigin(C2DO, C2DO);
	}

	GC aRatio() const
	{
		if(aRatio_)
			return (GC)aRatio_;
		else return xSize / ySize;
	}

	void setYSize(GC y)
	{
		IG::setSizesWithRatioY(xSize, ySize, (GC)aRatio_, y);

		iYSize = Gfx::toIYSize(ySize);
		if(aRatio_)
			iXSize = Gfx::toIXSize(xSize);
		//logMsg("set area size %f,%f %d,%d", xSize, ySize, iXSize, iYSize);
	}

	void setXSize(GC x)
	{
		IG::setSizesWithRatioX(xSize, ySize, (GC)aRatio_, x);

		iXSize = Gfx::toIXSize(xSize);
		if(aRatio_)
			iYSize = Gfx::toIYSize(ySize);
		//logMsg("set area size %f,%f %d,%d", xSize, ySize, iXSize, iYSize);
	}

	void scaleSize(GC scaler)
	{
		xSize *= scaler;
		ySize *= scaler;
		iXSize = Gfx::toIXSize(xSize);
		iYSize = Gfx::toIYSize(ySize);
	}

	void setSizeBestFit(GC x, GC y)
	{
		// TODO: rework this now that aspect ratio is stored in rational form
		int approxSameAR = IG::valIsWithinStretch(x / y, (GC)aRatio_, (GC)0.0009); // avoid rounding error when calculating x/y
		if(!aRatio_ || approxSameAR)
		{
			logDMsg("area fits size exactly, no aspect ratio scaling applied");
			xSize = x;
			ySize = y;

			iXSize = Gfx::toIXSize(x);
			iYSize = Gfx::toIYSize(y);
		}
		else if((GC)aRatio_ > x / y)
		{
			setXSize(x);
		}
		else
		{
			setYSize(y);
		}
	}

	void setSizeViewSpaceBestFit()
	{
		GC x = Gfx::proj.w;
		GC y = Gfx::proj.h;
		setSizeBestFit(x, y);
		logMsg("area sized %f,%f input %d,%d", (double)xSize, (double)ySize, iXSize, iYSize);
	}

	void updateInputPos()
	{
		inputXPos = AREA_INTERNAL_2DO.adjustX(Gfx::toIXPos(xPos_), iXSize, LT2DO);
		inputYPos = AREA_INTERNAL_2DO.adjustYInv(Gfx::toIYPos(yPos_), iYSize, LT2DO);
	}

	void alignToPixelUnits()
	{
		/*xPos_ = IG::roundMult(xPos_, xPerI);
		yPos_ = IG::roundMult(yPos_, yPerI);
		xSize = IG::roundMult(xSize, xPerI);
		ySize = IG::roundMult(ySize, yPerI);*/
		xPos_ = Gfx::alignXToPixel(xPos_);// - .5;
		yPos_ = Gfx::alignYToPixel(yPos_);// - .5;
		xSize = Gfx::alignXToPixel(xSize);//- .5;
		ySize = Gfx::alignYToPixel(ySize);// - .5;
		//logMsg("aligned to %f,%f size %f,%f", xPos_, yPos_, xSize, ySize);
	}

	// moves the area (origin defined by posOrigin) to the screen/view space's x,y coordinates (origin defined by screenOrigin)
	// examples:
	// setPos(0, 0, C2DO, C2DO) : move the area's center to the screen center
	// setPos(0, 0, LC2DO, C2DO) : move the area's left side to the screen center
	// setPos(0, 0, LC2DO, LC2DO) : move the area's left side to the screen's left side
	void setPos(GC x, GC y, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		//logMsg("called setPos with size %f,%f", xSize, ySize);
		assert(xSize != (GC)0 || ySize != (GC)0); // area size should be initialized beforehand

		// adjust to the requested origin on the screen
		xPos_ = C2DO.adjustX(x, Gfx::proj.wHalf(), Gfx::proj.w, screenOrigin);
		yPos_ = C2DO.adjustY(y, Gfx::proj.hHalf(), Gfx::proj.h, screenOrigin);
		//logMsg("adjusting for screen 2DO %f,%f", CtoF(a->xPos), CtoF(a->yPos));

		// adjust from the area position origin to the origin used for internal representation purposes
		xPos_ = posOrigin.adjustX(xPos_, xSize, AREA_INTERNAL_2DO);
		yPos_ = posOrigin.adjustY(yPos_, ySize, AREA_INTERNAL_2DO);
		//logMsg("adjusting for pos 2DO %f,%f", CtoF(a->xPos), CtoF(a->yPos));

		updateInputPos();

		//logMsg("set area pos %f,%f input %d,%d", (float)xPos_, (float)yPos_, inputXPos, inputYPos);
	}

	void setPosOrigin(_2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		setPos(0, 0, posOrigin, screenOrigin);
	}

	void setPos(Area *a, GC x, GC y, _2DOrigin posOrigin, _2DOrigin areaOrigin)
	{
		//logMsg("setPos(Area*, ...)");
		// adjust to the requested origin on the destination area
		xPos_ = AREA_INTERNAL_2DO.adjustX(a->xPos_ + x, a->xSize, areaOrigin);
		yPos_ = AREA_INTERNAL_2DO.adjustY(a->yPos_ + y, a->ySize, areaOrigin);
		//logMsg("adjusting for screen 2DO %f,%f", CtoF(a->xPos), CtoF(a->yPos));

		// adjust from the area position origin to the origin used for internal representation purposes
		xPos_ = posOrigin.adjustX(xPos_, xSize, AREA_INTERNAL_2DO);
		yPos_ = posOrigin.adjustY(yPos_, ySize, AREA_INTERNAL_2DO);

		updateInputPos();

		//logMsg("set area pos by area %f,%f input %d,%d", (float)xPos_, (float)yPos_, inputXPos, inputYPos);
	}

	void setPos(Area *a, _2DOrigin posOrigin, _2DOrigin areaOrigin)
	{
		setPos(a, 0, 0, posOrigin, areaOrigin);
	}

	void setPos(Rect2<int> *b, GC x, GC y, _2DOrigin posOrigin, _2DOrigin areaOrigin)
	{
		Area a;
		a.init(0,0);
		a.setXSize(Gfx::gXSize(*b));
		a.setYSize(Gfx::gYSize(*b));
		a.setPos(Gfx::gXPos(*b, LB2DO), Gfx::gYPos(*b, LB2DO), LB2DO, C2DO);
		setPos(&a, x, y, posOrigin, areaOrigin);
	}

	void setPos(Rect2<int> *b, _2DOrigin posOrigin, _2DOrigin areaOrigin)
	{
		setPos(b, 0, 0, posOrigin, areaOrigin);
	}

	int overlaps(int x, int y) const
	{
		//Rect<int> r(inputXPos, inputYPos, iXSize, iYSize, Rect<int>::rel);
		Rect2<int> r;
		r.setRel(inputXPos, inputYPos, iXSize, iYSize);
		return r.overlaps(x, y);
	}

	GC xOrigin(_2DOrigin o) const { return AREA_INTERNAL_2DO.adjustX(xPos_, xSize, o); }
	// area_xPos, returns the x position offset by the area position's and fraction of the area's size in gfx projection space
	// standard [0, 1.0] * size versions, z = 1.0
	GC xPos(GC frac, _2DOrigin o) const { return xOrigin(o) + (xSize * frac); }
	GC xPos(_2DOrigin o) const { return xOrigin(o); }

	GC yOrigin(_2DOrigin o) const { return AREA_INTERNAL_2DO.adjustY(yPos_, ySize, o); }
	// area_yPos, returns the y position offset by the area position's and fraction of the area's size in gfy projection space
	// standard [0, 1.0] * size versions, z = 1.0
	GC yPos(GC frac, _2DOrigin o) const { return yOrigin(o) + (ySize * frac); }
	GC yPos(_2DOrigin o) const { return yOrigin(o); }

	int xIOrigin(_2DOrigin o) const { return LT2DO.adjustX(inputXPos, iXSize, o); }
	// area_xIPos, returns the x position offset by the area position's and fraction of the area's size in input space
	int xIPos(GC frac, _2DOrigin o) const { return xIOrigin(o) + int((GC)iXSize * frac); }
	int xIPos(_2DOrigin o) const { return xIOrigin(o); }

	int yIOrigin(_2DOrigin o) const { return LT2DO.adjustYInv(inputYPos, iYSize, o); }
	// area_yIPos, returns the y position offset by the area position's and fraction of the area's size in input space
	int yIPos(GC frac, _2DOrigin o) const { return yIOrigin(o) + int((GC)iYSize * frac); }
	int yIPos(_2DOrigin o) const { return yIOrigin(o); }

	int xIPosCustomInvI(int pos, int range, _2DOrigin o) const { return (int)IG::scalePointRange((GC)(pos - xIOrigin(o)), (GC)0, (GC)iXSize, (GC)0, (GC)range); }
	int yIPosCustomInvI(int pos, int range, _2DOrigin o) const { return (int)IG::scalePointRange((GC)(pos - yIOrigin(o)), (GC)0, (GC)iYSize, (GC)0, (GC)range); }

	IG::Point2D<int> iPos(GC xFrac, GC yFrac, _2DOrigin o) const
	{
		return IG::Point2D<int>(xIPos(xFrac, o), yIPos(yFrac, o));
	}

	Rect2<int> toIntRect() const
	{
		Rect2<int> b;
		b.x = xIPos(LT2DO);
		b.y = yIPos(LT2DO);
		b.x2 = xIPos(RB2DO);
		b.y2 = yIPos(RB2DO);
		return b;
	}

	/*int boundInside(Area *dest)
	{
		Rect<GC> srcR(xPos(LB2DO), yPos(LB2DO), xSize, ySize),
			destR(dest->xPos(LB2DO), dest->yPos(LB2DO), dest->xSize, dest->ySize);
		srcR.makeAbsolute(Rect<GC>::rel);
		destR.makeAbsolute(Rect<GC>::rel);
		int result = srcR.boundInside(Rect<GC>::abs, &destR, Rect<GC>::abs);
		setXSize(srcR.x2 - srcR.x);
		setYSize(srcR.y2 - srcR.y);
		setPos(srcR.x, srcR.y, LB2DO, C2DO);
		return result;
	}*/

	GC xSize = 0, ySize = 0;
	int iXSize = 0, iYSize = 0;
private:
	Rational aRatio_;
	GC xPos_ = 0, yPos_ = 0;
	int inputXPos = 0, inputYPos = 0;
};
