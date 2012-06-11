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

#include <util/number.h>
#include <util/Rational.hh>
#include <util/operators.hh>

namespace IG
{

template<class T>
struct FixedPOD
{
	T val;
	FixedPOD() = default;
	constexpr FixedPOD(T val): val(val) { }

	operator int() const
	{
		return (int)val;
	}
};

template<class T, unsigned char F, class BIGGER>
class Fixed : public FixedPOD<T>,
	NotEquals< Fixed<T,F,BIGGER> >, Compares< Fixed<T,F,BIGGER> >,
	Arithmetics< Fixed<T,F,BIGGER> >/*, ArithmeticsConv< Fixed<T,F,BIGGER> >*/
{
public:
	static const unsigned char intBits = (sizeof(T)*8) - F;
	static const unsigned char fracBits = F;
	static const unsigned int fracMaxVal = 1 << fracBits;
	static const unsigned int fracMaxValExtraBit = 1 << (fracBits+1);
	static const unsigned int fracMaxValOneLessBit = 1 << (fracBits-1);
	static const unsigned int fracMask = fracMaxVal - 1;
	using FixedPOD<T>::val;

	constexpr Fixed() { }

	template<class FT>
	static constexpr T convertFloat(FT num)
	{
		//printf("converted from float\n");
		return (T)( ( (T)num << fracBits ) + ( ( ( num - (T)num ) + 1./(FT)fracMaxValExtraBit ) * fracMaxVal ) );
	}

	constexpr Fixed(float num): FixedPOD<T>(convertFloat(num)) { }
	constexpr Fixed(double num): FixedPOD<T>(convertFloat(num)) { }

	template<class IT>
	static constexpr T convertInt(IT num)
	{
		//printf("converted from int\n");
		return (T)num << fracBits;
	}

	constexpr Fixed(int num): FixedPOD<T>(convertInt(num)) { }
	constexpr Fixed(uint num): FixedPOD<T>(convertInt(num)) { }
	constexpr Fixed(short num): FixedPOD<T>(convertInt(num)) { }
	constexpr Fixed(char num): FixedPOD<T>(convertInt(num)) { }

	Fixed(Rational num)
	{
		Fixed<T,F,BIGGER> n = Fixed<T,F,BIGGER>(num.numer);
		Fixed<T,F,BIGGER> d = Fixed<T,F,BIGGER>(num.denom);
		Fixed<T,F,BIGGER> result = n/d;
		val = result.val;
	}

	constexpr Fixed(FixedPOD<T> num): FixedPOD<T>(num.val) { }

	T frac() const { return val & fracMask; }
	T integer() const { return val >> fracBits; }

	operator int() const
	{
		return (int)(integer());
	}

	operator unsigned int() const
	{
		return (unsigned int)(integer());
	}

	float floatingPt() const
	{
		return integer() + ( (frac()/(float)fracMaxVal) - 1./(float)fracMaxValExtraBit );
	}

	operator float() const
	{
		//printf("converted to float\n");
		return floatingPt();
	}

	operator bool() const
	{
		return(val > 0);
	}

	Fixed<T,F,BIGGER> operator -() const
	{
		Fixed<T,F,BIGGER> neg;
		neg.val = -val;
		return neg;
	}

	Fixed<T,F,BIGGER> & operator +=(Fixed<T,F,BIGGER> const& summand)
	{
		val += summand.val;
		return *this;
	}

	Fixed<T,F,BIGGER> & operator -=(Fixed<T,F,BIGGER> const& diminuend)
	{
		val -= diminuend.val;
		return *this;
	}

	Fixed<T,F,BIGGER> & operator *=(Fixed<T,F,BIGGER> const& factor)
	{
		val = ( (BIGGER)(val)*(factor.val) + fracMaxValOneLessBit ) >> fracBits;
		return *this;
	}

	Fixed<T,F,BIGGER> & operator /=(Fixed<T,F,BIGGER> const& divisor)
	{
		val = ( ( ( (BIGGER)val << (fracBits+1) ) / divisor.val ) +1 ) /2;
		return *this;
	}

	bool operator <(Fixed<T,F,BIGGER> const& rhs) const
	{
		return val < rhs.val;
	}

	bool operator >(Fixed<T,F,BIGGER> const& rhs) const
	{
		return val > rhs.val;
	}

	bool operator ==(Fixed<T,F,BIGGER> const& rhs) const
	{
		return val == rhs.val;
	}
};

// TODO: test/fix ceil & floor with negative values
template<class T, unsigned char F, class BIGGER>
static Fixed<T,F,BIGGER> ceil(Fixed<T,F,BIGGER> x)
{
	Fixed<T,F,BIGGER> intPart(x.integer());
	if(x > intPart)
		return intPart + 1;
	else
		return intPart;
}

template<class T, unsigned char F, class BIGGER>
static Fixed<T,F,BIGGER> floor(Fixed<T,F,BIGGER> x)
{
	//logMsg("calling fixed floor");
	Fixed<T,F,BIGGER> intPart(x.integer());
	return intPart;
}

template<class T, unsigned char F, class BIGGER>
static Fixed<T,F,BIGGER> round(Fixed<T,F,BIGGER> x)
{
	return x.frac() >= Fixed<T,F,BIGGER>(0.5) ? ceil(x) : floor(x);
}

template<class T, unsigned char F, class BIGGER>
static Fixed<T,F,BIGGER> pow(Fixed<T,F,BIGGER> x, uint p)
{
	if(p == 0)
		return Fixed<T,F,BIGGER>(1);
	else if(p == 1)
		return x;

	Fixed<T,F,BIGGER> result = x;
	iterateTimes(p - 1, i)
	{
		result = result * x;
	}
	return result;
}

// F (fraction bits) Must be even for sqrt to function properly
template<class T, unsigned char F, class BIGGER>
static Fixed<T,F,BIGGER> sqrt(Fixed<T,F,BIGGER> x)
{
	assert(F & 1);
	T root, remHi, remLo, testDiv, count;

	root = 0;         // Clear root
	remHi = 0;        // Clear high part of partial remainder
	remLo = x;        // Get argument into low part of partial remainder
	count = 15 + (F >> 1);    // Load loop counter

	do
	{
		remHi = (remHi << 2) | (remLo >> 30);  // get 2 bits of arg
		remLo <<= 2;
		root <<= 1;   // Get ready for the next bit in the root
		testDiv = (root << 1) + 1;    // Test radical
		if (remHi >= testDiv)
		{
			remHi -= testDiv;
			root += 1;
		}
	} while (count-- != 0);

	return root;
}

typedef Fixed<int32, 16, int64> Fixed16S16;
typedef FixedPOD<int32> Fixed16S16POD;

}
