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

#define __EXTFILT_CC__
#include "extfilt.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
ExternalFilter::ExternalFilter()
{
  reset();

  w0lp = 64475; /* 10 kHz, 3.3 kO and 4.7 nF RC filter from schematic */
  w0hp1 = 100;  /* 0.1 uF to transistor base, unknown resistance, estimated */
  w0hp2 = 167;  /* 26.5 Hz output */
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void ExternalFilter::reset()
{
  Vhp1 = Vhp2 = Vlp = Vo = 0;
}

/* API compatibility hacks */
void ExternalFilter::enable_filter(bool enable) { ; }

} // namespace reSID
