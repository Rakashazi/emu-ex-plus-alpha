#pragma once

class Rational
{
public:
	uint numer, denom;
	constexpr Rational(): numer(0), denom(1) { }
	Rational(int n, int d)
	{
		int g = gcd(n,d);
		if(g)
		{
			n /= g;
			d /= g;
		}

		numer = n;
		denom = d;
	}

	uint gcd(uint u, uint v)
	{
		int shift;

		// gcd(0,x) = x
		if (u == 0 || v == 0)
			return u | v;

		for (shift = 0; ((u | v) & 1) == 0; ++shift)
		{
			u >>= 1;
			v >>= 1;
		}

		while ((u & 1) == 0)
			u >>= 1;

		do
		{
			while ((v & 1) == 0)
				v >>= 1;

			if (u < v)
			{
				v -= u;
			}
			else
			{
				int diff = u - v;
				u = v;
				v = diff;
			}
			v >>= 1;
		} while (v != 0);

		return u << shift;
	}

	operator bool() const
	{
		return(numer > 0);
	}

	operator float() const
	{
		return (float)numer/(float)denom;
	}
};
