#pragma once

#include <util/number.h>

class Rational
{
public:
	uint numer = 0, denom = 1;
	constexpr Rational() { }
	constexpr Rational(int n, int d):
		numer(gcd(n,d) ? n / gcd(n,d) : n),
		denom(gcd(n,d) ? d / gcd(n,d) : d)
	{ }

	static constexpr int gcd_inner(uint a, uint b)
  {
    return b == 0 ? a : gcd_inner(b, a % b);
  }
  static constexpr int gcd(uint a, uint b)
  {
    return gcd_inner(IG::maxConst(a, b), IG::minConst(a, b));
  }

  constexpr operator bool() const
	{
		return numer > 0;
	}

	constexpr operator float() const
	{
		return (float)numer/(float)denom;
	}
};
