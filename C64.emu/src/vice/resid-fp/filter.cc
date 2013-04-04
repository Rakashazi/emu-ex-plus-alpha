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
//  Filter distortion code written by Antti S. Lankila 2007 - 2009.

#include "filter.h"
#include "sid.h"

#ifndef HAVE_LOGF_PROTOTYPE
extern float logf(float val);
#endif
 
#ifndef HAVE_EXPF_PROTOTYPE
extern float expf(float val);
#endif
 
#ifndef HAVE_LOGF
#define logf(val) (float)log((double)val)
#endif

#ifndef HAVE_EXPF
#define expf(val) (float)exp((double)val)
#endif

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
FilterFP::FilterFP()
    : enabled(true),
      model(MOS6581FP),
      clock_frequency(1000000),
      attenuation(0.5f), distortion_nonlinearity(3.3e6f),
    type3_baseresistance(129501), type3_offset(284015710.f), type3_steepness(1.0065f), type3_minimumfetresistance(18741),
      type4_k(20), type4_b(6.55f),
      nonlinearity(0.96f)
{
  reset();
}


// ----------------------------------------------------------------------------
// Enable filter.
// ----------------------------------------------------------------------------
void FilterFP::enable_filter(bool enable)
{
  enabled = enable;
  if (! enabled)
    filt = 0; // XXX should also restore this...
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void FilterFP::set_chip_model(chip_model model)
{
    this->model = model;
    set_Q();
    set_w0();
}

void FilterFP::set_nonlinearity(float nl)
{
    nonlinearity = nl;
    set_w0();
}

/* dist_CT eliminates 1/x at hot spot */
void FilterFP::set_clock_frequency(float clock) {
    clock_frequency = clock;
    distortion_CT = 1.f / (sidcaps_6581 * clock_frequency);
    set_w0();
}

void FilterFP::set_distortion_properties(float a, float nl)
{
    attenuation = a;
    distortion_nonlinearity = nl;
    set_w0();
}

void FilterFP::set_type4_properties(float k, float b)
{
    type4_k = k;
    type4_b = b;
    set_w0();
}

void FilterFP::set_type3_properties(float br, float o, float s, float mfr)
{
    type3_baseresistance = br;
    type3_offset = o;
    type3_steepness = -logf(s) / 512.f; /* s^x to e^(x*ln(s)), 1/e^x == e^-x. */
    type3_minimumfetresistance = mfr;
    set_w0();
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void FilterFP::reset()
{
  fc = 0;
  res = filt = voice3off = hp_bp_lp = 0; 
  vol = 0;
  volf = Vhp = Vbp = Vlp = 0;
  type3_fc_kink_exp = 0;
  type4_w0_cache = 0;
  set_w0();
  set_Q();
}

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void FilterFP::writeFC_LO(reg8 fc_lo)
{
  fc = (fc & 0x7f8) | (fc_lo & 0x007);
  set_w0();
}

void FilterFP::writeFC_HI(reg8 fc_hi)
{
  fc = ((fc_hi << 3) & 0x7f8) | (fc & 0x007);
  set_w0();
}

void FilterFP::writeRES_FILT(reg8 res_filt)
{
  res = (res_filt >> 4) & 0x0f;
  set_Q();
  filt = enabled ? res_filt & 0x0f : 0;
}

void FilterFP::writeMODE_VOL(reg8 mode_vol)
{
  voice3off = mode_vol & 0x80;

  hp_bp_lp = mode_vol >> 4;

  vol = mode_vol & 0x0f;
  volf = static_cast<float>(vol) / 15.f;
}

// Set filter cutoff frequency.
void FilterFP::set_w0()
{
  if (model == MOS6581FP) {
    float type3_fc_kink = SIDFP::kinked_dac(fc, nonlinearity, 11);
    type3_fc_kink_exp = type3_offset * expf(type3_fc_kink * type3_steepness * 512.f);
  }
  if (model == MOS8580FP) {
    type4_w0_cache = type4_w0();
  }
}

// Set filter resonance.
void FilterFP::set_Q()
{
    if (model == MOS6581FP) {
    /* These are handfitted approximations for algorithm that reduces hp by 6 dB.
     * For res=0, the desired behavior of filter is a 2 dB notch at the center frequency
     * followed by continuation of 6 dB lower when the energy is on hp output.
     * 
     * The filter hump must be about 8 dB when subtracting res=0 behavior from res=f.
     */
    _1_div_Q = 1.f / (0.5f + res / 18.f);
  } else {
    _1_div_Q = 1.f / (0.707f + res / 15.f);
  }
}
