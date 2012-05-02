/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8255.c,v $
**
** $Revision: 1.9 $
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
#include "I8255.h"
#include "SaveState.h"
#include <stdlib.h>

struct I8255
{
    I8255Read peekA;
    I8255Read readA;
    I8255Write writeA;
    I8255Read peekB;
    I8255Read readB;
    I8255Write writeB;
    I8255Read peekCLo;
    I8255Read readCLo;
    I8255Write writeCLo;
    I8255Read peekCHi;
    I8255Read readCHi;
    I8255Write writeCHi;
    void* ref;

    UInt8 reg[4];
};

static UInt8 readDummy(void* ref)
{
    return 0xff;
}

static void writeDummy(void* ref, UInt8 value)
{
}

I8255* i8255Create(I8255Read peekA,   I8255Read readA,   I8255Write writeA, 
                   I8255Read peekB,   I8255Read readB,   I8255Write writeB,
                   I8255Read peekCLo, I8255Read readCLo, I8255Write writeCLo,
                   I8255Read peekCHi, I8255Read readCHi, I8255Write writeCHi,
                   void* ref)
{
    I8255* i8255 = calloc(1, sizeof(I8255));

    i8255->peekA    = peekA    ? peekA    : readDummy;
    i8255->readA    = readA    ? readA    : readDummy;
    i8255->writeA   = writeA   ? writeA   : writeDummy;
    i8255->peekB    = peekB    ? peekB    : readDummy;
    i8255->readB    = readB    ? readB    : readDummy;
    i8255->writeB   = writeB   ? writeB   : writeDummy;
    i8255->peekCLo  = peekCLo  ? peekCLo  : readDummy;
    i8255->readCLo  = readCLo  ? readCLo  : readDummy;
    i8255->writeCLo = writeCLo ? writeCLo : writeDummy;
    i8255->peekCHi  = peekCHi  ? peekCHi  : readDummy;
    i8255->readCHi  = readCHi  ? readCHi  : readDummy;
    i8255->writeCHi = writeCHi ? writeCHi : writeDummy;
    i8255->ref      = ref;

    return i8255;
}

void i8255Reset(I8255* i8255)
{
    i8255->reg[3] = 0x9b;

    i8255Write(i8255, 0, 0);
    i8255Write(i8255, 1, 0);
    i8255Write(i8255, 2, 0);
}

void i8255Destroy(I8255* i8255) 
{
    free(i8255);
}

void i8255LoadState(I8255* i8255)
{
    SaveState* state = saveStateOpenForRead("i8255");

    i8255->reg[0] = (UInt8)saveStateGet(state, "reg00", 0);
    i8255->reg[1] = (UInt8)saveStateGet(state, "reg01", 0);
    i8255->reg[2] = (UInt8)saveStateGet(state, "reg02", 0);
    i8255->reg[3] = (UInt8)saveStateGet(state, "reg03", 0);

    saveStateClose(state);
}

void i8255SaveState(I8255* i8255)
{
    SaveState* state = saveStateOpenForWrite("i8255");
    
    saveStateSet(state, "reg00", i8255->reg[0]);
    saveStateSet(state, "reg01", i8255->reg[1]);
    saveStateSet(state, "reg02", i8255->reg[2]);
    saveStateSet(state, "reg03", i8255->reg[3]);

    saveStateClose(state);
}

UInt8 i8255Peek(I8255* i8255, UInt16 port)
{
    UInt8 value;

    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x10) {
                return i8255->peekA(i8255->ref);
            }
            return i8255->reg[0];

        case 0x20: // MODE 1
            return 0xff;

        default: // MODE 2
            return 0xff;
        }
        break;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x02) {
                return i8255->peekA(i8255->ref);
            }
            return i8255->reg[1];

        default: // MODE 1
            return 0xff;
        }
        break;

    case 2:
        value = i8255->reg[2];

        if (i8255->reg[3] & 0x01) {
            value = (value & 0xf0) | (i8255->peekCLo(i8255->ref) & 0x0f);
        }
        if (i8255->reg[3] & 0x08) {
            value = (value & 0x0f) | (i8255->peekCHi(i8255->ref) << 4);
        }
        return value;

    case 3:
        return i8255->reg[3];
    }

    return 0xff;
}

UInt8 i8255Read(I8255* i8255, UInt16 port)
{
    UInt8 value;

    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x10) {
                return i8255->readA(i8255->ref);
            }
            return i8255->reg[0];

        case 0x20: // MODE 1
            return 0xff;

        default: // MODE 2
            return 0xff;
        }
        break;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x02) {
                return i8255->readB(i8255->ref);
            }
            return i8255->reg[1];

        default: // MODE 1
            return 0xff;
        }
        break;

    case 2:
        // FIXME: Check mode

        value = i8255->reg[2];

        if (i8255->reg[3] & 0x01) {
            value = (value & 0xf0) | (i8255->readCLo(i8255->ref) & 0x0f);
        }
        if (i8255->reg[3] & 0x08) {
            value = (value & 0x0f) | (i8255->readCHi(i8255->ref) << 4);
        }
        return value;

    case 3:
        return i8255->reg[3];
    }

    return 0xff;
}

void i8255Write(I8255* i8255, UInt16 port, UInt8 value)
{
    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            break;
        case 0x20: // MODE 1
            break;
        default: // MODE 2
            break;
        }

        i8255->reg[0] = value;
        
        if (!(i8255->reg[3] & 0x10)) {
            i8255->writeA(i8255->ref, value);
        }
        return;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            break;
        default: // MODE 1
            break;
        }

        i8255->reg[1] = value;
        
        if (!(i8255->reg[3] & 0x02)) {
            i8255->writeB(i8255->ref, value);
        }
        return;
        
    case 2:
        i8255->reg[2] = value;

        // FIXME: Check mode

        if (!(i8255->reg[3] & 0x01)) {
            i8255->writeCLo(i8255->ref, value & 0x0f);
        }
        if (!(i8255->reg[3] & 0x08)) {
            i8255->writeCHi(i8255->ref, value >> 4);
        }
        return;

    case 3:
        if (value & 0x80) {
            i8255->reg[3] = value;
            i8255Write(i8255, 0, i8255->reg[0]);
            i8255Write(i8255, 1, i8255->reg[1]);
            i8255Write(i8255, 2, i8255->reg[2]);
        }
        else {
            UInt8 mask = 1 << ((value >> 1) & 0x07);
            if (value & 0x01) {
                i8255Write(i8255, 2, i8255->reg[2] | mask);
            }
            else {
                i8255Write(i8255, 2, i8255->reg[2] & ~mask);
            }
        }
        return;
    }
}


#if 0

/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8255.c,v $
**
** $Revision: 1.9 $
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
#include "I8255.h"
#include "SaveState.h"
#include <stdlib.h>

struct I8255
{
    I8255Read peekA;
    I8255Read readA;
    I8255Write writeA;
    I8255Read peekB;
    I8255Read readB;
    I8255Write writeB;
    I8255Read peekCLo;
    I8255Read readCLo;
    I8255Write writeCLo;
    I8255Read peekCHi;
    I8255Read readCHi;
    I8255Write writeCHi;
    void* ref;

    UInt8 reg[4];

    int   useInputLatch;

    UInt8 latchA;
    UInt8 latchB;
    UInt8 latchC;
};

static UInt8 readLatchA(I8255* i8255)
{
    return i8255->latchA;
}

static UInt8 readLatchB(I8255* i8255)
{
    return i8255->latchB;
}

static UInt8 readLatchCLo(I8255* i8255)
{
    return i8255->latchC & 0xff;
}

static UInt8 readLatchCHi(I8255* i8255)
{
    return i8255->latchC >> 4;
}

static void writeDummy(void* ref, UInt8 value)
{
}

I8255* i8255Create(I8255Read peekA,   I8255Read readA,   I8255Write writeA, 
                   I8255Read peekB,   I8255Read readB,   I8255Write writeB,
                   I8255Read peekCLo, I8255Read readCLo, I8255Write writeCLo,
                   I8255Read peekCHi, I8255Read readCHi, I8255Write writeCHi,
                   void* ref, int useInputLatch)
{
    I8255* i8255 = calloc(1, sizeof(I8255));

    i8255->peekA    = peekA    ? peekA    : readLatchA;
    i8255->readA    = readA    ? readA    : readLatchA;
    i8255->writeA   = writeA   ? writeA   : writeDummy;
    i8255->peekB    = peekB    ? peekB    : readLatchB;
    i8255->readB    = readB    ? readB    : readLatchB;
    i8255->writeB   = writeB   ? writeB   : writeDummy;
    i8255->peekCLo  = peekCLo  ? peekCLo  : readLatchCLo;
    i8255->readCLo  = readCLo  ? readCLo  : readLatchCLo;
    i8255->writeCLo = writeCLo ? writeCLo : writeDummy;
    i8255->peekCHi  = peekCHi  ? peekCHi  : readLatchCHi;
    i8255->readCHi  = readCHi  ? readCHi  : readLatchCHi;
    i8255->writeCHi = writeCHi ? writeCHi : writeDummy;
    i8255->ref      = ref;

    i8255->useInputLatch = useInputLatch;

    return i8255;
}

void i8255Reset(I8255* i8255)
{
    i8255->reg[3] = 0x9b;

    i8255->latchA = 0xff;
    i8255->latchB = 0xff;
    i8255->latchC = 0xff;

    i8255Write(i8255, 0, 0);
    i8255Write(i8255, 1, 0);
    i8255Write(i8255, 2, 0);
}

void i8255Destroy(I8255* i8255) 
{
    free(i8255);
}

void i8255LoadState(I8255* i8255)
{
    SaveState* state = saveStateOpenForRead("i8255");

    i8255->reg[0] = (UInt8)saveStateGet(state, "reg00", 0);
    i8255->reg[1] = (UInt8)saveStateGet(state, "reg01", 0);
    i8255->reg[2] = (UInt8)saveStateGet(state, "reg02", 0);
    i8255->reg[3] = (UInt8)saveStateGet(state, "reg03", 0);

    saveStateClose(state);
}

void i8255SaveState(I8255* i8255)
{
    SaveState* state = saveStateOpenForWrite("i8255");
    
    saveStateSet(state, "reg00", i8255->reg[0]);
    saveStateSet(state, "reg01", i8255->reg[1]);
    saveStateSet(state, "reg02", i8255->reg[2]);
    saveStateSet(state, "reg03", i8255->reg[3]);

    saveStateClose(state);
}

#define INTR_B 0x01
#define IBF_B  0x02
#define OBF_B  0x02
#define STB_B  0x04
#define ACK_B  0x04
#define INTR_A 0x08
#define STB_A  0x10
#define IBF_A  0x20
#define ACK_A  0x40
#define OBF_A  0x80


void i8255WriteLatchA(I8255* i8255, UInt8 value)
{
    if (i8255->reg[3] & 0x01) {
        return;
    }

    switch (i8255->reg[3] & 0x60) {
    case 0x00: // MODE 0
        i8255->latchA = value;
        break;
    default: // MODE 1 and MODE 2
        if (!(i8255->reg[2] & IBF_A)) {
            if (i8255->latchA != value) {
                i8255->reg[2] |= IBF_A;
                i8255->reg[2] |= STB_A;
                i8255->latchA = value;
                if (i8255->reg[3] & 0x10) {
                    i8255->reg[2] |= INTR_A;
                    i8255->writeCLo(i8255->ref, i8255->reg[2] & 0x0f);
                    i8255->writeCHi(i8255->ref, i8255->reg[2] >> 4);
                }
                }
        }
        break;
    }
}

void i8255WriteLatchB(I8255* i8255, UInt8 value)
{
    if (i8255->reg[3] & 0x08) {
        return;
    }
    switch (i8255->reg[3] & 0x04) {
    case 0x00: // MODE 0
        i8255->latchB = value;
        break;
    default: // MODE 1
        if (!(i8255->reg[2] & IBF_B)) {
            if (i8255->latchB != value) {
                i8255->reg[2] |= IBF_B;
                i8255->reg[2] |= STB_B;
                i8255->latchB = value;
                if (i8255->reg[3] & 0x04) {
                    i8255->reg[2] |= INTR_B;
                    i8255->writeCLo(i8255->ref, i8255->reg[2] & 0x0f);
                }
            }
        }
        break;
    }
}

void i8255WriteLatchC(I8255* i8255, UInt8 value)
{
    i8255->latchC = value;
}

UInt8 i8255Peek(I8255* i8255, UInt16 port)
{
    UInt8 value;

    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x10) {
                if (i8255->useInputLatch) {
                    return i8255->latchA;
                }
                return i8255->peekA(i8255->ref);
            }
            return i8255->reg[0];

        case 0x20: // MODE 1
            if (i8255->useInputLatch) {
                return i8255->latchA;
            }
            return 0xff;

        default: // MODE 2
            if (i8255->useInputLatch) {
                return i8255->latchA;
            }
            return 0xff;
        }
        break;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x02) {
                if (i8255->useInputLatch) {
                    return i8255->latchB;
                }
                return i8255->peekA(i8255->ref);
            }
            return i8255->reg[1];

        default: // MODE 1
            if (i8255->useInputLatch) {
                return i8255->latchB;
            }
            return 0xff;
        }
        break;

    case 2:
        value = i8255->reg[2];

        if (i8255->reg[3] & 0x01) {
            if (i8255->useInputLatch) {
                value = (value & 0xf0) | (i8255->latchC & 0x0f);
            }
            else {
                value = (value & 0xf0) | (i8255->peekCLo(i8255->ref) & 0x0f);
            }
        }
        if (i8255->reg[3] & 0x08) {
            if (i8255->useInputLatch) {
                value = (value & 0xf0) | (i8255->latchC & 0xf0);
            }
            else {
                value = (value & 0x0f) | (i8255->peekCHi(i8255->ref) << 4);
            }
        }
        return value;

    case 3:
        return i8255->reg[3];
    }

    return 0xff;
}

UInt8 i8255Read(I8255* i8255, UInt16 port)
{
    UInt8 value;

    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x10) {
                if (i8255->useInputLatch) {
                    return i8255->latchA;
                }
                return i8255->readA(i8255->ref);
            }
            return i8255->reg[0];

        case 0x20: // MODE 1
            if (i8255->useInputLatch) {
                return i8255->latchA;
            }
            i8255->reg[2] &= ~IBF_A;
            return 0xff;

        default: // MODE 2
            if (i8255->useInputLatch) {
                return i8255->latchA;
            }
            i8255->reg[2] &= ~IBF_A;
            return 0xff;
        }
        break;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            if (i8255->reg[3] & 0x02) {
                if (i8255->useInputLatch) {
                    return i8255->latchB;
                }
                return i8255->readB(i8255->ref);
            }
            return i8255->reg[1];

        default: // MODE 1
            if (i8255->useInputLatch) {
                return i8255->latchB;
            }
            i8255->reg[2] &= ~IBF_B;
            return 0xff;
        }
        break;

    case 2:
        value = i8255->reg[2];

        if (i8255->reg[3] & 0x60) {
            value &= ~0x60;
            if (i8255->reg[3] & 0x10) {
                if (i8255->reg[3] & 0x01) {
                    value |= 20;
                }
                else {
                    value |= 40;
                }
            }
        }

        if (i8255->reg[3] & 0x04) {
            value &= ~0x04;
            if (i8255->reg[3] & 0x04) {
                value |= 0x04;
            }
        }

        if (i8255->reg[3] & 0x01) {
            if (i8255->useInputLatch) {
                value = (value & 0xf0) | (i8255->latchC & 0x0f);
            }
            else {
                value = (value & 0xf0) | (i8255->readCLo(i8255->ref) & 0x0f);
            }
        }
        if (i8255->reg[3] & 0x08) {
            if (i8255->useInputLatch) {
                value = (value & 0xf0) | (i8255->latchC & 0xf0);
            }
            else {
                value = (value & 0x0f) | (i8255->readCHi(i8255->ref) << 4);
            }
        }

        return value;

    case 3:
        return i8255->reg[3];
    }

    return 0xff;
}

void i8255Write(I8255* i8255, UInt16 port, UInt8 value)
{
    port &= 0x03;

    switch (port) {
    case 0:
        switch (i8255->reg[3] & 0x60) {
        case 0x00: // MODE 0
            break;
        case 0x20: // MODE 1
            break;
        default: // MODE 2
            break;
        }

        i8255->reg[0] = value;
        
        if (!(i8255->reg[3] & 0x10)) {
            i8255->writeA(i8255->ref, value);
        }
        return;

    case 1:
        switch (i8255->reg[3] & 0x04) {
        case 0x00: // MODE 0
            break;
        default: // MODE 1
            break;
        }

        i8255->reg[1] = value;
        
        if (!(i8255->reg[3] & 0x02)) {
            i8255->writeB(i8255->ref, value);
        }
        return;
        
    case 2:
        {
            UInt8 mask = 0xff;

            if (i8255->reg[3] & 0x04) {
                mask &= 0xf8;
            }
            if (i8255->reg[3] & 0x60) {
                mask &= 0x07;
            }

            if (!(i8255->reg[3] & 0x01) && (mask & 0x0f)) {
                i8255->writeCLo(i8255->ref, value & mask & 0x0f);
            }
            if (!(i8255->reg[3] & 0x08) && (mask & 0xf0)) {
                i8255->writeCHi(i8255->ref, value >> 4);
            }

            i8255->reg[2] = (i8255->reg[2] & ~mask) | (value & mask);
        }
        return;

    case 3:
        if (value & 0x80) {
            if (value != i8255->reg[3]) {
                // Mode change

                if (value & 0x60) {
                    i8255->reg[2] = (i8255->reg[2] & 0xf8) | 0x00;
                }

                if (value & 0x04) {
                    i8255->reg[2] = (i8255->reg[2] & 0x07) | 0x00;
                }
            }

            i8255->reg[3] = value;
            i8255Write(i8255, 0, i8255->reg[0]);
            i8255Write(i8255, 1, i8255->reg[1]);
            i8255Write(i8255, 2, i8255->reg[2]);
        }
        else {
            UInt8 mask = 1 << ((value >> 1) & 0x07);
            if (value & 0x01) {
                i8255Write(i8255, 2, i8255->reg[2] | mask);
            }
            else {
                i8255Write(i8255, 2, i8255->reg[2] & ~mask);
            }
        }
        return;
    }
}
#endif