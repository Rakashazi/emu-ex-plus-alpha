/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Microwire93Cx6.h,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:40 $
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
#ifndef MICROWIRE_93Cx6_H
#define MICROWIRE_93Cx6_H

#include "MsxTypes.h"

typedef struct Microwire93Cx6 Microwire93Cx6;

Microwire93Cx6* microwire93Cx6Create(int size, int width, void* romData, int romSize, const char* sramFilename);
void microwire93Cx6Destroy(Microwire93Cx6* rtl);

void microwire93Cx6Reset(Microwire93Cx6* rtl);

void microwire93Cx6SetCs(Microwire93Cx6* rtl, int value);
void microwire93Cx6SetClk(Microwire93Cx6* rtl, int value);
void microwire93Cx6SetDi(Microwire93Cx6* rtl, int value);

int microwire93Cx6GetDo(Microwire93Cx6* rtl);

void microwire93Cx6SaveState(Microwire93Cx6* rtl);
void microwire93Cx6LoadState(Microwire93Cx6* rtl);

#endif

