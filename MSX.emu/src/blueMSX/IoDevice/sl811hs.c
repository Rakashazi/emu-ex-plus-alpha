/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/sl811hs.c,v $
**
** $Revision: 1.2 $
**
** $Date: 2008-03-30 18:38:41 $
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
#include "sl811hs.h"
#include "Board.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>

typedef struct SL811HS
{
    int dummy;
};


void sl811hsSaveState(SL811HS* sl)
{
}

void sl811hsLoadState(SL811HS* sl)
{
}

SL811HS* sl811hsCreate()
{
    SL811HS* sl = malloc(sizeof(SL811HS));

    return sl;
}

void sl811hsDestroy(SL811HS* sl)
{
    free(sl);
}

void sl811hsReset(SL811HS* sl)
{
}

UInt8 sl811hsRead(SL811HS* sl, UInt8 address)
{
    return 0xff;
}

void sl811hsWrite(SL811HS* sl, UInt8 address, UInt8 value)
{
}
