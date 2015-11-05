/*! \file resid-dtv/sid.h */

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

#ifndef VICE__SID_H__
#define VICE__SID_H__

#include "residdtv-config.h"

#include "bittrain.h"
#include "voice.h"
#include "filter.h"
#include "extfilt.h"

namespace reSID
{

class SID
{
public:
  SID();
  ~SID();
  
  /* Some hacks to keep DTV looking like regular ReSID engine -- hopefully
   * removed at some point. */
  void set_chip_model(chip_model ignored);
  void set_voice_mask(reg4 mask);
  void enable_filter(bool enable);
  void input(short input);

  void enable_external_filter(bool enable);
  void adjust_filter_bias(double bias);
  bool set_sampling_parameters(double clock_freq, sampling_method method,
			       double sample_freq, double pass_freq = 20000,
			       double filter_scale = 0.97);
  void adjust_sampling_frequency(double sample_freq);

  void clock();
  void clock(cycle_count delta_t);
  int clock(cycle_count& delta_t, short* buf, int n, int interleave = 1);
  void reset();
  
  // Read/write registers.
  reg8 read(reg8 offset);
  void write(reg8 offset, reg8 value);

  // Read/write state.
  class State
  {
  public:
    State();

    char sid_register[0x20];

    reg8 bus_value;

    reg24 accumulator[3];
    reg24 shift_register[3];
    reg16 rate_counter[3];
    reg16 rate_counter_period[3];
    reg16 exponential_counter[3];
    reg16 exponential_counter_period[3];
    reg8 envelope_counter[3];
    EnvelopeGenerator::State envelope_state[3];
    bool hold_zero[3];
  };
    
  State read_state();
  void write_state(const State& state);

  // 16-bit output (AUDIO OUT).
  int output();

protected:
  static double I0(double x);
  RESID_INLINE int clock_interpolate(cycle_count& delta_t, short* buf, int n,
				     int interleave);
  RESID_INLINE int clock_resample_interpolate(cycle_count& delta_t, short* buf,
					      int n, int interleave);
  RESID_INLINE int clock_resample_fast(cycle_count& delta_t, short* buf,
				       int n, int interleave);

  Voice voice[3];
  Filter filter;
  ExternalFilter extfilt;

  reg8 bus_value;

  // Resampling constants.
  // The error in interpolated lookup is bounded by 1.234/L^2,
  // while the error in non-interpolated lookup is bounded by
  // 0.7854/L + 0.4113/L^2, see
  // http://www-ccrma.stanford.edu/~jos/resample/Choice_Table_Size.html
  // For a resolution of 16 bits this yields L >= 285 and L >= 51473,
  // respectively.
  enum { FIR_N = 125 };
  enum { FIR_RES_INTERPOLATE = 285 };
  enum { FIR_RES_FAST = 51473 };
  enum { FIR_SHIFT = 15 };
  enum { RINGSIZE = 4096 };

  // Fixpoint constants (16.16 bits).
  enum { FIXP_SHIFT = 16 };
  enum { FIXP_MASK = 0xffff };

  // for DTV volume bittrain emulation
  unsigned int master_volume;

  // Sampling variables.
  sampling_method sampling;
  cycle_count cycles_per_sample;
  cycle_count sample_offset;
  int sample_index;
  int sample_prev;
  int fir_N;
  int fir_RES;

  // Ring buffer with overflow for contiguous storage of RINGSIZE samples.
  short* sample;

  // FIR_RES filter tables (FIR_N*FIR_RES).
  short* fir;
};

} // namespace reSID

#endif // not __SID_H__
