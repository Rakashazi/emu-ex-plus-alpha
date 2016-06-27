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

#ifndef VICE__WAVE_H__
#define VICE__WAVE_H__

#include "residdtv-config.h"

#include "bittrain.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// A 24 bit accumulator is the basis for waveform generation. FREQ is added to
// the lower 16 bits of the accumulator each cycle.
// The accumulator is set to zero when TEST is set, and starts counting
// when TEST is cleared.
// The noise waveform is taken from intermediate bits of a 23 bit shift
// register. This register is clocked by bit 19 of the accumulator.
// ----------------------------------------------------------------------------
class WaveformGenerator
{
public:
  WaveformGenerator();

  void set_sync_source(WaveformGenerator*);

  RESID_INLINE void clock();
  RESID_INLINE void synchronize();
  void reset();

  void writeFREQ_LO(reg8);
  void writeFREQ_HI(reg8);
  void writePW_LO(reg8);
  void writePW_HI(reg8);
  void writeCONTROL_REG(reg8);
  reg8 readOSC();
  void writeACC_HI(reg8);

  RESID_INLINE unsigned int output();

protected:
  RESID_INLINE void clock_noise();

  const WaveformGenerator* sync_source;
  WaveformGenerator* sync_dest;

  // Tell whether the accumulator MSB was set high on this cycle.
  bool msb_rising;

  reg24 accumulator, counter;
  reg24 shift_register;
  reg12 noise;

  // Fout  = (Fn*Fclk/16777216)Hz
  reg16 freq;
  // PWout = (PWn/40.95)%
  reg12 pw;

  // The control register right-shifted 4 bits; used for output function
  // table lookup.
  reg8 waveform;

  // The remaining control register bits.
  reg8 test;
  reg8 ring_mod;
  reg8 sync;
  // The gate bit is handled by the EnvelopeGenerator.

  // 16 possible combinations of waveforms.
  RESID_INLINE reg8 output___T();
  RESID_INLINE reg8 output__S_();
  RESID_INLINE reg8 output_P__();
  RESID_INLINE reg8 outputN___();

friend class Voice;
friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__WAVE_CC__)

RESID_INLINE
void WaveformGenerator::clock_noise()
{
  reg24 bit0 = ((shift_register >> 22) ^ (shift_register >> 17)) & 0x1;
  shift_register <<= 1;
  shift_register |= bit0;
    
  noise =
    ((shift_register & 0x400000) >> 11) |
    ((shift_register & 0x100000) >> 10) |
    ((shift_register & 0x010000) >> 7) |
    ((shift_register & 0x002000) >> 5) |
    ((shift_register & 0x000800) >> 4) |
    ((shift_register & 0x000080) >> 1) |
    ((shift_register & 0x000010) << 1) |
    ((shift_register & 0x000007) << 2);
  noise |= bit0 ? 0x3 : 0x0;
}

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::clock()
{
  // No operation if test bit is set.
  if (test) {
    return;
  }

  reg24 accumulator_prev = accumulator;

  // Calculate new accumulator value;
  accumulator += freq;
  accumulator &= 0xffffff;

  // Check whether the MSB is set high. This is used for synchronization.
  msb_rising = !(accumulator_prev & 0x800000) && (accumulator & 0x800000);

  // Shift noise register once for each time accumulator bit 19 is set high.
  if (!(accumulator_prev & 0x080000) && (accumulator & 0x080000)) {
    clock_noise();
  }
}

// ----------------------------------------------------------------------------
// Synchronize oscillators.
// This must be done after all the oscillators have been clock()'ed since the
// oscillators operate in parallel.
// Note that the oscillators must be clocked exactly on the cycle when the
// MSB is set high for hard sync to operate correctly. See SID::clock().
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::synchronize()
{
  // A special case occurs when a sync source is synced itself on the same
  // cycle as when its MSB is set high. In this case the destination will
  // not be synced. This has been verified by sampling OSC3.
  if (msb_rising && sync_dest->sync && !(sync && sync_source->msb_rising)) {
    sync_dest->accumulator = 0;
  }
}

// Triangle:
// The upper 12 bits of the accumulator are used.
// The MSB is used to create the falling edge of the triangle by inverting
// the lower 11 bits. The MSB is thrown away and the lower 11 bits are
// left-shifted (half the resolution, full amplitude).
// Ring modulation substitutes the MSB with sync_source MSB.
//
RESID_INLINE
reg8 WaveformGenerator::output___T()
{
  reg24 msb = (ring_mod ? sync_source->accumulator : accumulator) & 0x800000;
  return ((msb ? ~accumulator : accumulator) >> 11) & 0xfff;
}

// Sawtooth:
// The output is identical to the upper 12 bits of the accumulator.
//
RESID_INLINE
reg8 WaveformGenerator::output__S_()
{
  return accumulator >> 12;
}

// Pulse:
// The upper 12 bits of the accumulator are used.
// These bits are compared to the pulse width register by a 12 bit digital
// comparator; output is either all one or all zero bits.
// NB! The output is actually delayed one cycle after the compare.
// This is not modeled.
//
// The test bit, when set to one, holds the pulse waveform output at 0xfff
// regardless of the pulse width setting.
//
RESID_INLINE
reg8 WaveformGenerator::output_P__()
{
  return (accumulator >> 12) >= pw ? 0xfff : 0x00;
}

// Noise:
// The noise output is taken from intermediate bits of a 23-bit shift register
// which is clocked by bit 19 of the accumulator.
// NB! The output is actually delayed 2 cycles after bit 19 is set high.
// This is not modeled.
//
// Operation: Calculate EOR result, shift register, set bit 0 = result.
//
//                        ----------------------->---------------------
//                        |                                            |
//                   ----EOR----                                       |
//                   |         |                                       |
//                   2 2 2 1 1 1 1 1 1 1 1 1 1                         |
// Register bits:    2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 <---
//                   |   |       |     |   |       |     |   |
// OSC3 bits  :      7   6       5     4   3       2     1   0
//
// Since waveform output is 12 bits the output is left-shifted 4 times.
//
RESID_INLINE
reg8 WaveformGenerator::outputN___()
{
  return noise;
}

// ----------------------------------------------------------------------------
// Select one of 16 possible combinations of waveforms.
// ----------------------------------------------------------------------------
RESID_INLINE
unsigned int WaveformGenerator::output()
{
  reg12 output = 0;
  if (waveform & 0x1)
    output |= output___T();
  if (waveform & 0x2)
    output |= output__S_();
  if (waveform & 0x4)
    output |= output_P__();
  if (waveform & 0x8)
    output |= outputN___();

  counter += output;
  counter &= 0x7f;
  return wave_train_lut[output][counter];
}

#endif // RESID_INLINING || defined(__WAVE_CC__)

} // namespace reSID

#endif // not __WAVE_H__
