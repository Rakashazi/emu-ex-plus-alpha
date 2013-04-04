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

#include "residfp-config.h"

#include "wave.h"
#include "envelope.h"

class VoiceFP
{
public:
  VoiceFP();

  void set_chip_model(chip_model model);
  void reset();
  void mute(bool enable);

  void writeCONTROL_REG(WaveformGeneratorFP& source, reg8 value);

  // Amplitude modulated waveform output.
  // Range [-2048*255, 2047*255].
  RESID_INLINE float output(WaveformGeneratorFP& source);

protected:
  WaveformGeneratorFP wave;
  EnvelopeGeneratorFP envelope;

  // Multiplying D/A DC offset.
  float voice_DC;
friend class SIDFP;
};

// ----------------------------------------------------------------------------
// Amplitude modulated waveform output.
// Ideal range [-2048*255, 2047*255].
// ----------------------------------------------------------------------------

RESID_INLINE
float VoiceFP::output(WaveformGeneratorFP& source)
{
    return wave.output(source) * envelope.output() + voice_DC;
}

#endif // not VICE__VOICE_H__
