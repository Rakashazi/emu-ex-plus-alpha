/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Z8530.c,v $
**
** $Revision: 1.1 $
**
** $Date: 2009-04-29 00:05:05 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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
#include "Z8530.h"
#include "SaveState.h"
#include "Board.h"
#include <stdlib.h>

struct Z8530
{
    void* ref;
    int mode;
    int reg;
    int status;
    UInt8 reg_val_a[16];
    UInt8 reg_val_b[16];
};

static int z8530_getareg(Z8530* z8530)
{
    return z8530->reg_val_a[z8530->reg];
}

static int z8530_getbreg(Z8530* z8530)
{
    if (z8530->reg == 2) {
//        z8530_acknowledge(z8530);
        return z8530->status;
    }

    return z8530->reg_val_b[z8530->reg];
}

static void z8530_putbreg(Z8530* z8530, int data)
{
    if (z8530->reg == 0) {
//        if (data & 0x10)
//            z8530_acknowledge(z8530);
    }
    z8530->reg_val_b[z8530->reg] = data;
}

static void z8530_putareg(Z8530* z8530, int data)
{
    if (z8530->reg == 0) {
//        if (data & 0x10)
//            z8530_acknowledge(device);
    }
    z8530->reg_val_a[z8530->reg] = data;
}

UInt8 z8530Read(Z8530* z8530, UInt16 port)
{
    UInt8 value = 0;

    port %= 4;

    switch (port) {
    case 0:
        if (z8530->mode == 1)
            z8530->mode = 0;
        else
            z8530->reg = 0;
        
        value = z8530_getbreg(z8530);
        break;
    case 1:
        if (z8530->mode == 1)
            z8530->mode = 0;
        else
            z8530->reg = 0;

        value = z8530_getareg(z8530);
        break;
    case 2:
        break;
    case 3:
        break;
    }

    return value;
}

void z8530Write(Z8530* z8530, UInt16 port, UInt8 value)
{
    port &= 3;

    switch (port) {

    case 0:
        if (z8530->mode == 0) {
            if((value & 0xf0) == 0) {
                z8530->mode = 1;
                z8530->reg = value & 0x0f;
            }
        }
        else {
            z8530->mode = 0;
            z8530_putbreg(z8530, value);
        }
        break;
        case 1:
            if (z8530->mode == 0) {
                if((value & 0xf0) == 0) {
                    z8530->mode = 1;
                    z8530->reg = value & 0x0f;
//                    z8530_putareg(z8530, value & 0xf0);
                }
            }
            else {
                z8530->mode = 0;
                z8530_putareg(z8530, value);
            }
            break;
        case 2:
            break;
        case 3:
            break;
        }
}

void z8530SaveState(Z8530* z8530)
{
    SaveState* state = saveStateOpenForWrite("z8530");

    saveStateClose(state);
}

void z8530LoadState(Z8530* z8530)
{
    SaveState* state = saveStateOpenForRead("z8530");

    saveStateClose(state);
}

void z8530Reset(Z8530* z8530)
{
}

void z8530Destroy(Z8530* z8530) 
{
    free(z8530);
}

Z8530* z8530Create(void* ref)
{
    Z8530* z8530 = calloc(1, sizeof(Z8530));
    z8530->ref = ref;

    return z8530;
}
