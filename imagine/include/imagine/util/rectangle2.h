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

#include <imagine/util/2DOrigin.h>
#include <imagine/util/AssignmentArithmetics.hh>
#include <imagine/util/Point2D.hh>
#include <compare>

namespace IG
{

template<class T>
class Rect2 : public AssignmentArithmetics< Rect2<T> >
{
public:
	T x{}, y{}, x2{}, y2{};
	static constexpr _2DOrigin o = LTIC2DO;

	constexpr Rect2() {}
	constexpr Rect2(Point2D<T> p1, Point2D<T> p2): x(p1.x), y(p1.y), x2(p2.x), y2(p2.y) {}

	static Rect2 makeRel(Point2D<T> pos, Point2D<T> size)
	{
		Rect2 r;
		//logMsg("creating new rel rect %d,%d %d,%d", newX, newY, xSize, ySize);
		r.setRel(pos.x, pos.y, size.x, size.y);
		return r;
	}

	constexpr bool operator ==(Rect2 const& rhs) const = default;

	constexpr Rect2 operator +(Rect2 const& rhs) const
	{
		return {{x + rhs.x, y + rhs.y}, {x2 + rhs.x2, y2 + rhs.y2}};
	}

	constexpr Rect2 operator -(Rect2 const& rhs) const
	{
		return {{x - rhs.x, y - rhs.y}, {x2 - rhs.x2, y2 - rhs.y2}};
	}

	constexpr Rect2 operator *(Rect2 const& rhs) const
	{
		return {{x * rhs.x, y * rhs.y}, {x2 * rhs.x2, y2 * rhs.y2}};
	}

	constexpr Rect2 operator /(Rect2 const& rhs) const
	{
		return {{x / rhs.x, y / rhs.y}, {x2 / rhs.x2, y2 / rhs.y2}};
	}

	constexpr Rect2 operator-() const
	{
		return {{-x, -y}, {-x2, -y2}};
	}

	constexpr Rect2 & operator +=(IG::Point2D<T> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		x2 += rhs.x;
		y2 += rhs.y;
		return *this;
	}

	constexpr Rect2 & operator -=(IG::Point2D<T> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		x2 -= rhs.x;
		y2 -= rhs.y;
		return *this;
	}

	constexpr Rect2 & operator *=(IG::Point2D<T> const& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		x2 *= rhs.x;
		y2 *= rhs.y;
		return *this;
	}

	constexpr Rect2 & operator /=(IG::Point2D<T> const& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		x2 /= rhs.x;
		y2 /= rhs.y;
		return *this;
	}

	constexpr Rect2 makeInverted() const
	{
		return {{y, x}, {y2, x2}};
	}

	constexpr bool overlaps(Rect2 other) const
	{
		//logMsg("testing rect %d,%d %d,%d with %d,%d %d,%d", a1.x, a1.x2, a1.y, a1.y2, a2.x, a2.x2, a2.y, a2.y2);
		return  y2 < other.y ? 0 :
				y > other.y2 ? 0 :
				x2 < other.x ? 0 :
				x > other.x2 ? 0 : 1;
	}

	constexpr bool overlaps(IG::Point2D<T> p) const
	{
		//logMsg("testing %d,%d in rect %d,%d %d,%d", p.x, p.y, x, y, x2, y2);
		return IG::isInRange(p.x, x, x2+1) && IG::isInRange(p.y, y, y2+1);
	}

	constexpr bool contains(Rect2 other) const
	{
		//logMsg("testing %d,%d %d,%d is in %d,%d %d,%d", x, y, x2, y2, other.x, other.y, other.x2, other.y2);
		return x <= other.x && x2 >= other.x2 &&
			y <= other.y && y2 >= other.y2;
	}

	constexpr bool contains(IG::Point2D<T> point) const
	{
		return contains({point, point});
	}

	constexpr T xCenter() const
	{
		return Point2D<T>{x, x2}.midpoint();
	}

	constexpr T yCenter() const
	{
		return Point2D<T>{y, y2}.midpoint();
	}

	constexpr IG::Point2D<T> center() const
	{
		return {xCenter(), yCenter()};
	}

	constexpr IG::Point2D<T> xAxis() const
	{
		return {x, x2};
	}

	constexpr IG::Point2D<T> yAxis() const
	{
		return {y, y2};
	}

	// set x2,y2 coordinates relative to x,y

	constexpr void setRelX(T newX, T xSize)
	{
		assert(xSize >= 0);
		x = newX;
		x2 = newX + xSize;
	}

	constexpr void setRelY(T newY, T ySize)
	{
		assert(ySize >= 0);
		y = newY;
		y2 = newY + ySize;
	}

	constexpr void setRel(IG::Point2D<T> pos, IG::Point2D<T> size)
	{
		setRelX(pos.x, size.x);
		setRelY(pos.y, size.y);
		//logMsg("set rect to %d,%d %d,%d", x, y, x2, y2);
	}

	// set x,y, automatically setting x2,y2 to keep the same size

	constexpr void setXPos(T newX)
	{
		setLinked(x, newX, x2);
	}

	constexpr void setYPos(T newY)
	{
		setLinked(y, newY, y2);
	}

	constexpr void setPos(IG::Point2D<T> newPos)
	{
		setXPos(newPos.x);
		setYPos(newPos.y);
	}

	constexpr T xSize() const { return (x2 - x); }

	constexpr T ySize() const { return (y2 - y); }

	constexpr IG::Point2D<T> size() const
	{
		return {xSize(), ySize()};
	}

	constexpr void setXSize(T size, T anchor)
	{
		T offset = (T)0;
		if(x != anchor)
			offset = anchor - x;
		setRelX(x + offset, size);
	}

	constexpr void setYSize(T size, T anchor)
	{
		T offset = (T)0;
		if(y != anchor)
			offset = anchor - y;
		setRelY(y + offset, size);
	}

	constexpr void setSize(IG::Point2D<T> size, IG::Point2D<T> anchor)
	{
		setXSize(size.x, anchor.x);
		setYSize(size.y, anchor.y);
	}

	// fit x,x2 inside r's x,x2 at the nearest edge
	constexpr int fitInX(Rect2 r)
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
			setLinked(x2, r.x2, x);
			return 1;
		}
		return 0;
	}

	// fit y,y2 inside r's y,y2 at the nearest edge
	constexpr int fitInY(Rect2 r)
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
			setLinked(y2, r.y2, y);
			return 1;
		}
		return 0;
	}

	// fit the rectangle inside r at the nearest edges
	// if inner doesn't fit in outer, TODO
	constexpr int fitIn(Rect2 r)
	{
		int boundedX = fitInX(r);
		int boundedY = fitInY(r);
		return boundedX || boundedY;
	}

	[[nodiscard]]
	constexpr IG::Point2D<T> fitPoint(IG::Point2D<T> p)
	{
		if(p.x < x)
			p.x = x;
		else if(p.x > x2)
			p.x = x2;
		if(p.y < y)
			p.y = y;
		else if(p.y > y2)
			p.y = y2;
		return p;
	}

	constexpr static void setLinked(T &var, T newVal, T &linkedVar)
	{
		linkedVar += newVal - var;
		var = newVal;
	}
};

template<class T>
constexpr _2DOrigin Rect2<T>::o;

template<class T>
constexpr static Rect2<T> makeRectRel(Point2D<T> pos, Point2D<T> size)
{
	return Rect2<T>::makeRel(pos, size);
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
	constexpr CoordinateRect(Rect2<T> rect):Rect2<T>{rect} {}
	constexpr CoordinateRect(Point2DType p1, Point2DType p2): Rect2<T>{p1, p2} {}

	constexpr static CoordinateRect makeRel(Point2D<T> pos, Point2D<T> size)
	{
		CoordinateRect r;
		r.setRel(pos, size);
		return r;
	}

	constexpr T xPos(_2DOrigin origin) const
	{
		switch(origin.xScaler())
		{
			case xOriginVal: return x;
			case x2OriginVal: return x2;
			default: return Point2DType{x, x2}.midpoint();
		}
	}

	constexpr T yPos(_2DOrigin origin) const
	{
		switch(origin.yScaler())
		{
			case yOriginVal: return y;
			case y2OriginVal: return y2;
			default: return Point2DType{y, y2}.midpoint();
		}
	}

	constexpr Point2DType pos(_2DOrigin origin) const
	{
		return Point2DType{xPos(origin), yPos(origin)};
	}

	constexpr void setXPos(T p, _2DOrigin origin)
	{
		setXPos(p);
		auto offset = x - pos(origin).x;
		setXPos(x + offset);
	}

	constexpr void setYPos(T p, _2DOrigin origin)
	{
		setYPos(p);
		auto offset = y - pos(origin).y;
		setYPos(y + offset);
	}

	constexpr void setPos(Point2DType p, _2DOrigin origin)
	{
		setPos(p);
		auto offset = Point2DType{x, y} - pos(origin);
		setPos({x + offset.x, y + offset.y});
	}

	constexpr void setPosRel(Point2DType p, Point2DType size, _2DOrigin origin)
	{
		setRel(p, size);
		auto offset = Point2DType{x, y} - pos(origin);
		setRel({x + offset.x, y + offset.y}, size);
	}

	constexpr void setPosRel(Point2DType p, T size, _2DOrigin origin)
	{
		setPosRel(p, {size, size}, origin);
	}

	constexpr CoordinateRect makeInverted() const
	{
		return Rect2<T>::makeInverted();
	}
};

using WindowRect = CoordinateRect<int, true, false>;

using WP = WindowRect::Point2DType;

constexpr static WindowRect makeWindowRectRel(WP pos, WP size)
{
	return WindowRect::makeRel(pos, size);
}

}
