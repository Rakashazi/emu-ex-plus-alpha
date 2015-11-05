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

#ifndef VICE__VOICE_H__
#define VICE__VOICE_H__

#include "residdtv-config.h"

#include "wave.h"
#include "envelope.h"

namespace reSID
{

class Voice
{
public:
  Voice();

  void set_sync_source(Voice*);
  void reset();

  void writeCONTROL_REG(reg8);

  // Amplitude modulated waveform output.
  // Range [-2048*255, 2047*255].
  RESID_INLINE sound_sample output(unsigned int volume);

protected:
  WaveformGenerator wave;
  EnvelopeGenerator envelope;

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following function is defined inline because it is called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__VOICE_CC__)

// ----------------------------------------------------------------------------
// Amplitude modulated waveform output.
// Ideal range [-2048*255, 2047*255].
// ----------------------------------------------------------------------------
RESID_INLINE
sound_sample Voice::output(unsigned int volume)
{
  /* AND oscillator output with envelope output ANDed with volume output */
  unsigned int v = wave.output() & envelope.output() & volume_train_lut[volume];

  /* http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel */
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

#endif // RESID_INLINING || defined(__VOICE_CC__)

} // namespace reSID

#endif // not __VOICE_H__
