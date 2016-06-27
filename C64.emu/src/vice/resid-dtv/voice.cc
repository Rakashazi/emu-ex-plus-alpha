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

#define __VOICE_CC__
#include "voice.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
Voice::Voice()
{
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
