//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2010  Dag Lem <resid@nimrod.no>
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

#ifndef RESID_WAVE_H
#define RESID_WAVE_H

#include "resid-config.h"

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
  void set_chip_model(chip_model model);

  void clock();
  void clock(cycle_count delta_t);
  void synchronize();
  void reset();

  void writeFREQ_LO(reg8);
  void writeFREQ_HI(reg8);
  void writePW_LO(reg8);
  void writePW_HI(reg8);
  void writeCONTROL_REG(reg8);
  reg8 readOSC();

  // 12-bit waveform output.
  short output();

  // Calculate and set waveform output value.
  void set_waveform_output();
  void set_waveform_output(cycle_count delta_t);

protected:
  void clock_shift_register();
  void write_shift_register();
  void reset_shift_register();
  void set_noise_output();

  const WaveformGenerator* sync_source;
  WaveformGenerator* sync_dest;

  reg24 accumulator;

  // Tell whether the accumulator MSB was set high on this cycle.
  bool msb_rising;

  // Fout  = (Fn*Fclk/16777216)Hz
  // reg16 freq;
  reg24 freq;
  // PWout = (PWn/40.95)%
  reg12 pw;

  reg24 shift_register;

  // Remaining time to fully reset shift register.
  cycle_count shift_register_reset;
  // Emulation of pipeline causing bit 19 to clock the shift register.
  cycle_count shift_pipeline;

  // Helper variables for waveform table lookup.
  reg24 ring_msb_mask;
  unsigned short no_noise;
  unsigned short noise_output;
  unsigned short no_noise_or_noise_output;
  unsigned short no_pulse;
  unsigned short pulse_output;

  // The control register right-shifted 4 bits; used for waveform table lookup.
  reg8 waveform;

  // 8580 tri/saw pipeline
  reg12 tri_saw_pipeline;
  reg12 osc3;

  // The remaining control register bits.
  reg8 test;
  reg8 ring_mod;
  reg8 sync;
  // The gate bit is handled by the EnvelopeGenerator.

  // DAC input.
  reg12 waveform_output;
  // Fading time for floating DAC input (waveform 0).
  cycle_count floating_output_ttl;

  chip_model sid_model;

  // Sample data for waveforms, not including noise.
  unsigned short* wave;
  static unsigned short model_wave[2][8][1 << 12];
  // DAC lookup tables.
  static unsigned short model_dac[2][1 << 12];

friend class Voice;
friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(RESID_WAVE_CC)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::clock()
{
  if (unlikely(test)) {
    // Count down time to fully reset shift register.
    if (unlikely(shift_register_reset) && unlikely(!--shift_register_reset)) {
      reset_shift_register();
    }

    // The test bit sets pulse high.
    pulse_output = 0xfff;
  }
  else {
    // Calculate new accumulator value;
    reg24 accumulator_next = (accumulator + freq) & 0xffffff;
    reg24 accumulator_bits_set = ~accumulator & accumulator_next;
    accumulator = accumulator_next;

    // Check whether the MSB is set high. This is used for synchronization.
    msb_rising = (accumulator_bits_set & 0x800000) ? true : false;

    // Shift noise register once for each time accumulator bit 19 is set high.
    // The shift is delayed 2 cycles.
    if (unlikely(accumulator_bits_set & 0x080000)) {
      // Pipeline: Detect rising bit, shift phase 1, shift phase 2.
      shift_pipeline = 2;
    }
    else if (unlikely(shift_pipeline) && !--shift_pipeline) {
      clock_shift_register();
    }
  }
}

// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::clock(cycle_count delta_t)
{
  if (unlikely(test)) {
    // Count down time to fully reset shift register.
    if (shift_register_reset) {
      shift_register_reset -= delta_t;
      if (unlikely(shift_register_reset <= 0)) {
        reset_shift_register();
      }
    }

    // The test bit sets pulse high.
    pulse_output = 0xfff;
  }
  else {
    // Calculate new accumulator value;
    reg24 delta_accumulator = delta_t*freq;
    reg24 accumulator_next = (accumulator + delta_accumulator) & 0xffffff;
    reg24 accumulator_bits_set = ~accumulator & accumulator_next;
    accumulator = accumulator_next;

    // Check whether the MSB is set high. This is used for synchronization.
    msb_rising = (accumulator_bits_set & 0x800000) ? true : false;

    // NB! Any pipelined shift register clocking from single cycle clocking
    // will be lost. It is not worth the trouble to flush the pipeline here.

    // Shift noise register once for each time accumulator bit 19 is set high.
    // Bit 19 is set high each time 2^20 (0x100000) is added to the accumulator.
    reg24 shift_period = 0x100000;

    while (delta_accumulator) {
      if (likely(delta_accumulator < shift_period)) {
        shift_period = delta_accumulator;
        // Determine whether bit 19 is set on the last period.
        // NB! Requires two's complement integer.
        if (likely(shift_period <= 0x080000)) {
          // Check for flip from 0 to 1.
          if (((accumulator - shift_period) & 0x080000) || !(accumulator & 0x080000))
            {
              break;
            }
        }
        else {
          // Check for flip from 0 (to 1 or via 1 to 0) or from 1 via 0 to 1.
          if (((accumulator - shift_period) & 0x080000) && !(accumulator & 0x080000))
            {
              break;
            }
        }
      }

      // Shift the noise/random register.
      // NB! The two-cycle pipeline delay is only modeled for 1 cycle clocking.
      clock_shift_register();

      delta_accumulator -= shift_period;
    }

    // Calculate pulse high/low.
    // NB! The one-cycle pipeline delay is only modeled for 1 cycle clocking.
    pulse_output = (accumulator >> 12) >= pw ? 0xfff : 0x000;
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
  if (unlikely(msb_rising) && sync_dest->sync && !(sync && sync_source->msb_rising)) {
    sync_dest->accumulator = 0;
  }
}


// ----------------------------------------------------------------------------
// Waveform output.
// The output from SID 8580 is delayed one cycle compared to SID 6581;
// this is only modeled for single cycle clocking (see sid.cc).
// ----------------------------------------------------------------------------

// No waveform:
// When no waveform is selected, the DAC input is floating.
//

// Triangle:
// The upper 12 bits of the accumulator are used.
// The MSB is used to create the falling edge of the triangle by inverting
// the lower 11 bits. The MSB is thrown away and the lower 11 bits are
// left-shifted (half the resolution, full amplitude).
// Ring modulation substitutes the MSB with MSB EOR NOT sync_source MSB.
//

// Sawtooth:
// The output is identical to the upper 12 bits of the accumulator.
//

// Pulse:
// The upper 12 bits of the accumulator are used.
// These bits are compared to the pulse width register by a 12 bit digital
// comparator; output is either all one or all zero bits.
// The pulse setting is delayed one cycle after the compare; this is only
// modeled for single cycle clocking.
//
// The test bit, when set to one, holds the pulse waveform output at 0xfff
// regardless of the pulse width setting.
//

// Noise:
// The noise output is taken from intermediate bits of a 23-bit shift register
// which is clocked by bit 19 of the accumulator.
// The shift is delayed 2 cycles after bit 19 is set high; this is only
// modeled for single cycle clocking.
//
// Operation: Calculate EOR result, shift register, set bit 0 = result.
//
//                reset    -------------------------------------------
//                  |     |                                           |
//           test--OR-->EOR<--                                        |
//                  |         |                                       |
//                  2 2 2 1 1 1 1 1 1 1 1 1 1                         |
// Register bits:   2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 <---
//                      |   |       |     |   |       |     |   |
// Waveform bits:       1   1       9     8   7       6     5   4
//                      1   0
//
// The low 4 waveform bits are zero (grounded).
//

RESID_INLINE void WaveformGenerator::clock_shift_register()
{
  // bit0 = (bit22 | test) ^ bit17
  reg24 bit0 = ((shift_register >> 22) ^ (shift_register >> 17)) & 0x1;
  shift_register = ((shift_register << 1) | bit0) & 0x7fffff;

  // New noise waveform output.
  set_noise_output();
}

RESID_INLINE void WaveformGenerator::write_shift_register()
{
  // Write changes to the shift register output caused by combined waveforms
  // back into the shift register.
  // A bit once set to zero cannot be changed, hence the and'ing.
  // FIXME: Write test program to check the effect of 1 bits and whether
  // neighboring bits are affected.

  shift_register &=
    ~((1<<20)|(1<<18)|(1<<14)|(1<<11)|(1<<9)|(1<<5)|(1<<2)|(1<<0)) |
    ((waveform_output & 0x800) << 9) |  // Bit 11 -> bit 20
    ((waveform_output & 0x400) << 8) |  // Bit 10 -> bit 18
    ((waveform_output & 0x200) << 5) |  // Bit  9 -> bit 14
    ((waveform_output & 0x100) << 3) |  // Bit  8 -> bit 11
    ((waveform_output & 0x080) << 2) |  // Bit  7 -> bit  9
    ((waveform_output & 0x040) >> 1) |  // Bit  6 -> bit  5
    ((waveform_output & 0x020) >> 3) |  // Bit  5 -> bit  2
    ((waveform_output & 0x010) >> 4);   // Bit  4 -> bit  0

  noise_output &= waveform_output;
  no_noise_or_noise_output = no_noise | noise_output;
}

RESID_INLINE void WaveformGenerator::reset_shift_register()
{
  shift_register = 0x7fffff;
  shift_register_reset = 0;

  // New noise waveform output.
  set_noise_output();
}

RESID_INLINE void WaveformGenerator::set_noise_output()
{
  noise_output =
    ((shift_register & 0x100000) >> 9) |
    ((shift_register & 0x040000) >> 8) |
    ((shift_register & 0x004000) >> 5) |
    ((shift_register & 0x000800) >> 3) |
    ((shift_register & 0x000200) >> 2) |
    ((shift_register & 0x000020) << 1) |
    ((shift_register & 0x000004) << 3) |
    ((shift_register & 0x000001) << 4);

  no_noise_or_noise_output = no_noise | noise_output;
}

// Combined waveforms:
// By combining waveforms, the bits of each waveform are effectively short
// circuited. A zero bit in one waveform will result in a zero output bit
// (thus the infamous claim that the waveforms are AND'ed).
// However, a zero bit in one waveform may also affect the neighboring bits
// in the output.
//
// Example:
// 
//             1 1
// Bit #       1 0 9 8 7 6 5 4 3 2 1 0
//             -----------------------
// Sawtooth    0 0 0 1 1 1 1 1 1 0 0 0
//
// Triangle    0 0 1 1 1 1 1 1 0 0 0 0
//
// AND         0 0 0 1 1 1 1 1 0 0 0 0
//
// Output      0 0 0 0 1 1 1 0 0 0 0 0
//
//
// Re-vectorized die photographs reveal the mechanism behind this behavior.
// Each waveform selector bit acts as a switch, which directly connects
// internal outputs into the waveform DAC inputs as follows:
//
// * Noise outputs the shift register bits to DAC inputs as described above.
//   Each output is also used as input to the next bit when the shift register
//   is shifted.
// * Pulse connects a single line to all DAC inputs. The line is connected to
//   either 5V (pulse on) or 0V (pulse off) at bit 11, and ends at bit 0.
// * Triangle connects the upper 11 bits of the (MSB EOR'ed) accumulator to the
//   DAC inputs, so that DAC bit 0 = 0, DAC bit n = accumulator bit n - 1.
// * Sawtooth connects the upper 12 bits of the accumulator to the DAC inputs,
//   so that DAC bit n = accumulator bit n. Sawtooth blocks out the MSB from
//   the EOR used to generate the triangle waveform.
//
// We can thus draw the following conclusions:
//
// * The shift register may be written to by combined waveforms.
// * The pulse waveform interconnects all bits in combined waveforms via the
//   pulse line.
// * The combination of triangle and sawtooth interconnects neighboring bits
//   of the sawtooth waveform.
//
// This behavior would be quite difficult to model exactly, since the short
// circuits are not binary, but are subject to analog effects. Tests show that
// minor (1 bit) differences can actually occur in the output from otherwise
// identical samples from OSC3 when waveforms are combined. To further
// complicate the situation the output changes slightly with time (more
// neighboring bits are successively set) when the 12-bit waveform
// registers are kept unchanged.
//
// The output is instead approximated by using the upper bits of the
// accumulator as an index to look up the combined output in a table
// containing actual combined waveform samples from OSC3.
// These samples are 8 bit, so 4 bits of waveform resolution is lost.
// All OSC3 samples are taken with FREQ=0x1000, adding a 1 to the upper 12
// bits of the accumulator each cycle for a sample period of 4096 cycles.
//
// Sawtooth+Triangle:
// The accumulator is used to look up an OSC3 sample.
// 
// Pulse+Triangle:
// The accumulator is used to look up an OSC3 sample. When ring modulation is
// selected, the accumulator MSB is substituted with MSB EOR NOT sync_source MSB.
// 
// Pulse+Sawtooth:
// The accumulator is used to look up an OSC3 sample.
// The sample is output if the pulse output is on.
//
// Pulse+Sawtooth+Triangle:
// The accumulator is used to look up an OSC3 sample.
// The sample is output if the pulse output is on.
// 
// Combined waveforms including noise:
// All waveform combinations including noise output zero after a few cycles,
// since the waveform bits are and'ed into the shift register via the shift
// register outputs.

RESID_INLINE
void WaveformGenerator::set_waveform_output()
{
  // Set output value.
  if (likely(waveform)) {
    // The bit masks no_pulse and no_noise are used to achieve branch-free
    // calculation of the output value.
    int ix = (accumulator ^ (~sync_source->accumulator & ring_msb_mask)) >> 12;

    waveform_output = wave[ix] & (no_pulse | pulse_output) & no_noise_or_noise_output;

    // Triangle/Sawtooth output is delayed half cycle on 8580.
    // This will appear as a one cycle delay on OSC3 as it is
    // latched in the first phase of the clock.
    if ((waveform & 3) && (sid_model == MOS8580))
    {
        osc3 = tri_saw_pipeline & (no_pulse | pulse_output) & no_noise_or_noise_output;
        tri_saw_pipeline = wave[ix];
    }
    else
    {
        osc3 = waveform_output;
    }

    if ((waveform & 0x2) && unlikely(waveform & 0xd) && (sid_model == MOS6581)) {
        // In the 6581 the top bit of the accumulator may be driven low by combined waveforms
        // when the sawtooth is selected
        accumulator &= (waveform_output << 12) | 0x7fffff;
    }

    if (unlikely(waveform > 0x8) && likely(!test) && likely(shift_pipeline != 1)) {
      // Combined waveforms write to the shift register.
      write_shift_register();
    }
  }
  else {
    // Age floating DAC input.
    if (likely(floating_output_ttl) && unlikely(!--floating_output_ttl)) {
      waveform_output = 0;
    }
  }

  // The pulse level is defined as (accumulator >> 12) >= pw ? 0xfff : 0x000.
  // The expression -((accumulator >> 12) >= pw) & 0xfff yields the same
  // results without any branching (and thus without any pipeline stalls).
  // NB! This expression relies on that the result of a boolean expression
  // is either 0 or 1, and furthermore requires two's complement integer.
  // A few more cycles may be saved by storing the pulse width left shifted
  // 12 bits, and dropping the and with 0xfff (this is valid since pulse is
  // used as a bit mask on 12 bit values), yielding the expression
  // -(accumulator >= pw24). However this only results in negligible savings.

  // The result of the pulse width compare is delayed one cycle.
  // Push next pulse level into pulse level pipeline.
  pulse_output = -((accumulator >> 12) >= pw) & 0xfff;
}

RESID_INLINE
void WaveformGenerator::set_waveform_output(cycle_count delta_t)
{
  // Set output value.
  if (likely(waveform)) {
    // The bit masks no_pulse and no_noise are used to achieve branch-free
    // calculation of the output value.
    int ix = (accumulator ^ (~sync_source->accumulator & ring_msb_mask)) >> 12;
    waveform_output =
      wave[ix] & (no_pulse | pulse_output) & no_noise_or_noise_output;
    // Triangle/Sawtooth output delay for the 8580 is not modeled
    osc3 = waveform_output;

    if ((waveform & 0x2) && unlikely(waveform & 0xd) && (sid_model == MOS6581)) {
        accumulator &= (waveform_output << 12) | 0x7fffff;
    }

    if (unlikely(waveform > 0x8) && likely(!test)) {
      // Combined waveforms write to the shift register.
      // NB! Since cycles are skipped in delta_t clocking, writes will be
      // missed. Single cycle clocking must be used for 100% correct operation.
      write_shift_register();
    }
  }
  else {
    if (likely(floating_output_ttl)) {
      // Age floating D/A output.
      floating_output_ttl -= delta_t;
      if (unlikely(floating_output_ttl <= 0)) {
        floating_output_ttl = 0;
        waveform_output = 0;
      }
    }
  }
}


// ----------------------------------------------------------------------------
// Waveform output (12 bits).
// ----------------------------------------------------------------------------

// The digital waveform output is converted to an analog signal by a 12-bit
// DAC. Re-vectorized die photographs reveal that the DAC is an R-2R ladder
// built up as follows:
// 
//        12V     11  10   9   8   7   6   5   4   3   2   1   0    GND
// Strange  |      |   |   |   |   |   |   |   |   |   |   |   |     |  Missing
// part    2R     2R  2R  2R  2R  2R  2R  2R  2R  2R  2R  2R  2R    2R  term.
// (bias)   |      |   |   |   |   |   |   |   |   |   |   |   |     |
//          --R-   --R---R---R---R---R---R---R---R---R---R---R--   ---
//                 |          _____
//               __|__     __|__   |
//               -----     =====   |
//               |   |     |   |   |
//        12V ---     -----     ------- GND
//                      |
//                     wout
//
// Bit on:  5V
// Bit off: 0V (GND)
//
// As is the case with all MOS 6581 DACs, the termination to (virtual) ground
// at bit 0 is missing. The MOS 8580 has correct termination, and has also
// done away with the bias part on the left hand side of the figure above.
//

RESID_INLINE
short WaveformGenerator::output()
{
  // DAC imperfections are emulated by using waveform_output as an index
  // into a DAC lookup table. readOSC() uses waveform_output directly.
  return model_dac[sid_model][waveform_output];
}

#endif // RESID_INLINING || defined(RESID_WAVE_CC)

} // namespace reSID

#endif // not RESID_WAVE_H
