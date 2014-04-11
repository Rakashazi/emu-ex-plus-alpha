#pragma once

#include <imagine/util/number.h>
#include <imagine/util/Point2D.hh>

class Rational
{
public:
	static constexpr int gcdInner(uint a, uint b)
  {
    return b == 0 ? a : gcdInner(b, a % b);
  }

  static constexpr int gcd(uint a, uint b)
  {
    return gcdInner(IG::maxConst(a, b), IG::minConst(a, b));
  }

	template <class T>
	static constexpr IG::Point2D<T> make(T n, T d)
	{
		return {gcd(n,d) ? n / gcd(n,d) : n, gcd(n,d) ? d / gcd(n,d) : d};
	}
};

namespace IG
{

template <class T, class T2>
static void setSizesWithRatioBestFit(T &xSize, T &ySize, T2 destAspectRatio, T x, T y)
{
	auto sourceRat = Rational::make(x,y);
	T2 sourceAspectRatio = sourceRat.ratio();
	logMsg("ar %f %f, %d %d", sourceAspectRatio, destAspectRatio, x, y);
	if(destAspectRatio == sourceAspectRatio)
	{
		xSize = x;
		ySize = y;
	}
	else if(destAspectRatio > sourceAspectRatio)
	{
		IG::setSizesWithRatioX(xSize, ySize, destAspectRatio, x);
	}
	else
	{
		IG::setSizesWithRatioY(xSize, ySize, destAspectRatio, y);
	}
}

}
