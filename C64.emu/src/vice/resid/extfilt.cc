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

#define RESID_EXTFILT_CC

#include "extfilt.h"

namespace reSID
{

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
ExternalFilter::ExternalFilter()
{
  reset();
  enable_filter(true);

  // Low-pass:  R = 10 kOhm, C = 1000 pF; w0l = dt/(dt+RC) = 1e-6/(1e-6+1e4*1e-9) = 0.091
  // High-pass: R =  1 kOhm, C =   10 uF; w0h = dt/(dt+RC) = 1e-6/(1e-6+1e3*1e-5) = 0.0000999

  // Assume a 1MHz clock.
  // Cutoff frequency accuracy (4 bits) is traded off for filter signal
  // accuracy (27 bits). This is crucial since w0lp and w0hp are so far apart.
  w0lp_1_s7 = int(1e-6/(1e-6+1e4*1e-9)*(1 << 7) + 0.5);
  w0hp_1_s17 = int(1e-6/(1e-6+1e3*1e-5)*(1 << 17) + 0.5);
}


// ----------------------------------------------------------------------------
// Enable filter.
// ----------------------------------------------------------------------------
void ExternalFilter::enable_filter(bool enable)
{
  enabled = enable;
}


// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void ExternalFilter::reset()
{
  // State of filter.
  Vlp = 0;
  Vhp = 0;
}

} // namespace reSID
