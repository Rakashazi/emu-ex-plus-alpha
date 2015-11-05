//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------
// C64 DTV modifications written by
//   Daniel Kahlin <daniel@kahlin.net>
// Copyright (C) 2007  Daniel Kahlin <daniel@kahlin.net>
//   Hannu Nuotio <hannu.nuotio@tut.fi>
// Copyright (C) 2009  Hannu Nuotio <hannu.nuotio@tut.fi>

#define __ENVELOPE_CC__
#include "envelope.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
EnvelopeGenerator::EnvelopeGenerator()
{
  reset();
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void EnvelopeGenerator::reset()
{
  envelope_counter = 0;
  envelope_train_counter = 0;

  attack = 0;
  decay = 0;
  sustain = 0;
  release = 0;

  gate = 0;

  exponential_counter = exponential_counter_period = 1;

  state = RELEASE;
  rate_counter = rate_period = rate_counter_period[release];
}


// DTVSID uses the following table for attack, decay and release rates.
// Attack = 0 results in immediate 0x00 -> 0xff (handled elsewhere).
// Decay is linear and the rate is 3*the corresponding attack rate.
// Release rate increases with the base rate on envelope values $de, $be, $9e...
// These values are measured by polling ENV3 with DMA.
//
reg16 EnvelopeGenerator::rate_counter_period[] = {
    8,
    24,
    40,
    56,
    80,
    120,
    144,
    176,
    208,
    504,
    1008,
    2032,
    2040,
    6008,
    10008,
    16024
};


// The DTVSID sustain levels. These have been verified by sampling ENV3.
//
reg8 EnvelopeGenerator::sustain_level[] = {
  0x00,
  0x1f,
  0x20,
  0x3f,
  0x40,
  0x5f,
  0x60,
  0x7f,
  0x80,
  0x9f,
  0xa0,
  0xbf,
  0xc0,
  0xdf,
  0xe0,
  0xff,
};


// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void EnvelopeGenerator::writeCONTROL_REG(reg8 control)
{
  reg8 gate_next = control & 0x01;

  // The rate counter is never reset, thus there will be a delay before the
  // envelope counter starts counting up (attack) or down (release).

  // Gate bit on: Start attack, decay, sustain.
  if (!gate && gate_next) {
    state = ATTACK;
    rate_period = rate_counter_period[attack];

    /* Attacks are delayed by 1 decay period.
     * Turning on attack ignores even the current release interval, indicating
     * that somehow shifting to attack overwrites the counter. & 7 is for
     * keeping 8-sync intact. */
    rate_counter = (rate_counter & 7) + rate_counter_period[decay];
    exponential_counter = 1;
    exponential_counter_period = 1;
  }
  // Gate bit off: Start release.
  else if (gate && !gate_next) {
    state = RELEASE;
    rate_period = rate_counter_period[release];
    /* Envelope appears to wait one extra release period before doing stuff.
     * Every release seems to occur at same rate, indicating the value of
     * exponential_counter_period is always kept up-to-date. */
    exponential_counter_period = 8 - (envelope_counter >> 5);
  }

  gate = gate_next;
}

void EnvelopeGenerator::writeATTACK_DECAY(reg8 attack_decay)
{
  attack = (attack_decay >> 4) & 0x0f;
  decay = attack_decay & 0x0f;
  if (state == ATTACK) {
    rate_period = rate_counter_period[attack];
  }
  else if (state == DECAY_SUSTAIN) {
    rate_period = rate_counter_period[decay];
  }
}

void EnvelopeGenerator::writeSUSTAIN_RELEASE(reg8 sustain_release)
{
  sustain = (sustain_release >> 4) & 0x0f;
  release = sustain_release & 0x0f;
  if (state == RELEASE) {
    rate_period = rate_counter_period[release];
  }
}

reg8 EnvelopeGenerator::readENV()
{
  return envelope_counter;
}

void EnvelopeGenerator::writeENV(reg8 value)
{
  envelope_counter = value;
}

} // namespace reSID
