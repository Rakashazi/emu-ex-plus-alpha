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
#include <util/operators.hh>

// NormalInt scales floats to/from 0-1 when constructed/converted
template<class T, unsigned int S>
class NormalInt : NotEquals< NormalInt<T,S> >,
	Arithmetics< NormalInt<T,S> >
{
public:
	T val;

	constexpr NormalInt() { }
	constexpr NormalInt(const NormalInt<T,S> &num) : val(num.val) {  }

	constexpr NormalInt(float num) : val(num * (T)S) { }
	constexpr NormalInt(double num) : val(num * (T)S) { }
	constexpr NormalInt(int num) : val(num) { }
	constexpr NormalInt(unsigned int num) : val(num) { }
	constexpr NormalInt(short num) : val(num) { }
	constexpr NormalInt(char num) : val(num) { }

	operator float() const { return val / (T)S; }
	operator double() const { return val / (T)S; }
	operator int() const { return val; }
	operator short() const { return val; }
	operator char() const { return val; }

	NormalInt<T,S> & operator +=(NormalInt<T,S> const& summand)
	{
		val += summand.val;
		return *this;
	}

	NormalInt<T,S> & operator -=(NormalInt<T,S> const& diminuend)
	{
		val -= diminuend.val;
		return *this;
	}

	NormalInt<T,S> & operator *=(NormalInt<T,S> const& factor)
	{
		val *= factor.val;
		return *this;
	}

	NormalInt<T,S> & operator /=(NormalInt<T,S> const& divisor)
	{
		val /= divisor.val;
		return *this;
	}

	bool operator <(NormalInt<T,S> const& rhs) const { return val < rhs.val; }
	bool operator <(T const& rhs) const { return val < (NormalInt<T,S>)rhs; }
	bool operator >(NormalInt<T,S> const& rhs) const { return val > rhs.val; }
	bool operator >(T const& rhs) const { return val > (NormalInt<T,S>)rhs; }

	bool operator ==(NormalInt<T,S> const& rhs) const
	{
		return val == rhs.val;
	}
};

template<unsigned int S>
class NormalFloat : NotEquals< NormalFloat<S> >,
	Arithmetics< NormalFloat<S> >
{
public:
	float val;

	constexpr NormalFloat() { }
	constexpr NormalFloat(const NormalFloat<S> &num) : val(num.val) {  }

	constexpr NormalFloat(float num) : val(num) { }
	constexpr NormalFloat(double num) : val(num) { }
	constexpr NormalFloat(int num) : val((float)num / (float)S) { }
	constexpr NormalFloat(unsigned int num) : val((float)num / (float)S) { }
	constexpr NormalFloat(short num) : NormalFloat((int)num) { }
	constexpr NormalFloat(char num) : NormalFloat((int)num) { }

	operator float() const { return val; }
	operator double() const { return val; }
	operator int() const { return val * (float)S; }
	operator unsigned int() const { return val * (float)S; }
	operator short() const { return val * (float)S; }
	operator char() const { return val * (float)S; }
	template<class T2>
	operator NormalInt<T2,S>() const { return (int)val * (float)S; }

	NormalFloat<S> & operator +=(NormalFloat<S> const& summand)
	{
		val += summand.val;
		return *this;
	}

	NormalFloat<S> & operator -=(NormalFloat<S> const& diminuend)
	{
		val -= diminuend.val;
		return *this;
	}

	NormalFloat<S> & operator *=(NormalFloat<S> const& factor)
	{
		val *= factor.val;
		return *this;
	}

	NormalFloat<S> & operator /=(NormalFloat<S> const& divisor)
	{
		val /= divisor.val;
		return *this;
	}

	bool operator <(NormalFloat<S> const& rhs) const { return val < rhs.val; }
	bool operator <(float const& rhs) const { return val < (NormalFloat<S>)rhs; }
	bool operator >(NormalFloat<S> const& rhs) const { return val > rhs.val; }
	bool operator >(float const& rhs) const { return val > (NormalFloat<S>)rhs; }

	bool operator ==(NormalFloat<S> const& rhs) const
	{
		return val == rhs.val;
	}

	NormalFloat<S> operator -() const
	{
		NormalFloat<S> result;
		result.val = -val;
		return result;
	}
};
