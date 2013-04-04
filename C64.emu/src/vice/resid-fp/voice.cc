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

#include "voice.h"

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
VoiceFP::VoiceFP()
{
  set_chip_model(MOS6581FP);
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void VoiceFP::set_chip_model(chip_model model)
{
  wave.set_chip_model(model);

  if (model == MOS6581FP) {
    /* there is some level from each voice even if the env is down and osc
     * is stopped. You can hear this by routing a voice into filter (filter
     * should be kept disabled for this) as the master level changes. This
     * tunable affects the volume of digis. */
    voice_DC = static_cast<float>(0x800 * 0xff);
    /* In 8580 the waveforms seem well centered, but on the 6581 there is some
     * offset change as envelope grows, indicating that the waveforms are not
     * perfectly centered. The likely cause for this is the follows:
     *
     * The waveform DAC generates a voltage between 5 and 12 V corresponding
     * to oscillator state 0 .. 4095.
     *
     * The envelope DAC generates a voltage between waveform gen output and
     * the 5V level.
     *
     * The outputs are amplified against the 12V voltage and sent to the
     * mixer.
     *
     * The SID virtual ground is around 6.5 V. */
  }
  else {
    voice_DC = 0.f;
  }
}

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void VoiceFP::writeCONTROL_REG(WaveformGeneratorFP& source, reg8 control)
{
  wave.writeCONTROL_REG(source, control);
  envelope.writeCONTROL_REG(control);
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void VoiceFP::reset()
{
  wave.reset();
  envelope.reset();
}


// ----------------------------------------------------------------------------
// Voice mute.
// ----------------------------------------------------------------------------
void VoiceFP::mute(bool enable)
{
  envelope.mute(enable);
}
