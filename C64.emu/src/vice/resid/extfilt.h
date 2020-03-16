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

#ifndef RESID_EXTFILT_H
#define RESID_EXTFILT_H

#include "resid-config.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// The audio output stage in a Commodore 64 consists of two STC networks,
// a low-pass filter with 3-dB frequency 16kHz followed by a high-pass
// filter with 3-dB frequency 1.6Hz (the latter provided an audio equipment
// input impedance of 10kOhm).
// The STC networks are connected with a BJT supposedly meant to act as
// a unity gain buffer, which is not really how it works. A more elaborate
// model would include the BJT, however DC circuit analysis yields BJT
// base-emitter and emitter-base impedances sufficiently low to produce
// additional low-pass and high-pass 3dB-frequencies in the order of hundreds
// of kHz. This calls for a sampling frequency of several MHz, which is far
// too high for practical use.
// ----------------------------------------------------------------------------
class ExternalFilter
{
public:
  ExternalFilter();

  void enable_filter(bool enable);

  void clock(short Vi);
  void clock(cycle_count delta_t, short Vi);
  void reset();

  // Audio output (16 bits).
  short output();

protected:
  // Filter enabled.
  bool enabled;

  // State of filters (27 bits).
  int Vlp; // lowpass
  int Vhp; // highpass

  // Cutoff frequencies.
  int w0lp_1_s7;
  int w0hp_1_s17;

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(RESID_EXTFILT_CC)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void ExternalFilter::clock(short Vi)
{
  // This is handy for testing.
  if (unlikely(!enabled)) {
    // Vo  = Vlp - Vhp;
    Vlp = Vi << 11;
    Vhp = 0;
    return;
  }

  // Calculate filter outputs.
  // Vlp = Vlp + w0lp*(Vi - Vlp)*delta_t;
  // Vhp = Vhp + w0hp*(Vlp - Vhp)*delta_t;
  // Vo  = Vlp - Vhp;

  int dVlp = w0lp_1_s7*int((unsigned(Vi) << 11) - unsigned(Vlp)) >> 7;
  int dVhp = w0hp_1_s17*(Vlp - Vhp) >> 17;
  Vlp += dVlp;
  Vhp += dVhp;
}

// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void ExternalFilter::clock(cycle_count delta_t, short Vi)
{
  // This is handy for testing.
  if (unlikely(!enabled)) {
    // Vo  = Vlp - Vhp;
    Vlp = Vi << 11;
    Vhp = 0;
    return;
  }

  // Maximum delta cycles for the external filter to work satisfactorily
  // is approximately 8.
  cycle_count delta_t_flt = 8;

  while (delta_t) {
    if (unlikely(delta_t < delta_t_flt)) {
      delta_t_flt = delta_t;
    }

    // Calculate filter outputs.
    // Vlp = Vlp + w0lp*(Vi - Vlp)*delta_t;
    // Vhp = Vhp + w0hp*(Vlp - Vhp)*delta_t;
    // Vo  = Vlp - Vhp;

    int dVlp = (w0lp_1_s7*delta_t_flt >> 3)*((Vi << 11) - Vlp) >> 4;
    int dVhp = (w0hp_1_s17*delta_t_flt >> 3)*(Vlp - Vhp) >> 14;
    Vlp += dVlp;
    Vhp += dVhp;

    delta_t -= delta_t_flt;
  }
}


// ----------------------------------------------------------------------------
// Audio output (16 bits).
// ----------------------------------------------------------------------------
RESID_INLINE
short ExternalFilter::output()
{
  // Saturated arithmetics to guard against 16 bit sample overflow.
  const int half = 1 << 15;
  int Vo = (Vlp - Vhp) >> 11;
  if (Vo >= half) {
    Vo = half - 1;
  }
  else if (Vo < -half) {
    Vo = -half;
  }
  return Vo;
}

#endif // RESID_INLINING || defined(RESID_EXTFILT_CC)

} // namespace reSID

#endif // not RESID_EXTFILT_H
