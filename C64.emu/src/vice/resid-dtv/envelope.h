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
//   Antti S. Lankila <alankila@bel.fi>
// Copyright (C) 2009  Antti S. Lankila <alankila@bel.fi>

#ifndef VICE__ENVELOPE_H__
#define VICE__ENVELOPE_H__

#include "residdtv-config.h"

#include "bittrain.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// A 15 bit counter is used to implement the envelope rates, in effect
// dividing the clock to the envelope counter by the currently selected rate
// period.
// In addition, another counter is used to implement the exponential envelope
// decay, in effect further dividing the clock to the envelope counter.
// The period of this counter is set to 1, 2, 4, 8, 16, 30 at the envelope
// counter values 255, 93, 54, 26, 14, 6, respectively.
// ----------------------------------------------------------------------------
class EnvelopeGenerator
{
public:
  EnvelopeGenerator();

  enum State { ATTACK, DECAY_SUSTAIN, RELEASE };

  RESID_INLINE void clock();
  void reset();

  void writeCONTROL_REG(reg8);
  void writeATTACK_DECAY(reg8);
  void writeSUSTAIN_RELEASE(reg8);
  reg8 readENV();
  void writeENV(reg8);

  RESID_INLINE unsigned int output();

protected:
  reg16 rate_counter;
  reg16 rate_period;
  reg8 exponential_counter;
  reg8 exponential_counter_period;
  reg8 envelope_counter;
  bool hold_zero;
  unsigned int envelope_train_counter;

  reg4 attack;
  reg4 decay;
  reg4 sustain;
  reg4 release;

  reg8 gate;

  State state;

  // Lookup table to convert from attack, decay, or release value to rate
  // counter period.
  static reg16 rate_counter_period[];

  // The 16 selectable sustain levels.
  static reg8 sustain_level[];

friend class SID;
};

// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__ENVELOPE_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGenerator::clock()
{
  envelope_train_counter += envelope_counter;
  envelope_train_counter &= 0x7;

  /* envelope always waits for the previous rate counter to trigger.
   * Therefore we formulate it differently from exponential counter. */
  if (-- rate_counter) {
    return;
  }
  rate_counter = rate_period;
  
  /* Envelope skips this wait during ATTACK.
   * This is handled during control register write: switching to attack
   * sets counter=0, period=1. Envelope uses this during decay and release,
   * with decay having 1/3 rate. */
  if (-- exponential_counter) {
    return;
  }
  exponential_counter = exponential_counter_period;

  if (state == ATTACK) {
    /* attack 0 is a special case: envelope simply jumps to 0xff. */
    if (attack == 0) {
      envelope_counter = 0xff;
    }
    if (envelope_counter != 0xff) {
      ++ envelope_counter;
    }
    if (envelope_counter == 0xff) {
      state = DECAY_SUSTAIN;

      /* decay rates are 1/3 of attack rates. If one now switches to attack,
       * DTVSID notices this after 1 rate_period has elapsed. The first decay
       * occurs at attack rate on DTV, presumably because the counter changes
       * are seen only after the currently set counters manage to elapse. */
      rate_period = rate_counter_period[decay];
      exponential_counter_period = 3;
    }
    return;
  }

  if (state == DECAY_SUSTAIN) {
    if (envelope_counter != 0 && envelope_counter != sustain_level[sustain]) {
      -- envelope_counter;
    }
    return;
  }

  if (state == RELEASE) {
    if (envelope_counter != 0) {
      -- envelope_counter;
    }
    exponential_counter_period = 8 - (envelope_counter >> 5);
    return;
  }
}

// ----------------------------------------------------------------------------
// Read the envelope generator output.
// ----------------------------------------------------------------------------
RESID_INLINE
unsigned int EnvelopeGenerator::output()
{
  return env_train_lut[envelope_counter][envelope_train_counter];
}

#endif // RESID_INLINING || defined(__ENVELOPE_CC__)

} // namespace reSID

#endif // not __ENVELOPE_H__
