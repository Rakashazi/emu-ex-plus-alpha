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

#include <config.h>
#include <util/number.h>
#include <util/2DOrigin.h>
#include <util/operators.hh>

//TODO: remove old code

template<class T>
class Rect2 : NotEquals< Rect2<T> >, Subtracts< Rect2<T> >, Adds< Rect2<T> >
{
public:
	T x = 0, y = 0, x2 = 0, y2 = 0;
	#ifdef __clang__
		// hack for clang due to missing symbol issue
		#define o LTIC2DO
	#else
		static constexpr _2DOrigin o = LTIC2DO;
	#endif

	constexpr Rect2() { }
	constexpr Rect2(T x, T y, T x2, T y2): x(x), y(y), x2(x2), y2(y2) { }

	bool operator ==(Rect2 const& rhs) const
	{
		return x == rhs.x && y == rhs.y && x2 == rhs.x2 && y2 == rhs.y2;
	}

	Rect2 & operator +=(Rect2 const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		x2 += rhs.x2;
		y2 += rhs.y2;
		return *this;
	}

	Rect2 & operator -=(Rect2 const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		x2 -= rhs.x2;
		y2 -= rhs.y2;
		return *this;
	}

	Rect2 & operator +=(IG::Point2D<T> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		x2 += rhs.x;
		y2 += rhs.y;
		return *this;
	}

	Rect2 & operator -=(IG::Point2D<T> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		x2 -= rhs.x;
		y2 -= rhs.y;
		return *this;
	}

	bool overlaps(const Rect2 &other) const
	{
		//logMsg("testing rect %d,%d %d,%d with %d,%d %d,%d", a1.x, a1.x2, a1.y, a1.y2, a2.x, a2.x2, a2.y, a2.y2);
		return  y2 < other.y ? 0 :
				y > other.y2 ? 0 :
				x2 < other.x ? 0 :
				x > other.x2 ? 0 : 1;
	}

	bool overlaps(T px, T py) const
	{
		//logMsg("testing %d,%d in rect %d,%d %d,%d", px, py, a.x, a.y, a.x2, a.y2);
		return IG::isInRange(px, x, x2+1) && IG::isInRange(py, y, y2+1);
	}

	bool contains(const Rect2 &other) const
	{
		//logMsg("testing %d,%d %d,%d is in %d,%d %d,%d", x, y, x2, y2, other.x, other.y, other.x2, other.y2);
		return x <= other.x && x2 >= other.x2 &&
			y <= other.y && y2 >= other.y2;
	}

	bool contains(const IG::Point2D<T> point) const
	{
		return contains({point.x, point.y, point.x, point.y});
	}

	T xCenter() const
	{
		return IG::midpoint(x, x2);
	}

	T xPos(_2DOrigin origin) const
	{
		switch(origin.xScaler())
		{
			case -1: return x;
			case 1: return x2;
		}
		return xCenter();
	}

	T yCenter() const
	{
		return IG::midpoint(y, y2);
	}

	T yPos(_2DOrigin origin) const
	{
		return yPos(origin, o);
	}

	T yPos(_2DOrigin origin, _2DOrigin rectOrigin) const
	{
		if(!rectOrigin.isYCartesian())
			origin = origin.invertYIfCartesian();
		switch(origin.yScaler())
		{
			case -1: return y;
			case 1: return y2;
		}
		return yCenter();
	}

	IG::Point2D<T> pos(_2DOrigin origin) const
	{
		return IG::Point2D<T>(xPos(origin), yPos(origin));
	}

	// set x2,y2 coordinates relative to x,y

	void setRelX(T newX, T xSize)
	{
		assert(xSize >= 0);
		x = newX;
		x2 = newX + xSize;
	}

	void setRelY(T newY, T ySize)
	{
		assert(ySize >= 0);
		y = newY;
		y2 = newY + ySize;
	}

	void setRel(T newX, T newY, T xSize, T ySize)
	{
		setRelX(newX, xSize);
		setRelY(newY, ySize);
		//logMsg("set rect to %d,%d %d,%d", x, y, x2, y2);
	}

	// same as above, but transform x,y to the specified corner

	void setXPosRel(T newX, T size, _2DOrigin origin)
	{
		newX = origin.adjustX(newX, size, o);
		setRelX(newX, size);
	}

	void setYPosRel(T newY, T size, _2DOrigin origin, _2DOrigin rectOrigin)
	{
		if(!rectOrigin.isYCartesian())
			origin = origin.invertYIfCartesian();
		newY = origin.adjustY(newY, size, o);
		setRelY(newY, size);
	}

	void setYPosRel(T newY, T size, _2DOrigin origin)
	{
		setYPosRel(newY, size, origin, o);
	}

	void setPosRel(T newX, T newY, T xSize, T ySize, _2DOrigin origin, _2DOrigin rectOrigin)
	{
		setXPosRel(newX, xSize, origin);
		setYPosRel(newY, ySize, origin, rectOrigin);
		//logMsg("set rect pos to %d,%d %d,%d", x, y, x2, y2);
	}

	void setPosRel(T newX, T newY, T xSize, T ySize, _2DOrigin origin)
	{
		setPosRel(newX, newY, xSize, ySize, origin, o);
	}

	void setPosRel(T newX, T newY, T size, _2DOrigin origin) // square shortcut
	{
		setPosRel(newX, newY, size, size, origin);
	}

	void setPosRel(IG::Point2D<T> pos, IG::Point2D<T> size, _2DOrigin origin)
	{
		setPosRel(pos.x, pos.y, size.x, size.y, origin);
	}

	void setPosRel(IG::Point2D<T> pos, T size, _2DOrigin origin) // square shortcut
	{
		setPosRel(pos.x, pos.y, size, size, origin);
	}

	void setPosRel(IG::Point2D<T> pos, T xSize, T ySize, _2DOrigin origin)
	{
		setPosRel(pos.x, pos.y, xSize, ySize, origin);
	}

	/*#ifdef CONFIG_GFX
	void setPosRel(T newX, T newY, T xSize, T ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		// adjust to the requested origin on the screen
		newX = LTIC2DO.adjustX(newX, (int)Gfx::viewPixelWidth(), screenOrigin.invertYIfCartesian());
		newY = LTIC2DO.adjustY(newY, (int)Gfx::viewPixelHeight(), screenOrigin.invertYIfCartesian());
		setPosRel(newX, newY, xSize, ySize, posOrigin);
	}

	void setPosRel(T newX, T newY, T size, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		setPosRel(newX, newY, size, size, posOrigin, screenOrigin);
	}
	#endif*/

	/*void setRelCentered(T newX, T newY, T xSize, T ySize)
	{
		x = x2 = newX;
		y = y2 = newY;
		T halfSizeX = xSize / (T)2;
		T halfSizeY = ySize / (T)2;
		x -= halfSizeX;
		y -= halfSizeY;
		x2 += halfSizeX;
		y2 += halfSizeY;
	}*/

	// set x,y, automatically setting x2,y2 to keep the same size

	void setXPos(T newX)
	{
		IG::setLinked(x, newX, x2);
	}

	void setXPos(T newX, _2DOrigin origin)
	{
		newX = origin.adjustX(newX, xSize(), o);
		setXPos(newX);
	}

	void setYPos(T newY)
	{
		IG::setLinked(y, newY, y2);
	}

	void setYPos(T newY, _2DOrigin origin)
	{
		if(!o.isYCartesian())
			origin = origin.invertYIfCartesian();
		newY = origin.adjustY(newY, ySize(), o);
		IG::setLinked(y, newY, y2);
	}

	void setPos(IG::Point2D<T> newPos)
	{
		setXPos(newPos.x);
		setYPos(newPos.y);
	}

	void setPos(IG::Point2D<T> newPos, _2DOrigin origin)
	{
		setXPos(newPos.x, origin);
		setYPos(newPos.y, origin);
	}

	static Rect2 createRel(T newX, T newY, T xSize, T ySize)
	{
		Rect2 r;
		logMsg("creating new rel rect %d,%d %d,%d", newX, newY, xSize, ySize);
		r.setRel(newX, newY, xSize, ySize);
		return r;
	}

	T xSize() const { return (x2 - x); }

	T ySize() const { return (y2 - y); }

	IG::Point2D<T> size() const
	{
		return IG::Point2D<T>(xSize(), ySize());
	}

	// fit x,x2 inside r's x,x2 at the nearest edge
	int fitInX(const Rect2 &r)
	{
		if(xSize() > r.xSize())
		{
			setXPos(r.x, CT2DO);
			return 1;
		}
		else if(x < r.x)
		{
			setXPos(r.x);
			return -1;
		}
		else if(x2 > r.x2)
		{
			IG::setLinked(x2, r.x2, x);
			return 1;
		}
		return 0;
	}

	// fit y,y2 inside r's y,y2 at the nearest edge
	int fitInY(const Rect2 &r)
	{
		if(ySize() > r.ySize())
		{
			setYPos(r.x, CIC2DO);
			return 1;
		}
		else if(y < r.y)
		{
			setYPos(r.y);
			return -1;
		}
		else if(y2 > r.y2)
		{
			IG::setLinked(y2, r.y2, y);
			return 1;
		}
		return 0;
	}

	// fit the rectangle inside r at the nearest edges
	// if inner doesn't fit in outer, TODO
	int fitIn(const Rect2 &r)
	{
		int boundedX = fitInX(r);
		int boundedY = fitInY(r);
		return boundedX || boundedY;
	}

	/*#ifdef CONFIG_GFX
	Coordinate gXSize() const { return gfx_iXSize(xSize()); }
	Coordinate gYSize() const { return gfx_iYSize(ySize()); }
	Coordinate gXPos(_2DOrigin o) const
	{
		T pos = xPos(o);
		if(o.xScaler() == 1)
			pos++;
		return gfx_iXPos(pos);
	}
	Coordinate gYPos(_2DOrigin o) const
	{
		T pos = yPos(o);
		if(o.invertYIfCartesian().yScaler() == 1)
			pos++;
		return gfx_iYPos(pos);
	}
	Coordinate gXPos(Coordinate scale, _2DOrigin o) const { return gXPos(o) + (gXSize() * (scale)); }
	Coordinate gYPos(Coordinate scale, _2DOrigin o) const { return gYPos(o) + (gYSize() * (scale)); }

	void setClipRectBounds()
	{
		gfx_setClipRectBounds(x, y, xSize(), ySize());
	}

	void setFullView()
	{
		x = y = 0;
		x2 = gfx_viewPixelWidth();
		y2 = gfx_viewPixelHeight();
	}

	void loadGfxTransforms(_2DOrigin o) const
	{
		gfx_loadTranslate(gXPos(o), gYPos(o));
		gfx_applyScale(gXSize(), gYSize());
	}
	#endif*/

	#ifdef __clang__
		#undef o
	#endif
};

