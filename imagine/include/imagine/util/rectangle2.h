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

#include <imagine/util/number.h>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/operators.hh>
#include <imagine/util/Point2D.hh>

namespace IG
{

template<class T>
class Rect2 : public NotEquals< Rect2<T> >, public Arithmetics< Rect2<T> >
{
public:
	T x = 0, y = 0, x2 = 0, y2 = 0;
	static constexpr _2DOrigin o = LTIC2DO;

	constexpr Rect2() {}
	constexpr Rect2(T x, T y, T x2, T y2): x(x), y(y), x2(x2), y2(y2) {}

	static Rect2 makeRel(T newX, T newY, T xSize, T ySize)
	{
		Rect2 r;
		//logMsg("creating new rel rect %d,%d %d,%d", newX, newY, xSize, ySize);
		r.setRel(newX, newY, xSize, ySize);
		return r;
	}

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

	Rect2 & operator *=(Rect2 const& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		x2 *= rhs.x2;
		y2 *= rhs.y2;
		return *this;
	}

	Rect2 & operator /=(Rect2 const& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		x2 /= rhs.x2;
		y2 /= rhs.y2;
		return *this;
	}

	Rect2 & operator *=(IG::Point2D<T> const& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		x2 *= rhs.x;
		y2 *= rhs.y;
		return *this;
	}

	Rect2 & operator /=(IG::Point2D<T> const& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		x2 /= rhs.x;
		y2 /= rhs.y;
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

	bool overlaps(const IG::Point2D<T> point) const
	{
		//logMsg("testing %d,%d in rect %d,%d %d,%d", px, py, a.x, a.y, a.x2, a.y2);
		return IG::isInRange(point.x, x, x2+1) && IG::isInRange(point.y, y, y2+1);
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

	/*T xPos(_2DOrigin origin) const
	{
		switch(origin.xScaler())
		{
			case -1: return x;
			case 1: return x2;
		}
		return xCenter();
	}*/

	T yCenter() const
	{
		return IG::midpoint(y, y2);
	}

	IG::Point2D<T> center() const
	{
		return {xCenter(), yCenter()};
	}

	IG::Point2D<T> xAxis() const
	{
		return {x, x2};
	}

	IG::Point2D<T> yAxis() const
	{
		return {y, y2};
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

	// set x,y, automatically setting x2,y2 to keep the same size

	void setXPos(T newX)
	{
		IG::setLinked(x, newX, x2);
	}

	void setYPos(T newY)
	{
		IG::setLinked(y, newY, y2);
	}

	void setPos(IG::Point2D<T> newPos)
	{
		setXPos(newPos.x);
		setYPos(newPos.y);
	}

	T xSize() const { return (x2 - x); }

	T ySize() const { return (y2 - y); }

	IG::Point2D<T> size() const
	{
		return {xSize(), ySize()};
	}

	void setXSize(T size, T anchor)
	{
		T offset = (T)0;
		if(x != anchor)
			offset = anchor - x;
		setRelX(x + offset, size);
	}

	void setYSize(T size, T anchor)
	{
		T offset = (T)0;
		if(y != anchor)
			offset = anchor - y;
		setRelY(y + offset, size);
	}

	void setSize(IG::Point2D<T> size, IG::Point2D<T> anchor)
	{
		setXSize(size.x, anchor.x);
		setYSize(size.y, anchor.y);
	}

	// fit x,x2 inside r's x,x2 at the nearest edge
	int fitInX(const Rect2 &r)
	{
		if(xSize() > r.xSize())
		{
			setXPos(r.x - xSize()/2);
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
			setYPos(r.y - ySize()/2);
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

	void fitPoint(IG::Point2D<T> &p)
	{
		if(p.x < x)
			p.x = x;
		else if(p.x > x2)
			p.x = x2;
		if(p.y < y)
			p.y = y;
		else if(p.y > y2)
			p.y = y2;
	}
};

template<class T>
constexpr _2DOrigin Rect2<T>::o;

template<class T>
static Rect2<T> makeRectRel(T x, T y, T xSize, T ySize)
{
	return Rect2<T>::makeRel(x, y, xSize, ySize);
}

template<class T, bool xIsCartesian, bool yIsCartesian>
class CoordinateRect : public Rect2<T>
{
public:
	using Point2DType = IG::Point2D<T>;
	using Rect2<T>::setXPos;
	using Rect2<T>::setYPos;
	using Rect2<T>::setPos;
	using Rect2<T>::setRel;
	using Rect2<T>::x;
	using Rect2<T>::y;
	using Rect2<T>::x2;
	using Rect2<T>::y2;
	static constexpr int xOriginVal = xIsCartesian ? -1 : 1;
	static constexpr int x2OriginVal = xIsCartesian ? 1 : -1;
	static constexpr int yOriginVal = yIsCartesian ? -1 : 1;
	static constexpr int y2OriginVal = yIsCartesian ? 1 : -1;

	constexpr CoordinateRect() {}
	constexpr CoordinateRect(T x, T y, T x2, T y2): Rect2<T>{x, y, x2, y2} {}

	static CoordinateRect makeRel(T newX, T newY, T xSize, T ySize)
	{
		CoordinateRect r;
		//logMsg("creating new rel rect %d,%d %d,%d", newX, newY, xSize, ySize);
		r.setRel(newX, newY, xSize, ySize);
		return r;
	}

	T xPos(_2DOrigin origin) const
	{
		switch(origin.xScaler())
		{
			case xOriginVal: return x;
			case x2OriginVal: return x2;
			default: return IG::midpoint(x, x2);
		}
	}

	T yPos(_2DOrigin origin) const
	{
		switch(origin.yScaler())
		{
			case yOriginVal: return y;
			case y2OriginVal: return y2;
			default: return IG::midpoint(y, y2);
		}
	}

	Point2DType pos(_2DOrigin origin) const
	{
		return Point2DType{xPos(origin), yPos(origin)};
	}

	void setXPos(T p, _2DOrigin origin)
	{
		setXPos(p);
		auto offset = x - pos(origin).x;
		setXPos(x + offset);
	}

	void setYPos(T p, _2DOrigin origin)
	{
		setYPos(p);
		auto offset = y - pos(origin).y;
		setYPos(y + offset);
	}

	void setPos(Point2DType p, _2DOrigin origin)
	{
		setPos(p);
		auto offset = Point2DType{x, y} - pos(origin);
		setPos({x + offset.x, y + offset.y});
	}

	void setPosRel(Point2DType p, Point2DType size, _2DOrigin origin)
	{
		setRel(p.x, p.y, size.x, size.y);
		auto offset = Point2DType{x, y} - pos(origin);
		setRel(x + offset.x, y + offset.y, size.x, size.y);
	}

	void setPosRel(Point2DType p, T size, _2DOrigin origin)
	{
		setPosRel(p, {size, size}, origin);
	}
};

using WindowRect = CoordinateRect<int, true, false>;

using WP = WindowRect::Point2DType;

static WindowRect makeWindowRectRel(WP pos, WP size)
{
	return WindowRect::makeRel(pos.x, pos.y, size.x, size.y);
}

}
