/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SCC.c,v $
**
** $Revision: 1.27 $
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
#include "SCC.h"
#include "Board.h"
#include "SaveState.h"
#include "DebugDeviceManager.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>


//#define BASE_PHASE_STEP 0x28959becUL  /* = (1 << 28) * 3579545 / 32 / 44100 */
#define BASE_PHASE_STEP 0xA2566FBUL  /* = (1 << 28) * 3579545 / 32 / (44100 * 4) */

#define ROTATE_OFF 32
#define ROTATE_ON  28

#define OFFSETOF(s, a) ((int)(&((s*)0)->a))

static Int32* sccSync(SCC* scc, UInt32 count);


struct SCC
{
    Mixer* mixer;
    Int32  handle;
    Int32  debugHandle;
    
    SccMode mode;
    UInt8 deformReg;
    Int8 curWave[5];
    Int8 wave[5][32];
    UInt32 period[5];
    UInt32 phase[5];
    UInt32 phaseStep[5];
    int  volume[5];
    int  nextVolume[5];
    UInt8 enable;
    UInt16 bus;
    int rotate[5];
    int readOnly[5];
    Int32 oldSample[5];
    Int32 deformSample[5];
    Int32  daVolume[5];

    Int32 in[95];
    Int32 inHp[3];
    Int32 outHp[3];

    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};

void sccLoadState(SCC* scc)
{
    SaveState* state = saveStateOpenForRead("scc");
    char tag[32];
    int i;

    scc->mode      =         saveStateGet(state, "mode", SCC_COMPATIBLE);
    scc->deformReg = (UInt8) saveStateGet(state, "deformReg", 0);
    
    for (i = 0; i < 5; i++) {
        int j;
        for (j = 0; j < 32; j++) {
            sprintf(tag, "wave%d%d", i, j);
            scc->wave[i][j] = (UInt8)saveStateGet(state, tag, 0);
        }

        sprintf(tag, "period%d", i);
        scc->period[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "phase%d", i);
        scc->phase[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "step%d", i);
        scc->phaseStep[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "volume%d", i);
        scc->volume[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "nextVolume%d", i);
        scc->nextVolume[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "rotate%d", i);
        scc->rotate[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "readOnly%d", i);
        scc->readOnly[i] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "daVolume%d", i);
        scc->daVolume[i] = saveStateGet(state, tag, 0);

        sprintf(tag, "oldSample%d", i);
        scc->oldSample[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
}

void sccSaveState(SCC* scc)
{
    SaveState* state = saveStateOpenForWrite("scc");
    char tag[32];
    int i;

    saveStateSet(state, "mode", scc->mode);
    saveStateSet(state, "deformReg", scc->deformReg);
    
    for (i = 0; i < 5; i++) {
        int j;
        for (j = 0; j < 32; j++) {
            sprintf(tag, "wave%d%d", i, j);
            saveStateSet(state, tag, scc->wave[i][j]);
        }

        sprintf(tag, "period%d", i);
        saveStateSet(state, tag, scc->period[i]);
        
        sprintf(tag, "phase%d", i);
        saveStateSet(state, tag, scc->phase[i]);
        
        sprintf(tag, "step%d", i);
        saveStateSet(state, tag, scc->phaseStep[i]);
        
        sprintf(tag, "volume%d", i);
        saveStateSet(state, tag, scc->volume[i]);
        
        sprintf(tag, "nextVolume%d", i);
        saveStateSet(state, tag, scc->nextVolume[i]);
        
        sprintf(tag, "rotate%d", i);
        saveStateSet(state, tag, scc->rotate[i]);
        
        sprintf(tag, "readOnly%d", i);
        saveStateSet(state, tag, scc->readOnly[i]);
        
        sprintf(tag, "daVolume%d", i);
        saveStateSet(state, tag, scc->daVolume[i]);

        sprintf(tag, "oldSample%d", i);
        saveStateSet(state, tag, scc->oldSample[i]);
    }

    saveStateClose(state);
}

static UInt8 sccGetWave(SCC* scc, UInt8 channel, UInt8 address)
{
    if (scc->rotate[channel] == ROTATE_OFF) {
        UInt8 value = scc->wave[channel][address & 0x1f];
        scc->bus = value;
        return value;
    } 
    else {
        UInt8 periodCh = channel;
        UInt8 value;
        int shift;

        mixerSync(scc->mixer);

         if ((scc->deformReg & 0xc0) == 0x80) {
             if (channel == 4) {
                 periodCh = 3;
             }
         }
         else if (channel == 3 && scc->mode != SCC_PLUS) {
             periodCh = 4;
         }

         shift = scc->oldSample[periodCh] - scc->deformSample[periodCh];

        value = scc->wave[channel][(address + shift) & 0x1f];
        scc->bus = value;
        return value;
    }
}

static UInt8 sccGetFreqAndVol(SCC* scc, UInt8 address)
{
    address &= 0x0f;

    if (address < 0x0a) {
        // get period
        UInt8 channel = address / 2;
        if (address & 1) {
            return (UInt8)(scc->period[channel] >> 8);
        } else {
            return (UInt8)(scc->period[channel] & 0xff);
        }
    } else if (address < 0x0f) {
        // get volume
        return scc->nextVolume[address - 0xa];
    } else {
        // get enable-bits
        return scc->enable;
    }
}

static void sccUpdateWave(SCC* scc, UInt8 channel, UInt8 address, UInt8 value)
{
    if (!scc->readOnly[channel]) {
        UInt8 pos = address & 0x1f;

        scc->bus = value;

        scc->wave[channel][pos] = value;
        if ((scc->mode != SCC_PLUS) && (channel == 3)) {
            scc->wave[4][pos] = scc->wave[3][pos];
        }
    }
}

static void sccUpdateFreqAndVol(SCC* scc, UInt8 address, UInt8 value)
{
    address &= 0x0f;
    if (address < 0x0a) {
        UInt8 channel = address / 2;
        UInt32 period;

        mixerSync(scc->mixer);

        if (address & 1) {
            scc->period[channel] = ((value & 0xf) << 8) | (scc->period[channel] & 0xff);
        } 
        else {
            scc->period[channel] = (scc->period[channel] & 0xf00) | (value & 0xff);
        }
        if (scc->deformReg & 0x20) {
            scc->phase[channel] = 0;
        }
        period = scc->period[channel];

        if (scc->deformReg & 2) {
            period &= 0xff;
        }
        else if (scc->deformReg & 1) {
            period >>= 8;
        }
        
        scc->phaseStep[channel] = period > 0 ? BASE_PHASE_STEP / (1 + period) : 0;
        
        scc->volume[channel] = scc->nextVolume[channel];
        scc->phase[channel] &= 0x1f << 23;
        scc->oldSample[channel] = 0xff;
    } 
    else if (address < 0x0f) {
        scc->nextVolume[address - 0x0a] = value & 0x0f;
    } 
    else {
        scc->enable = value;
    }
}

static void sccUpdateDeformation(SCC* scc, UInt8 value)
{
    int channel;

    if (value == scc->deformReg) {
        return;
    }

    mixerSync(scc->mixer);

    scc->deformReg = value;
    
    for (channel = 0; channel < 5; channel++) {
        scc->deformSample[channel] = scc->oldSample[channel];
    }

    if (scc->mode != SCC_REAL) {
        value &= ~0x80;
    }

    switch (value & 0xc0) {
        case 0x00:
            for (channel = 0; channel < 5; channel++) {
                scc->rotate[channel]   = ROTATE_OFF;
                scc->readOnly[channel] = 0;
            }
            break;
        case 0x40:
            for (channel = 0; channel < 5; channel++) {
                scc->rotate[channel]   = ROTATE_ON;
                scc->readOnly[channel] = 1;
            }
            break;
        case 0x80:
            for (channel = 0; channel < 3; channel++) {
                scc->rotate[channel]   = ROTATE_OFF;
                scc->readOnly[channel] = 0;
            }
            for (channel = 3; channel < 5; channel++) {
                scc->rotate[channel]   = ROTATE_ON;
                scc->readOnly[channel] = 1;
            }
            break;
        case 0xC0:
            for (channel = 0; channel < 3; channel++) {
                scc->rotate[channel]   = ROTATE_ON;
                scc->readOnly[channel] = 1;
            }
            for (channel = 3; channel < 5; channel++) {
                scc->rotate[channel]   = ROTATE_OFF;
                scc->readOnly[channel] = 1;
            }
            break;
    }
}

void sccReset(SCC* scc) {
    int channel;

    if (scc->mode != SCC_REAL) {
        sccSetMode(scc, SCC_COMPATIBLE);
    }

    for (channel = 0; channel < 5; channel++) {
        scc->curWave[channel]    = 0;
        scc->phase[channel]      = 0;
        scc->phaseStep[channel]  = 0;
        scc->volume[channel]     = 0;
        scc->nextVolume[channel] = 0;
        scc->rotate[channel]     = ROTATE_OFF;
        scc->readOnly[channel]   = 0;
        scc->daVolume[channel]   = 0;
        scc->oldSample[channel]  = 0xff;
    }

    scc->deformReg = 0;
    scc->enable      = 0xFF;
    scc->bus         = 0xFFFF;
}

void sccSetMode(SCC* scc, SccMode newMode)
{
    scc->mode = newMode;
}

static void getDebugInfo(SCC* scc, DbgDevice* dbgDevice)
{
    static UInt8 ram[0x100];
    int i;

    for (i = 0; i < 0x100; i++) {
        sccPeek(scc, i);
    }

    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemScc(), 1, 0, 0x100, ram);
}

SCC* sccCreate(Mixer* mixer)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    SCC* scc = (SCC*)calloc(1, sizeof(SCC));

    scc->mixer = mixer;

//    scc->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevScc(), &dbgCallbacks, scc);

    scc->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_SCC, 0, sccSync, NULL, scc);

    sccReset(scc);

    return scc;
}

void sccDestroy(SCC* scc)
{
//    debugDeviceUnregister(scc->debugHandle);
    mixerUnregisterChannel(scc->mixer, scc->handle);
    free(scc);
}

UInt8 sccRead(SCC* scc, UInt8 address)
{
    switch (scc->mode) {

    case SCC_REAL:
        if (address < 0x80) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xa0) {
            return sccGetFreqAndVol(scc, address);
        } 
        
        if (address < 0xe0) {
            return 0xff;
        }

        sccUpdateDeformation(scc, 0xff);

        return 0xff;

    case SCC_COMPATIBLE:
        if (address < 0x80) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xa0) {
            return sccGetFreqAndVol(scc, address);
        }
        
        if (address < 0xc0) {
            return sccGetWave(scc, 4, address);
        } 

        if (address < 0xe0) {
            sccUpdateDeformation(scc, 0xff);
            return 0xff;
        }
 
        return 0xff;

    case SCC_PLUS:
        if (address < 0xa0) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xc0) {
            return sccGetFreqAndVol(scc, address);
        } 
        
        if (address < 0xe0) {
            sccUpdateDeformation(scc, 0xff);
            return 0xff;
        }

        return 0xff;
    }

    return 0xff;
}

UInt8 sccPeek(SCC* scc, UInt8 address)
{
    UInt8 result;

    switch (scc->mode) {

    case SCC_REAL:
        if (address < 0x80) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xa0) {
            return sccGetFreqAndVol(scc, address);
        } 
        
        if (address < 0xe0) {
            return 0xff;
        }

        return 0xff;

    case SCC_COMPATIBLE:
        if (address < 0x80) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xa0) {
            return sccGetFreqAndVol(scc, address);
        }
        
        if (address < 0xc0) {
            result = sccGetWave(scc, 4, address);
        } 

        if (address < 0xe0) {
            return 0xff;
        }
 
        result = 0xff;

    case SCC_PLUS:
        if (address < 0xa0) {
            return sccGetWave(scc, address >> 5, address);
        } 
        
        if (address < 0xc0) {
            return sccGetFreqAndVol(scc, address);
        } 
        
        if (address < 0xe0) {
            return 0xff;
        }

        return 0xff;
    }

    return 0xff;
}

void sccWrite(SCC* scc, UInt8 address, UInt8 value)
{
    mixerSync(scc->mixer);

    switch (scc->mode) {
    case SCC_REAL:
        if (address < 0x80) {
            sccUpdateWave(scc, address >> 5, address, value);
            return;
        } 
        
        if (address < 0xa0) {
            sccUpdateFreqAndVol(scc, address, value);
            return;
        } 
        
        if (address < 0xe0) {
            return;
        }

        sccUpdateDeformation(scc, value);
        return;

    case SCC_COMPATIBLE:
        if (address < 0x80) {
            sccUpdateWave(scc, address >> 5, address, value);
            return;
        } 
        
        if (address < 0xa0) {
            sccUpdateFreqAndVol(scc, address, value);
            return;
        } 
        
        if (address < 0xc0) {
            return;
        } 
        
        if (address < 0xe0) {
            sccUpdateDeformation(scc, value);
            return;
        } 

        return;

    case SCC_PLUS:
        if (address < 0xa0) {
            sccUpdateWave(scc, address >> 5, address, value);
            return;
        } 
        
        if (address < 0xc0) {
            sccUpdateFreqAndVol(scc, address, value);
            return;
        } 
        
        if (address < 0xe0) {
            sccUpdateDeformation(scc, value);
            return;
        }

        return;
    }
}

void sccGetDebugInfo(SCC* scc, DbgDevice* dbgDevice)
{
}

static Int32 filter(SCC* scc, Int32 input) {
    scc->in[4] = scc->in[3];
    scc->in[3] = scc->in[2];
    scc->in[2] = scc->in[1];
    scc->in[1] = scc->in[0];
    scc->in[0] = input;

    scc->inHp[2] = scc->inHp[1];
    scc->inHp[1] = scc->inHp[0];
    scc->inHp[0] = (1 * (scc->in[0] + scc->in[4]) + 12 * (scc->in[1] + scc->in[3]) + 45 * scc->in[2]) / 100;

    scc->outHp[2] = scc->outHp[1];
    scc->outHp[1] = scc->outHp[0];
    scc->outHp[0] =(997 * scc->inHp[0] - 1994 * scc->inHp[1] + 997 * scc->inHp[2] + 1994 * scc->outHp[1] - 994 * scc->outHp[2]) / 1000;

    return scc->outHp[0];
}

// Filter type: Low pass
// Passband: 0.0 - 750.0 Hz
// Order: 94
// Transition band: 250.0 Hz
// Stopband attenuation: 50.0 dB
//
static Int32 filter4(SCC* scc, Int32 in1, Int32 in2, Int32 in3, Int32 in4)
{
    int i;
    DoubleT res;

    for (i = 0; i < 91; ++i) {
        scc->in[i] = scc->in[i + 4];
    }
    scc->in[91] = in1;
    scc->in[92] = in2;
    scc->in[93] = in3;
    scc->in[94] = in4;

    res =  2.8536195E-4 * (scc->in[ 0] + scc->in[94]) +
           9.052306E-5  * (scc->in[ 1] + scc->in[93]) +
          -2.6902245E-4 * (scc->in[ 2] + scc->in[92]) +
          -6.375284E-4  * (scc->in[ 3] + scc->in[91]) +
          -7.87536E-4   * (scc->in[ 4] + scc->in[90]) +
          -5.3910224E-4 * (scc->in[ 5] + scc->in[89]) +
           1.1107049E-4 * (scc->in[ 6] + scc->in[88]) +
           9.2801993E-4 * (scc->in[ 7] + scc->in[87]) +
           0.0015018889 * (scc->in[ 8] + scc->in[86]) +
           0.0014338732 * (scc->in[ 9] + scc->in[85]) +
           5.688559E-4  * (scc->in[10] + scc->in[84]) +
          -8.479743E-4  * (scc->in[11] + scc->in[83]) +
          -0.0021999443 * (scc->in[12] + scc->in[82]) +
          -0.0027432537 * (scc->in[13] + scc->in[81]) +
          -0.0019824558 * (scc->in[14] + scc->in[80]) +
           2.018935E-9  * (scc->in[15] + scc->in[79]) +
           0.0024515253 * (scc->in[16] + scc->in[78]) +
           0.00419754   * (scc->in[17] + scc->in[77]) +
           0.0041703423 * (scc->in[18] + scc->in[76]) +
           0.0019952168 * (scc->in[19] + scc->in[75]) +
          -0.0016656333 * (scc->in[20] + scc->in[74]) +
          -0.005242034  * (scc->in[21] + scc->in[73]) +
          -0.0068841926 * (scc->in[22] + scc->in[72]) +
          -0.005360789  * (scc->in[23] + scc->in[71]) +
          -8.1365916E-4 * (scc->in[24] + scc->in[70]) +
           0.0050464263 * (scc->in[25] + scc->in[69]) +
           0.00950725   * (scc->in[26] + scc->in[68]) +
           0.010038091  * (scc->in[27] + scc->in[67]) +
           0.005602208  * (scc->in[28] + scc->in[66]) +
          -0.00253724   * (scc->in[29] + scc->in[65]) +
          -0.011011368  * (scc->in[30] + scc->in[64]) +
          -0.015622435  * (scc->in[31] + scc->in[63]) +
          -0.013267951  * (scc->in[32] + scc->in[62]) +
          -0.0036876823 * (scc->in[33] + scc->in[61]) +
           0.009843254  * (scc->in[34] + scc->in[60]) +
           0.021394625  * (scc->in[35] + scc->in[59]) +
           0.02469893   * (scc->in[36] + scc->in[58]) +
           0.01608393   * (scc->in[37] + scc->in[57]) +
          -0.0032088074 * (scc->in[38] + scc->in[56]) +
          -0.026453404  * (scc->in[39] + scc->in[55]) +
          -0.043139543  * (scc->in[40] + scc->in[54]) +
          -0.042553578  * (scc->in[41] + scc->in[53]) +
          -0.018007802  * (scc->in[42] + scc->in[52]) +
           0.029919287  * (scc->in[43] + scc->in[51]) +
           0.09252273   * (scc->in[44] + scc->in[50]) +
           0.15504532   * (scc->in[45] + scc->in[49]) +
           0.20112106   * (scc->in[46] + scc->in[48]) +
           0.2180678    *  scc->in[47];

    return (Int32)res;
}

static Int32* sccSync(SCC* scc, UInt32 count)
{
    Int32* buffer  = scc->buffer;
    Int32  channel;
    UInt32 index;

    for (index = 0; index < count; index++) {
        Int32 masterVolume[4] = {0, 0, 0, 0};
        int i;
        for (i = 0; i < 4; i++) {
            for (channel = 0; channel < 5; channel++) {
                Int32 refVolume;
                Int32 phase;
                Int32 sample;

                phase = scc->phase[channel] + scc->phaseStep[channel];
                phase &= 0xfffffff;
                scc->phase[channel] = phase;

                sample = (phase >> 23) & 0x1f;

                if (sample != scc->oldSample[channel]) {
                    scc->volume[channel] = scc->nextVolume[channel];

#if 0
                    if ((sample == 15 || sample == 16) && scc->bus != 0xFFFF) {
                        scc->curWave[channel] = (UInt8)scc->bus;
                    }
                    else {
                        scc->curWave[channel] = scc->wave[channel][sample];
                    }
#else
                    scc->curWave[channel] = scc->wave[channel][sample];
#endif

                    scc->oldSample[channel] = sample;   
                }

                refVolume = 25 * ((scc->enable >> channel) & 1) * (Int32)scc->volume[channel];
                if (scc->daVolume[channel] < refVolume) {
                    scc->daVolume[channel] = refVolume;
                }

                masterVolume[i] += scc->curWave[channel] * scc->daVolume[channel];
                
                if (scc->daVolume[channel] > refVolume) {
                    scc->daVolume[channel] = scc->daVolume[channel] * 9 / 10;
                }
            }
        }
        buffer[index] = filter4(scc, masterVolume[0], masterVolume[1], masterVolume[2], masterVolume[3]);
        scc->bus = 0xFFFF;

//        buffer[index] = (masterVolume[0] + masterVolume[1] + masterVolume[2] + masterVolume[3]);
    }

    return scc->buffer;
}

