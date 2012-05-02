/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SN76489.h,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:45 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#ifndef SN76489_H
#define SN76489_H

#include "MsxTypes.h"
#include "AudioMixer.h"

/* Type definitions */
typedef struct SN76489 SN76489;

/* Constructor and destructor */
SN76489* sn76489Create(Mixer* mixer);
void sn76489Destroy(SN76489* sn76489);

/* Reset chip */
void sn76489Reset(SN76489* sn76489);

/* Register read/write methods */
void sn76489WriteData(SN76489* sn76489, UInt16 port, UInt8 data);

void sn76489LoadState(SN76489* sn76489);
void sn76489SaveState(SN76489* sn76489);

#endif

