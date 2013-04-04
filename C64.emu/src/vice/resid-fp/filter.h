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

#ifndef VICE__FILTER_H__
#define VICE__FILTER_H__

#include <math.h>

#include "residfp-config.h"

// ----------------------------------------------------------------------------
// The SID filter is modeled with a two-integrator-loop biquadratic filter,
// which has been confirmed by Bob Yannes to be the actual circuit used in
// the SID chip.
//
// Measurements show that excellent emulation of the SID filter is achieved,
// except when high resonance is combined with high sustain levels.
// In this case the SID op-amps are performing less than ideally and are
// causing some peculiar behavior of the SID filter. This however seems to
// have more effect on the overall amplitude than on the color of the sound.
//
// The theory for the filter circuit can be found in "Microelectric Circuits"
// by Adel S. Sedra and Kenneth C. Smith.
// The circuit is modeled based on the explanation found there except that
// an additional inverter is used in the feedback from the bandpass output,
// allowing the summer op-amp to operate in single-ended mode. This yields
// inverted filter outputs with levels independent of Q, which corresponds with
// the results obtained from a real SID.
//
// We have been able to model the summer and the two integrators of the circuit
// to form components of an IIR filter.
// Vhp is the output of the summer, Vbp is the output of the first integrator,
// and Vlp is the output of the second integrator in the filter circuit.
//
// According to Bob Yannes, the active stages of the SID filter are not really
// op-amps. Rather, simple NMOS inverters are used. By biasing an inverter
// into its region of quasi-linear operation using a feedback resistor from
// input to output, a MOS inverter can be made to act like an op-amp for
// small signals centered around the switching threshold.
//
// Qualified guesses at SID filter schematics are depicted below.
//
// SID filter
// ----------
// 
//     -----------------------------------------------
//    |                                               |
//    |            ---Rq--                            |
//    |           |       |                           |
//    |  ------------<A]-----R1---------              |
//    | |                               |             |
//    | |                        ---C---|      ---C---|
//    | |                       |       |     |       |
//    |  --R1--    ---R1--      |---Rs--|     |---Rs--| 
//    |        |  |       |     |       |     |       |
//     ----R1--|-----[A>--|--R-----[A>--|--R-----[A>--|
//             |          |             |             |
// vi -----R1--           |             |             |
// 
//                       vhp           vbp           vlp
// 
// 
// vi  - input voltage
// vhp - highpass output
// vbp - bandpass output
// vlp - lowpass output
// [A> - op-amp
// R1  - summer resistor
// Rq  - resistor array controlling resonance (4 resistors)
// R   - NMOS FET voltage controlled resistor controlling cutoff frequency
// Rs  - shunt resitor
// C   - capacitor
// 
// 
// 
// SID integrator
// --------------
// 
//                                   V+
// 
//                                   |
//                                   |
//                              -----|
//                             |     |
//                             | ||--
//                              -||
//                   ---C---     ||->
//                  |       |        |
//                  |---Rs-----------|---- vo
//                  |                |
//                  |            ||--
// vi ----     -----|------------||
//        |   ^     |            ||->
//        |___|     |                |
//        -----     |                |
//          |       |                |
//          |---R2--                 |
//          |
//          R1                       V-
//          |
//          |
// 
//          Vw
//
// ----------------------------------------------------------------------------
class FilterFP
{
public:
  FilterFP();

  void enable_filter(bool enable);
  void set_chip_model(chip_model model);
  void set_distortion_properties(float, float);
  void set_type3_properties(float, float, float, float);
  void set_type4_properties(float, float);
  void set_clock_frequency(float);
  void set_nonlinearity(float);

  RESID_INLINE
  float clock(float voice1, float voice2, float voice3,
	      float ext_in);
  void reset();

  // Write registers.
  void writeFC_LO(reg8);
  void writeFC_HI(reg8);
  void writeRES_FILT(reg8);
  void writeMODE_VOL(reg8);

private:
  void set_Q();
  void set_w0();
  float type3_w0(const float dist);
  float type4_w0();
  void calculate_helpers();
  void nuke_denormals();
  float waveshaper1(float value);
  float waveshaper2(float value);

  // Filter enabled.
  bool enabled;

  // 6581/8580 filter model (XXX: we should specialize in separate classes)
  chip_model model;

  // Filter cutoff frequency.
  reg12 fc;

  // Filter resonance.
  reg8 res;

  // Selects which inputs to route through filter.
  reg8 filt;

  // Switch voice 3 off.
  reg8 voice3off;

  // Highpass, bandpass, and lowpass filter modes.
  reg8 hp_bp_lp;

  // Output master volume.
  reg4 vol;
  float volf; /* avoid integer-to-float conversion at output */

  // clock
  float clock_frequency;

  /* Distortion params for Type3 */
  float attenuation, distortion_nonlinearity;

  /* Type3 params. */
  float type3_baseresistance, type3_offset, type3_steepness, type3_minimumfetresistance;

  /* Type4 params */
  float type4_k, type4_b;

  // State of filter.
  float Vhp, Vbp, Vlp;

  /* Resonance/Distortion/Type3/Type4 helpers. */
  float type4_w0_cache, _1_div_Q, type3_fc_kink_exp, distortion_CT;

  float nonlinearity;
friend class SIDFP;
};

// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

const float sidcaps_6581 = 470e-12f;

RESID_INLINE
static float fastexp(float val) {
    typedef union {
        int i;
        float f;
    } conv;

    conv tmp;

    /* single precision fp has 1 + 8 + 23 bits, exponent bias is 127.
     * It therefore follows that we need to shift left by 23 bits, and to
     * calculate exp(x) instead of pow(2, x) we divide the power by ln(2). */
    const float a = static_cast<float>((1 << 23) / M_LN2);
    /* The other factor corrects for the exponent bias so that 2^0 = 1. */
    const float b = static_cast<float>((1 << 23) * 127);
    /* According to "A Fast, Compact Approximation of the Exponential Function"
     * by Nicol N. Schraudolph, 60801.48 yields the minimum RMS error for the
     * piecewise-linear approximation when using doubles (20 bits residual).
     * We have 23 bits, so we scale this value by 8. */
    const float c = 60801.48f * 8.f;

    /* Parenthesis are important: C standard disallows folding subtraction.
     * Unfortunately GCC appears to generate a write to memory rather than
     * handle this conversion entirely in registers. */
    tmp.i = static_cast<int>(a * val + (b - c + 0.5f));
    return tmp.f;
}

RESID_INLINE
float FilterFP::type3_w0(const float dist)
{
    float fetresistance = type3_fc_kink_exp;
    if (dist > 0) {
        fetresistance *= fastexp(dist * type3_steepness);
    }
    const float dynamic_resistance = type3_minimumfetresistance + fetresistance;

    /* 2 parallel resistors */
    const float _1_div_resistance = (type3_baseresistance + dynamic_resistance) / (type3_baseresistance * dynamic_resistance);
    /* 1.f / (clock * caps * resistance) */
    return distortion_CT * _1_div_resistance;
}

RESID_INLINE
float FilterFP::type4_w0()
{
    const float freq = type4_k * fc + type4_b;
    return 2.f * static_cast<float>(M_PI) * freq / clock_frequency;
}

RESID_INLINE
float FilterFP::waveshaper1(float value)
{
    if (value > distortion_nonlinearity) {
        value -= (value - distortion_nonlinearity) * 0.5f;
    }
    return value;
}

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
float FilterFP::clock(float voice1,
		   float voice2,
		   float voice3,
		   float ext_in)
{
    float Vi = 0.f, Vf = 0.f;

    // Route voices into or around filter.
    ((filt & 1) ? Vi : Vf) += voice1;
    ((filt & 2) ? Vi : Vf) += voice2;
    // NB! Voice 3 is not silenced by voice3off if it is routed through
    // the filter.
    if (filt & 4) {
	Vi += voice3;
    } else if (! voice3off) {
	Vf += voice3;
    }
    ((filt & 8) ? Vi : Vf) += ext_in;

    if (hp_bp_lp & 1) {
	Vf += Vlp;
    }
    if (hp_bp_lp & 2) {
	Vf += Vbp;
    }
    if (hp_bp_lp & 4) {
	Vf += Vhp;
    }
    
    if (model == MOS6581FP) {
        Vlp -= Vbp * type3_w0(Vbp);
        Vbp -= Vhp * type3_w0(Vhp);
        Vhp = (Vbp * _1_div_Q - Vlp - Vi) * attenuation;

        Vf *= volf;
        Vf = waveshaper1(Vf);
    } else {
        /* On the 8580, BP appears mixed in phase with the rest. */
        Vlp += Vbp * type4_w0_cache;
        Vbp += Vhp * type4_w0_cache;
        Vhp = -Vbp * _1_div_Q - Vlp - Vi;
        Vf *= volf;
    }
    
    return Vf;
}

RESID_INLINE
void FilterFP::nuke_denormals()
{
    /* We only need this for systems that don't do -msse and -mfpmath=sse */
    if (Vbp > -1e-12f && Vbp < 1e-12f)
        Vbp = 0;
    if (Vlp > -1e-12f && Vlp < 1e-12f)
        Vlp = 0;
}

#endif // not VICE__FILTER_H__
