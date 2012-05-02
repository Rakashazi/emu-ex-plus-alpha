/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Z80/R800SaveState.h,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:48 $
**
** Author: Daniel Vik
**
** Description: Save/load state of an R800 object
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
#ifndef R800_SAVE_STATE_H
#define R800_SAVE_STATE_H
 
#include "MsxTypes.h"
#include "R800.h"

void r800LoadState(R800* r800);
void r800SaveState(R800* r800);

#endif
