//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Rect.hxx 3131 2015-01-01 03:49:32Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RECT_HXX
#define RECT_HXX

#include <cassert>

#include "bspf.hxx"

namespace GUI {

/*
  This small class is an helper for position and size values.
*/
struct Point
{
  int x;  //!< The horizontal part of the point
  int y;  //!< The vertical part of the point

  Point() : x(0), y(0) { };
  Point(const Point& p) : x(p.x), y(p.y) { };
  explicit Point(int x1, int y1) : x(x1), y(y1) { };
  Point(const string& p) {
    char c = '\0';
    x = y = -1;
    istringstream buf(p);
    buf >> x >> c >> y;
    if(c != 'x')
      x = y = 0;
  }
  Point& operator=(const Point & p) { x = p.x; y = p.y; return *this; };
  bool operator==(const Point & p) const { return x == p.x && y == p.y; };
  bool operator!=(const Point & p) const { return x != p.x || y != p.y; };

  friend ostream& operator<<(ostream& os, const Point& p) {
    os << p.x << "x" << p.y;
    return os;
  }
};

struct Size
{
  uInt32 w;  //!< The width part of the size
  uInt32 h;  //!< The height part of the size

  Size() : w(0), h(0) { };
  Size(const Size& s) : w(s.w), h(s.h) { };
  explicit Size(uInt32 w1, uInt32 h1) : w(w1), h(h1) { };
  Size(const string& s) {
    char c = '\0';
    w = h = 0;
    istringstream buf(s);
    buf >> w >> c >> h;
    if(c != 'x')
      w = h = 0;
  }
  bool valid() const { return w > 0 && h > 0; }

  Size& operator=(const Size& s) { w = s.w; h = s.h; return *this; };
  bool operator==(const Size& s) const { return w == s.w && h == s.h; };
  bool operator!=(const Size& s) const { return w != s.w || h != s.h; };
  bool operator<(const Size& s)  const { return w < s.w && h < s.h;   };
  bool operator<=(const Size& s) const { return w <= s.w && h <= s.h; };
  bool operator>(const Size& s)  const { return w > s.w && h > s.h;   };
  bool operator>=(const Size& s) const { return w >= s.w && h >= s.h; };

  friend ostream& operator<<(ostream& os, const Size& s) {
    os << s.w << "x" << s.h;
    return os;
  }
};

/*
  This small class is an helper for rectangles.
  Note: This implementation is built around the assumption that (top,left) is
  part of the rectangle, but (bottom,right) is not! This is reflected in 
  various methods, including contains(), intersects() and others.

  Another very wide spread approach to rectangle classes treats (bottom,right)
  also as a part of the rectangle.

  Coneptually, both are sound, but the approach we use saves many intermediate
  computations (like computing the height in our case is done by doing this:
    height = bottom - top;
  while in the alternate system, it would be
    height = bottom - top + 1;

  When writing code using our Rect class, always keep this principle in mind!
*/
struct Rect
{
  uInt32 top, left;        //!< The point at the top left of the rectangle (part of the rect).
  uInt32 bottom, right;    //!< The point at the bottom right of the rectangle (not part of the rect).

  Rect() : top(0), left(0), bottom(0), right(0) { }
  Rect(const Rect& s) : top(s.top), left(s.left), bottom(s.bottom), right(s.right) { }
  Rect(uInt32 w, uInt32 h) : top(0), left(0), bottom(h), right(w) { }
  Rect(const Point& p, uInt32 w, uInt32 h) : top(p.y), left(p.x), bottom(h), right(w) { }
  Rect(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) : top(y1), left(x1), bottom(y2), right(x2)
  {
    assert(valid());
  }

  uInt32 x() const { return left; }
  uInt32 y() const { return top; }
  Point point() const { return Point(x(), y()); }

  uInt32 width() const  { return right - left; }
  uInt32 height() const { return bottom - top; }
  Size size() const { return Size(width(), height()); }

  void setWidth(uInt32 aWidth)   { right = left + aWidth;  }
  void setHeight(uInt32 aHeight) { bottom = top + aHeight; }
  void setSize(const Size& size) { setWidth(size.w); setHeight(size.h); }

  void setBounds(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) {
    top = y1;
    left = x1;
    bottom = y2;
    right = x2;
    assert(valid());
  }

  bool valid() const {
    return (left <= right && top <= bottom);
  }

  bool empty() const {
    return top == 0 && left == 0 && bottom == 0 && right == 0;
  }

  void moveTo(uInt32 x, uInt32 y) {
    bottom += y - top;
    right += x - left;
    top = y;
    left = x;
  }

  void moveTo(const Point& p) {
    moveTo(p.x, p.y);
  }

  friend ostream& operator<<(ostream& os, const Rect& r) {
    os << r.point() << "," << r.size();
    return os;
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Rect EmptyRect;

}  // End of namespace GUI

#endif
