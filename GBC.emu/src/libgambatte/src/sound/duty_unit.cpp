/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include "duty_unit.h"
#include <algorithm>

static inline bool toOutState(unsigned duty, unsigned pos) {
	static unsigned char const duties[4] = { 0x80, 0x81, 0xE1, 0x7E };
	return duties[duty] >> pos & 1;
}

static inline unsigned toPeriod(unsigned freq) {
	return (2048 - freq) * 2;
}

namespace gambatte {

DutyUnit::DutyUnit()
: nextPosUpdate_(counter_disabled)
, period_(4096)
, pos_(0)
, duty_(0)
, high_(false)
, enableEvents_(true)
{
}

void DutyUnit::updatePos(unsigned long const cc) {
	if (cc >= nextPosUpdate_) {
		unsigned long const inc = (cc - nextPosUpdate_) / period_ + 1;
		nextPosUpdate_ += period_ * inc;
		pos_ += inc;
		pos_ &= 7;
	}
}

void DutyUnit::setDuty(unsigned nr1) {
	duty_ = nr1 >> 6;
	high_ = toOutState(duty_, pos_);
}

void DutyUnit::setCounter() {
	static unsigned char const nextStateDistance[4 * 8] = {
		6, 5, 4, 3, 2, 1, 0, 0,
		0, 5, 4, 3, 2, 1, 0, 1,
		0, 3, 2, 1, 0, 3, 2, 1,
		0, 5, 4, 3, 2, 1, 0, 1
	};

	if (enableEvents_ && nextPosUpdate_ != counter_disabled)
		counter_ = nextPosUpdate_ + period_ * nextStateDistance[duty_ * 8 + pos_];
	else
		counter_ = counter_disabled;
}

void DutyUnit::setFreq(unsigned newFreq, unsigned long cc) {
	updatePos(cc);
	period_ = toPeriod(newFreq);
	setCounter();
}

void DutyUnit::event() {
	unsigned inc = period_ << duty_;

	if (duty_ == 3)
		inc -= period_ * 2;

	if (!(high_ ^= true))
		inc = period_ * 8 - inc;

	counter_ += inc;
}

void DutyUnit::nr1Change(unsigned newNr1, unsigned long cc) {
	updatePos(cc);
	setDuty(newNr1);
	setCounter();
}

void DutyUnit::nr3Change(unsigned newNr3, unsigned long cc) {
	setFreq((freq() & 0x700) | newNr3, cc);
}

void DutyUnit::nr4Change(unsigned const newNr4, unsigned long const cc) {
	setFreq((newNr4 << 8 & 0x700) | (freq() & 0xFF), cc);

	if (newNr4 & 0x80) {
		nextPosUpdate_ = (cc & ~1) + period_;
		setCounter();
	}
}

void DutyUnit::reset() {
	pos_ = 0;
	high_ = toOutState(duty_, pos_);
	nextPosUpdate_ = counter_disabled;
	setCounter();
}

void DutyUnit::saveState(SaveState::SPU::Duty &dstate, unsigned long const cc) {
	updatePos(cc);
	dstate.nextPosUpdate = nextPosUpdate_;
	dstate.nr3 = freq() & 0xFF;
	dstate.pos = pos_;
}

void DutyUnit::loadState(const SaveState::SPU::Duty &dstate,
		unsigned const nr1, unsigned const nr4, unsigned long const cc) {
	nextPosUpdate_ = std::max(dstate.nextPosUpdate, cc);
	pos_ = dstate.pos & 7;
	setDuty(nr1);
	period_ = toPeriod((nr4 << 8 & 0x700) | dstate.nr3);
	enableEvents_ = true;
	setCounter();
}

void DutyUnit::resetCounters(unsigned long const oldCc) {
	if (nextPosUpdate_ == counter_disabled)
		return;

	updatePos(oldCc);
	nextPosUpdate_ -= counter_max;
	SoundUnit::resetCounters(oldCc);
}

void DutyUnit::killCounter() {
	enableEvents_ = false;
	setCounter();
}

void DutyUnit::reviveCounter(unsigned long const cc) {
	updatePos(cc);
	high_ = toOutState(duty_, pos_);
	enableEvents_ = true;
	setCounter();
}

}
