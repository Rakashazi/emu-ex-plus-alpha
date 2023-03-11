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

#define RESID_DAC_CC

#ifdef _M_ARM
#undef _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1
#endif

#include "dac.h"
#include <math.h>

#ifndef INFINITY
union MSVC_EVIL_FLOAT_HACK
{
   unsigned char Bytes[4];
   float Value;
};
static union MSVC_EVIL_FLOAT_HACK INFINITY_HACK = {{0x00, 0x00, 0x80, 0x7F}};
#define INFINITY (INFINITY_HACK.Value)
#endif

namespace reSID
{

// ----------------------------------------------------------------------------
// Calculation of lookup tables for SID DACs.
// ----------------------------------------------------------------------------

// The SID DACs are built up as follows:
//
//          n  n-1      2   1   0    VGND
//          |   |       |   |   |      |   Termination
//         2R  2R      2R  2R  2R     2R   only for
//          |   |       |   |   |      |   MOS 8580
//      Vo  --R---R--...--R---R--    ---
//
//
// All MOS 6581 DACs are missing a termination resistor at bit 0. This causes
// pronounced errors for the lower 4 - 5 bits (e.g. the output for bit 0 is
// actually equal to the output for bit 1), resulting in DAC discontinuities
// for the lower bits.
// In addition to this, the 6581 DACs exhibit further severe discontinuities
// for higher bits, which may be explained by a less than perfect match between
// the R and 2R resistors, or by output impedance in the NMOS transistors
// providing the bit voltages. A good approximation of the actual DAC output is
// achieved for 2R/R ~ 2.20.
//
// The MOS 8580 DACs, on the other hand, do not exhibit any discontinuities.
// These DACs include the correct termination resistor, and also seem to have
// very accurately matched R and 2R resistors (2R/R = 2.00).

void build_dac_table(unsigned short* dac, int bits, double _2R_div_R, bool term)
{
  // FIXME: No variable length arrays in ISO C++, hardcoding to max 12 bits.
  // double vbit[bits];
  double vbit[12];

  // Calculate voltage contribution by each individual bit in the R-2R ladder.
  for (int set_bit = 0; set_bit < bits; set_bit++) {
    int bit;

    double Vn = 1.0;          // Normalized bit voltage.
    double R = 1.0;           // Normalized R
    double _2R = _2R_div_R*R; // 2R
    double Rn = term ?        // Rn = 2R for correct termination,
      _2R : INFINITY;         // INFINITY for missing termination.

    // Calculate DAC "tail" resistance by repeated parallel substitution.
    for (bit = 0; bit < set_bit; bit++) {
      if (Rn == INFINITY) {
        Rn = R + _2R;
      }
      else {
        Rn = R + _2R*Rn/(_2R + Rn); // R + 2R || Rn
      }
    }

    // Source transformation for bit voltage.
    if (Rn == INFINITY) {
      Rn = _2R;
    }
    else {
      Rn = _2R*Rn/(_2R + Rn);  // 2R || Rn
      Vn = Vn*Rn/_2R;
    }

    // Calculate DAC output voltage by repeated source transformation from
    // the "tail".
    for (++bit; bit < bits; bit++) {
      Rn += R;
      double I = Vn/Rn;
      Rn = _2R*Rn/(_2R + Rn);  // 2R || Rn
      Vn = Rn*I;
    }

    vbit[set_bit] = Vn;
  }

  // Calculate the voltage for any combination of bits by superpositioning.
  for (int i = 0; i < (1 << bits); i++) {
    int x = i;
    double Vo = 0;
    for (int j = 0; j < bits; j++) {
      Vo += (x & 0x1)*vbit[j];
      x >>= 1;
    }

    // Scale maximum output to 2^bits - 1.
    dac[i] = (unsigned short)(((1 << bits) - 1)*Vo + 0.5);
  }
}

} // namespace reSID
