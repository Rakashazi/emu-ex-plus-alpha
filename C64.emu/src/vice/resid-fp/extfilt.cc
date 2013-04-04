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

#include "extfilt.h"
 
const float pass_frequency = 15915.6f;

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
ExternalFilterFP::ExternalFilterFP()
{
  reset();
  set_clock_frequency(1e6f);
}

// ----------------------------------------------------------------------------
// Setup of the external filter sampling parameters.
// ----------------------------------------------------------------------------
void ExternalFilterFP::set_clock_frequency(float clock_frequency)
{
  // Low-pass:  R = 10kOhm, C = 1000pF; w0l = 1/RC = 1/(1e4*1e-9) = 100000
  // High-pass: R =  1kOhm, C =   10uF; w0h = 1/RC = 1/(1e3*1e-5) =    100
  w0hp = 100.f / clock_frequency;
  w0lp = pass_frequency * 2.f * static_cast<float>(M_PI) / clock_frequency;
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void ExternalFilterFP::reset()
{
  // State of filter.
  Vlp = 0;
  Vhp = 0;
}
