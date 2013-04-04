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

#define RESID_VOICE_CC

#include "voice.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
Voice::Voice()
{
  set_chip_model(MOS6581);
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void Voice::set_chip_model(chip_model model)
{
  wave.set_chip_model(model);
  envelope.set_chip_model(model);

  if (model == MOS6581) {
    // The waveform D/A converter introduces a DC offset in the signal
    // to the envelope multiplying D/A converter. The "zero" level of
    // the waveform D/A converter can be found as follows:
    //
    // Measure the "zero" voltage of voice 3 on the SID audio output
    // pin, routing only voice 3 to the mixer ($d417 = $0b, $d418 =
    // $0f, all other registers zeroed).
    //
    // Then set the sustain level for voice 3 to maximum and search for
    // the waveform output value yielding the same voltage as found
    // above. This is done by trying out different waveform output
    // values until the correct value is found, e.g. with the following
    // program:
    //
    //	lda #$08
    //	sta $d412
    //	lda #$0b
    //	sta $d417
    //	lda #$0f
    //	sta $d418
    //	lda #$f0
    //	sta $d414
    //	lda #$21
    //	sta $d412
    //	lda #$01
    //	sta $d40e
    //
    //	ldx #$00
    //	lda #$38	; Tweak this to find the "zero" level
    //l	cmp $d41b
    //	bne l
    //	stx $d40e	; Stop frequency counter - freeze waveform output
    //	brk
    //
    // The waveform output range is 0x000 to 0xfff, so the "zero"
    // level should ideally have been 0x800. In the measured chip, the
    // waveform output "zero" level was found to be 0x380 (i.e. $d41b
    // = 0x38) at an audio output voltage of 5.94V.
    //
    // With knowledge of the mixer op-amp characteristics, further estimates
    // of waveform voltages can be obtained by sampling the EXT IN pin.
    // From EXT IN samples, the corresponding waveform output can be found by
    // using the model for the mixer.
    //
    // Such measurements have been done on a chip marked MOS 6581R4AR
    // 0687 14, and the following results have been obtained:
    // * The full range of one voice is approximately 1.5V.
    // * The "zero" level rides at approximately 5.0V.
    //
    wave_zero = 0x380;
  }
  else {
    // No DC offsets in the MOS8580.
    wave_zero = 0x800;
  }
}

// ----------------------------------------------------------------------------
// Set sync source.
// ----------------------------------------------------------------------------
void Voice::set_sync_source(Voice* source)
{
  wave.set_sync_source(&source->wave);
}

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void Voice::writeCONTROL_REG(reg8 control)
{
  wave.writeCONTROL_REG(control);
  envelope.writeCONTROL_REG(control);
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void Voice::reset()
{
  wave.reset();
  envelope.reset();
}

} // namespace reSID
