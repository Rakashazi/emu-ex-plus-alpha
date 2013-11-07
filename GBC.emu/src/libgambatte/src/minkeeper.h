/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#ifndef MINKEEPER_H
#define MINKEEPER_H

#include <algorithm>

namespace MinKeeperUtil {
template<int n> struct CeiledLog2 { enum { r = 1 + CeiledLog2<(n + 1) / 2>::r }; };
template<> struct CeiledLog2<1> { enum { r = 0 }; };

template<int v, int n> struct RoundedDiv2n { enum { r = RoundedDiv2n<(v + 1) / 2, n - 1>::r }; };
template<int v> struct RoundedDiv2n<v,1> { enum { r = v }; };

template<template<int> class T, int n> struct Sum { enum { r = T<n-1>::r + Sum<T, n-1>::r }; };
template<template<int> class T> struct Sum<T,0> { enum { r = 0 }; };
}

// Keeps track of minimum value identified by id as values change.
// Higher ids prioritized (as min value) if values are equal. Can easily be reversed by swapping < for <=.
// Higher ids can be faster to change when the number of ids isn't a power of 2.
// Thus the ones that change more frequently should have higher ids if priority allows it.
template<int ids>
class MinKeeper {
public:
	explicit MinKeeper(unsigned long initValue = 0xFFFFFFFF);
	int min() const { return a_[0]; }
	unsigned long minValue() const { return minValue_; }

	template<int id>
	void setValue(unsigned long cnt) {
		values_[id] = cnt;
		updateValue<id / 2>(*this);
	}

	void setValue(int id, unsigned long cnt) {
		values_[id] = cnt;
		updateValueLut.call(id >> 1, *this);
	}

	unsigned long value(int id) const { return values_[id]; }

private:
	enum { levels = MinKeeperUtil::CeiledLog2<ids>::r };
	template<int l> struct Num { enum { r = MinKeeperUtil::RoundedDiv2n<ids, levels + 1 - l>::r }; };
	template<int l> struct Sum { enum { r = MinKeeperUtil::Sum<Num, l>::r }; };

	template<int id, int level>
	struct UpdateValue {
		enum { p = Sum<level-1>::r + id };
		enum { c0 = Sum<level>::r + id * 2 };

		static void updateValue(MinKeeper<ids> &m) {
			m.a_[p] = id * 2 + 1 == Num<level>::r || m.values_[m.a_[c0]] < m.values_[m.a_[c0 + 1]]
			        ? m.a_[c0    ]
			        : m.a_[c0 + 1];
			UpdateValue<id / 2, level - 1>::updateValue(m);
		}
	};

	template<int id>
	struct UpdateValue<id,0> {
		static void updateValue(MinKeeper<ids> &m) {
			m.minValue_ = m.values_[m.a_[0]];
		}
	};

	class UpdateValueLut {
		template<int id, int dummy> struct FillLut {
			static void fillLut(UpdateValueLut & l) {
				l.lut_[id] = updateValue<id>;
				FillLut<id-1,dummy>::fillLut(l);
			}
		};

		template<int dummy> struct FillLut<-1,dummy> {
			static void fillLut(UpdateValueLut &) {}
		};

		void (*lut_[Num<levels-1>::r])(MinKeeper<ids>&);

	public:
		UpdateValueLut() { FillLut<Num<levels-1>::r-1,0>::fillLut(*this); }
		void call(int id, MinKeeper<ids> &mk) const { lut_[id](mk); }
	};

	static UpdateValueLut updateValueLut;
	unsigned long values_[ids];
	unsigned long minValue_;
	int a_[Sum<levels>::r];

	template<int id> static void updateValue(MinKeeper<ids> &m);
};

template<int ids> typename MinKeeper<ids>::UpdateValueLut MinKeeper<ids>::updateValueLut;

template<int ids>
MinKeeper<ids>::MinKeeper(unsigned long const initValue) {
	std::fill(values_, values_ + ids, initValue);

	for (int i = 0; i < Num<levels-1>::r; ++i) {
		a_[Sum<levels-1>::r + i] = i * 2 + 1 == ids || values_[i * 2] < values_[i * 2 + 1]
		                         ? i * 2
		                         : i * 2 + 1;
	}

	int n   = Num<levels-1>::r;
	int off = Sum<levels-1>::r;

	while (off) {
		int const pn = (n + 1) >> 1;
		int const poff = off - pn;

		for (int i = 0; i < pn; ++i) {
			a_[poff + i] =
				  i * 2 + 1 == n || values_[a_[off + i * 2]] < values_[a_[off + i * 2 + 1]]
				? a_[off + i * 2    ]
				: a_[off + i * 2 + 1];
		}

		off = poff;
		n   = pn;
	}

	minValue_ = values_[a_[0]];
}

template<int ids>
template<int id>
void MinKeeper<ids>::updateValue(MinKeeper<ids> &m) {
	m.a_[Sum<levels-1>::r + id] = id * 2 + 1 == ids || m.values_[id * 2] < m.values_[id * 2 + 1]
	                            ? id * 2
	                            : id * 2 + 1;
	UpdateValue<id / 2, levels-1>::updateValue(m);
}

#endif
