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

#ifndef RESID_VOICE_H
#define RESID_VOICE_H

#include "resid-config.h"
#include "wave.h"
#include "envelope.h"

namespace reSID
{

class Voice
{
public:
  Voice();

  void set_chip_model(chip_model model);
  void set_sync_source(Voice*);
  void reset();

  void writeCONTROL_REG(reg8);

  // Amplitude modulated waveform output.
  // Range [-2048*255, 2047*255].
  int output();

  WaveformGenerator wave;
  EnvelopeGenerator envelope;

protected:
  // Waveform D/A zero level.
  short wave_zero;

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following function is defined inline because it is called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(RESID_VOICE_CC)

// ----------------------------------------------------------------------------
// Amplitude modulated waveform output (20 bits).
// Ideal range [-2048*255, 2047*255].
// ----------------------------------------------------------------------------

// The output for a voice is produced by a multiplying DAC, where the
// waveform output modulates the envelope output.
//
// As noted by Bob Yannes: "The 8-bit output of the Envelope Generator was then
// sent to the Multiplying D/A converter to modulate the amplitude of the
// selected Oscillator Waveform (to be technically accurate, actually the
// waveform was modulating the output of the Envelope Generator, but the result
// is the same)".
//
//          7   6   5   4   3   2   1   0   VGND
//          |   |   |   |   |   |   |   |     |   Missing
//         2R  2R  2R  2R  2R  2R  2R  2R    2R   termination
//          |   |   |   |   |   |   |   |     |
//          --R---R---R---R---R---R---R--   ---
//          |          _____
//        __|__     __|__   |
//        -----     =====   |
//        |   |     |   |   |
// 12V ---     -----     ------- GND
//               |
//              vout
//
// Bit on:  wout (see figure in wave.h)
// Bit off: 5V (VGND)
//
// As is the case with all MOS 6581 DACs, the termination to (virtual) ground
// at bit 0 is missing. The MOS 8580 has correct termination.
//

RESID_INLINE
int Voice::output()
{
  // Multiply oscillator output with envelope output.
  return (wave.output() - wave_zero)*envelope.output();
}

#endif // RESID_INLINING || defined(RESID_VOICE_CC)

} // namespace reSID

#endif // not RESID_VOICE_H
