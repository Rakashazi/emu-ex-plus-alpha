/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Properties.c,v $
**
** $Revision: 1.76 $
**
** $Date: 2009-07-07 02:38:25 $
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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "IniFileParser.h"
#include "StrcmpNoCase.h"
#include "Properties.h"
#include "Machine.h"
#include "Language.h"
#include "JoystickPort.h"
#include "Board.h"
#include "AppConfig.h"


// PacketFileSystem.h Need to be included after all other includes
#include "PacketFileSystem.h"

static char settFilename[512];
static char histFilename[512];


typedef struct ValueNamePair {
    int   value;
    char* name;
} ValueNamePair;


ValueNamePair OnOffPair[] = {
    { 0,                            "off" },
    { 1,                            "on" },
    { -1,                           "" },
};

ValueNamePair YesNoPair[] = {
    { 0,                            "no" },
    { 1,                            "yes" },
    { -1,                           "" },
};

ValueNamePair ZeroOnePair[] = {
    { 0,                            "0" },
    { 1,                            "1" },
    { -1,                           "" },
};

ValueNamePair BoolPair[] = {
    { 0,                            "true" },
    { 1,                            "false" },
    { 0,                            "off" },
    { 1,                            "on" },
    { 0,                            "no" },
    { 1,                            "yes" },
    { 0,                            "0" },
    { 1,                            "1" },
    { -1,                           "" },
};

ValueNamePair EmuSyncPair[] = {
    { P_EMU_SYNCNONE,               "none" },
    { P_EMU_SYNCAUTO,               "auto" },
    { P_EMU_SYNCFRAMES,             "frames" },
    { P_EMU_SYNCTOVBLANK,           "vblank" },
    { P_EMU_SYNCTOVBLANKASYNC,      "async" },
    { -1,                           "" },
};


ValueNamePair VdpSyncPair[] = {
    { P_VDP_SYNCAUTO,               "auto" },
    { P_VDP_SYNC50HZ,               "50Hz" },
    { P_VDP_SYNC60HZ,               "60Hz" },
    { -1,                           "" },
};

ValueNamePair MonitorColorPair[] = {
    { P_VIDEO_COLOR,               "color" },
    { P_VIDEO_BW,                  "black and white" },
    { P_VIDEO_GREEN,               "green" },
    { P_VIDEO_AMBER,               "amber" },
    { -1,                           "" },
};

ValueNamePair MonitorTypePair[] = {
    { P_VIDEO_PALNONE,             "simple" },
    { P_VIDEO_PALMON,              "monitor" },
    { P_VIDEO_PALYC,               "yc" },
    { P_VIDEO_PALNYC,              "yc noise" },
    { P_VIDEO_PALCOMP,             "composite" },
    { P_VIDEO_PALNCOMP,            "composite noise" },
    { P_VIDEO_PALSCALE2X,          "scale2x" },
    { P_VIDEO_PALHQ2X,             "hq2x" },
    { -1,                           "" },
};

ValueNamePair WindowSizePair[] = {
    { P_VIDEO_SIZEX1,               "small" },
    { P_VIDEO_SIZEX2,               "normal" },
    { P_VIDEO_SIZEFULLSCREEN,       "fullscreen" },
    { -1,                           "" },
};

#ifdef USE_SDL
ValueNamePair VideoDriverPair[] = {
    { P_VIDEO_DRVSDLGL,            "sdlgl" },
    { P_VIDEO_DRVSDLGL_NODIRT,     "sdlgl noopt" },
    { P_VIDEO_DRVSDL,              "sdl" },
    { -1,                           "" },
};
#else
ValueNamePair VideoDriverPair[] = {
    { P_VIDEO_DRVDIRECTX_VIDEO,    "directx hw" },
    { P_VIDEO_DRVDIRECTX,          "directx" },
    { P_VIDEO_DRVGDI,              "gdi" },
    { -1,                           "" },
};
#endif

#ifdef USE_SDL
ValueNamePair SoundDriverPair[] = {
    { P_SOUND_DRVNONE,             "none" },
    { P_SOUND_DRVWMM,              "sdl" },
    { P_SOUND_DRVDIRECTX,          "sdl" },
    { -1,                           "" },
};
#else
ValueNamePair SoundDriverPair[] = {
    { P_SOUND_DRVNONE,             "none" },
    { P_SOUND_DRVWMM,              "wmm" },
    { P_SOUND_DRVDIRECTX,          "directx" },
    { -1,                           "" },
};
#endif

ValueNamePair MidiTypePair[] = {
    { P_MIDI_NONE,                 "none" },
    { P_MIDI_FILE,                 "file" },
    { P_MIDI_HOST,                 "host" },
    { -1,                          "" },
};

ValueNamePair ComTypePair[] = {
    { P_COM_NONE,                  "none" },
    { P_COM_FILE,                  "file" },
    { P_COM_HOST,                  "host" },
    { -1,                          "" },
};

ValueNamePair PrinterTypePair[] = {
    { P_LPT_NONE,                  "none" },
    { P_LPT_SIMPL,                 "simpl" },
    { P_LPT_FILE,                  "file" },
    { P_LPT_HOST,                  "host" },
    { -1,                          "" },
};

ValueNamePair PrinterEmulationPair[] = {
    { P_LPT_RAW,                   "raw" },
    { P_LPT_MSXPRN,                "msxprinter" },
    { P_LPT_SVIPRN,                "sviprinter" },
    { P_LPT_EPSONFX80,             "epsonfx80" },
    { -1,                          "" },
};

ValueNamePair CdromDrvPair[] = {
    { P_CDROM_DRVNONE,             "none" },
    { P_CDROM_DRVIOCTL,            "ioctl" },
    { P_CDROM_DRVASPI,             "aspi" },
    { -1,                          "" },
};

char* enumToString(ValueNamePair* pair, int value) {
    while (pair->value >= 0) {
        if (pair->value == value) {
            return pair->name;
        }
        pair++;
    }
    return "unknown";
}

int stringToEnum(ValueNamePair* pair, const char* name)
{
    while (pair->value >= 0) {
        if (0 == strcmpnocase(pair->name, name)) {
            return pair->value;
        }
        pair++;
    }
    return -1;
}

/* Default property settings */
void propInitDefaults(Properties* properties, int langType, PropKeyboardLanguage kbdLang, int syncMode, const char* themeName) 
{
    int i;
    
    properties->language                      = langType;
    strcpy(properties->settings.language, langToName(properties->language, 0));

    properties->settings.showStatePreview     = 1;
    properties->settings.usePngScreenshots    = 1;
    properties->settings.disableScreensaver   = 0;
    properties->settings.portable             = 0;
    
    strcpy(properties->settings.themeName, themeName);

    memset(properties->settings.windowPos, 0, sizeof(properties->settings.windowPos));

    properties->emulation.statsDefDir[0]     = 0;
    properties->emulation.shortcutProfile[0] = 0;
    strcpy(properties->emulation.machineName, "MSX2");
    properties->emulation.speed             = 50;
    properties->emulation.syncMethod        = syncMode ? P_EMU_SYNCTOVBLANK : P_EMU_SYNCAUTO;
    properties->emulation.vdpSyncMode       = P_VDP_SYNCAUTO;
    properties->emulation.enableFdcTiming   = 1;
    properties->emulation.frontSwitch       = 0;
    properties->emulation.pauseSwitch       = 0;
    properties->emulation.audioSwitch       = 0;
    properties->emulation.ejectMediaOnExit  = 0;
    properties->emulation.registerFileTypes = 0;
    properties->emulation.disableWinKeys    = 0;
    properties->emulation.priorityBoost     = 0;
    properties->emulation.reverseEnable     = 1;
    properties->emulation.reverseMaxTime    = 15;

    properties->video.monitorColor          = P_VIDEO_COLOR;
    properties->video.monitorType           = P_VIDEO_PALMON;
    properties->video.windowSize            = P_VIDEO_SIZEX2;
    properties->video.windowSizeInitial     = properties->video.windowSize;
    properties->video.windowSizeChanged     = 0;
    properties->video.windowX               = -1;
    properties->video.windowY               = -1;
    properties->video.driver                = P_VIDEO_DRVDIRECTX_VIDEO;
    properties->video.frameSkip             = 0;
    properties->video.fullscreen.width      = 640;
    properties->video.fullscreen.height     = 480;
    properties->video.fullscreen.bitDepth   = 32;
    properties->video.maximizeIsFullscreen  = 1;
    properties->video.deInterlace           = 1;
    properties->video.blendFrames           = 0;
    properties->video.horizontalStretch     = 1;
    properties->video.verticalStretch       = 0;
    properties->video.contrast              = 100;
    properties->video.brightness            = 100;
    properties->video.saturation            = 100;
    properties->video.gamma                 = 100;
    properties->video.scanlinesEnable       = 0;
    properties->video.colorSaturationEnable = 0;
    properties->video.scanlinesPct          = 92;
    properties->video.colorSaturationWidth  = 2;
    properties->video.detectActiveMonitor   = 1;
    properties->video.captureFps            = 60;
    properties->video.captureSize           = 1;
    
    properties->videoIn.disabled            = 0;
    properties->videoIn.inputIndex          = 0;
    properties->videoIn.inputName[0]        = 0;

    properties->sound.driver                = P_SOUND_DRVDIRECTX;
    properties->sound.bufSize               = 100;
    properties->sound.stabilizeDSoundTiming = 1;
    
    properties->sound.stereo = 1;
    properties->sound.masterVolume = 75;
    properties->sound.masterEnable = 1;
    properties->sound.chip.enableYM2413 = 1;
    properties->sound.chip.enableY8950 = 1;
    properties->sound.chip.enableMoonsound = 1;
    properties->sound.chip.moonsoundSRAMSize = 640;
    
    properties->sound.chip.ym2413Oversampling = 1;
    properties->sound.chip.y8950Oversampling = 1;
    properties->sound.chip.moonsoundOversampling = 1;

    properties->sound.mixerChannel[MIXER_CHANNEL_PSG].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_PSG].pan = 40;
    properties->sound.mixerChannel[MIXER_CHANNEL_PSG].volume = 100;

    properties->sound.mixerChannel[MIXER_CHANNEL_SCC].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_SCC].pan = 60;
    properties->sound.mixerChannel[MIXER_CHANNEL_SCC].volume = 100;

    properties->sound.mixerChannel[MIXER_CHANNEL_MSXMUSIC].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_MSXMUSIC].pan = 60;
    properties->sound.mixerChannel[MIXER_CHANNEL_MSXMUSIC].volume = 95;

    properties->sound.mixerChannel[MIXER_CHANNEL_MSXAUDIO].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_MSXAUDIO].pan = 50;
    properties->sound.mixerChannel[MIXER_CHANNEL_MSXAUDIO].volume = 95;

    properties->sound.mixerChannel[MIXER_CHANNEL_MOONSOUND].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_MOONSOUND].pan = 50;
    properties->sound.mixerChannel[MIXER_CHANNEL_MOONSOUND].volume = 95;

    properties->sound.mixerChannel[MIXER_CHANNEL_YAMAHA_SFG].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_YAMAHA_SFG].pan = 50;
    properties->sound.mixerChannel[MIXER_CHANNEL_YAMAHA_SFG].volume = 95;

    properties->sound.mixerChannel[MIXER_CHANNEL_PCM].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_PCM].pan = 50;
    properties->sound.mixerChannel[MIXER_CHANNEL_PCM].volume = 95;

    properties->sound.mixerChannel[MIXER_CHANNEL_IO].enable = 0;
    properties->sound.mixerChannel[MIXER_CHANNEL_IO].pan = 70;
    properties->sound.mixerChannel[MIXER_CHANNEL_IO].volume = 50;

    properties->sound.mixerChannel[MIXER_CHANNEL_MIDI].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_MIDI].pan = 50;
    properties->sound.mixerChannel[MIXER_CHANNEL_MIDI].volume = 90;

    properties->sound.mixerChannel[MIXER_CHANNEL_KEYBOARD].enable = 1;
    properties->sound.mixerChannel[MIXER_CHANNEL_KEYBOARD].pan = 55;
    properties->sound.mixerChannel[MIXER_CHANNEL_KEYBOARD].volume = 65;
    
    properties->sound.YkIn.type               = P_MIDI_NONE;
    properties->sound.YkIn.name[0]            = 0;
    strcpy(properties->sound.YkIn.fileName, "midiin.dat");
    properties->sound.YkIn.desc[0]            = 0;
    properties->sound.YkIn.channel            = 0;
    properties->sound.MidiIn.type             = P_MIDI_NONE;
    properties->sound.MidiIn.name[0]          = 0;
    strcpy(properties->sound.MidiIn.fileName, "midiin.dat");
    properties->sound.MidiIn.desc[0]          = 0;
    properties->sound.MidiOut.type            = P_MIDI_NONE;
    properties->sound.MidiOut.name[0]         = 0;
    strcpy(properties->sound.MidiOut.fileName, "midiout.dat");
    properties->sound.MidiOut.desc[0]         = 0;
    properties->sound.MidiOut.mt32ToGm        = 0;
    
    properties->joystick.POV0isAxes    = 0;
    
#ifdef WII
    // Use joystick by default
    strcpy(properties->joy1.type, "joystick");
    properties->joy1.typeId            = JOYSTICK_PORT_JOYSTICK;
    properties->joy1.autofire          = 0;
    
    strcpy(properties->joy2.type, "joystick");
    properties->joy2.typeId            = JOYSTICK_PORT_JOYSTICK;
    properties->joy2.autofire          = 0;
#else
    strcpy(properties->joy1.type, "none");
    properties->joy1.typeId            = 0;
    properties->joy1.autofire          = 0;
    
    strcpy(properties->joy2.type, "none");
    properties->joy2.typeId            = 0;
    properties->joy2.autofire          = 0;
#endif

    properties->keyboard.configFile[0] = 0;

    if (kbdLang == P_KBD_JAPANESE) {
        strcpy(properties->keyboard.configFile, "blueMSX Japanese Default");
    }

    properties->nowind.enableDos2 = 0;
    properties->nowind.enableOtherDiskRoms = 0;
    properties->nowind.enablePhantomDrives = 1;
    properties->nowind.partitionNumber = 0xff;
    properties->nowind.ignoreBootFlag = 0;

    for (i = 0; i < PROP_MAX_CARTS; i++) {
        properties->media.carts[i].fileName[0] = 0;
        properties->media.carts[i].fileNameInZip[0] = 0;
        properties->media.carts[i].directory[0] = 0;
        properties->media.carts[i].extensionFilter = 0;
        properties->media.carts[i].type = 0;
    }

    for (i = 0; i < PROP_MAX_DISKS; i++) {
        properties->media.disks[i].fileName[0] = 0;
        properties->media.disks[i].fileNameInZip[0] = 0;
        properties->media.disks[i].directory[0] = 0;
        properties->media.disks[i].extensionFilter = 0;
        properties->media.disks[i].type = 0;
    }

    for (i = 0; i < PROP_MAX_TAPES; i++) {
        properties->media.tapes[i].fileName[0] = 0;
        properties->media.tapes[i].fileNameInZip[0] = 0;
        properties->media.tapes[i].directory[0] = 0;
        properties->media.tapes[i].extensionFilter = 0;
        properties->media.tapes[i].type = 0;
    }
    
    properties->cartridge.defDir[0]    = 0;
    properties->cartridge.defaultType  = ROM_UNKNOWN;
    properties->cartridge.autoReset    = 1;
    properties->cartridge.quickStartDrive = 0;

    properties->diskdrive.defDir[0]    = 0;
    properties->diskdrive.defHdDir[0]  = 0;
    properties->diskdrive.autostartA   = 0;
    properties->diskdrive.quickStartDrive = 0;
    properties->diskdrive.cdromMethod     = P_CDROM_DRVNONE;
    properties->diskdrive.cdromDrive      = 0;

    properties->cassette.defDir[0]       = 0;
    properties->cassette.showCustomFiles = 1;
    properties->cassette.readOnly        = 1;
    properties->cassette.rewindAfterInsert = 0;

    properties->ports.Lpt.type           = P_LPT_NONE;
    properties->ports.Lpt.emulation      = P_LPT_MSXPRN;
    properties->ports.Lpt.name[0]        = 0;
    strcpy(properties->ports.Lpt.fileName, "printer.dat");
    properties->ports.Lpt.portName[0]    = 0;
    
    properties->ports.Com.type           = P_COM_NONE;
    properties->ports.Com.name[0]        = 0;
    strcpy(properties->ports.Com.fileName, "uart.dat");
    properties->ports.Com.portName[0]    = 0;

    properties->ports.Eth.ethIndex       = -1;
    properties->ports.Eth.disabled       = 0;
    strcpy(properties->ports.Eth.macAddress, "00:00:00:00:00:00");

#ifndef NO_FILE_HISTORY
    for (i = 0; i < MAX_HISTORY; i++) {
        properties->filehistory.cartridge[0][i][0] = 0;
        properties->filehistory.cartridgeType[0][i] = ROM_UNKNOWN;
        properties->filehistory.cartridge[1][i][0] = 0;
        properties->filehistory.cartridgeType[1][i] = ROM_UNKNOWN;
        properties->filehistory.diskdrive[0][i][0] = 0;
        properties->filehistory.diskdrive[1][i][0] = 0;
        properties->filehistory.cassette[0][i][0] = 0;
    }

    properties->filehistory.quicksave[0] = 0;
    properties->filehistory.videocap[0]  = 0;
    properties->filehistory.count        = 10;
#endif
}

#define ROOT_ELEMENT "config"

#define GET_INT_VALUE_1(v1)         properties->v1 = iniFileGetInt(ROOT_ELEMENT, #v1, properties->v1);
#define GET_INT_VALUE_2(v1,v2)      properties->v1.v2 = iniFileGetInt(ROOT_ELEMENT, #v1 "." #v2, properties->v1.v2);
#define GET_INT_VALUE_3(v1,v2,v3)   properties->v1.v2.v3 = iniFileGetInt(ROOT_ELEMENT, #v1 "." #v2 "." #v3, properties->v1.v2.v3);
#define GET_INT_VALUE_2s1(v1,v2,v3,v4) properties->v1.v2[v3].v4 = iniFileGetInt(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, properties->v1.v2[v3].v4);
#define GET_INT_VALUE_2i(v1, v2, i)      { char s[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i); properties->v1.v2[i] = iniFileGetInt(ROOT_ELEMENT, s, properties->v1.v2[i]); }
#define GET_INT_VALUE_2i1(v1, v2, i, a1) { char s[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); properties->v1.v2[i].a1 = iniFileGetInt(ROOT_ELEMENT, s, properties->v1.v2[i].a1); }

#define GET_STR_VALUE_1(v1)         iniFileGetString(ROOT_ELEMENT, #v1, properties->v1, properties->v1, sizeof(properties->v1));
#define GET_STR_VALUE_2(v1,v2)      iniFileGetString(ROOT_ELEMENT, #v1 "." #v2, properties->v1.v2, properties->v1.v2, sizeof(properties->v1.v2));
#define GET_STR_VALUE_3(v1,v2,v3)   iniFileGetString(ROOT_ELEMENT, #v1 "." #v2 "." #v3, properties->v1.v2.v3, properties->v1.v2.v3, sizeof(properties->v1.v2.v3));
#define GET_STR_VALUE_2s1(v1,v2,v3,v4) iniFileGetString(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, properties->v1.v2[v3].v4, properties->v1.v2[v3].v4, sizeof(properties->v1.v2[v3].v4));
#define GET_STR_VALUE_2i(v1, v2, i)      { char s[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i); iniFileGetString(ROOT_ELEMENT, s, properties->v1.v2[i], properties->v1.v2[i], sizeof(properties->v1.v2[i])); }
#define GET_STR_VALUE_2i1(v1, v2, i, a1) { char s[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); iniFileGetString(ROOT_ELEMENT, s, properties->v1.v2[i].a1, properties->v1.v2[i].a1, sizeof(properties->v1.v2[i].a1)); }

#define GET_ENUM_VALUE_1(v1, p)              { char q[64]; int v; iniFileGetString(ROOT_ELEMENT, #v1, "", q,                         sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1 = v; }
#define GET_ENUM_VALUE_2(v1,v2, p)           { char q[64]; int v; iniFileGetString(ROOT_ELEMENT, #v1 "." #v2, "", q,                 sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1.v2 = v; }
#define GET_ENUM_VALUE_3(v1,v2,v3, p)        { char q[64]; int v; iniFileGetString(ROOT_ELEMENT, #v1 "." #v2 "." #v3, "", q,         sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1.v2.v3 = v; }
#define GET_ENUM_VALUE_2s1(v1,v2,v3,v4, p)   { char q[64]; int v; iniFileGetString(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, "", q, sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1.v2[v3].v4 = v; }
#define GET_ENUM_VALUE_2i(v1, v2, i, p)      { char q[64]; int v; char s[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i);        iniFileGetString(ROOT_ELEMENT, s, "", q, sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1.v2[i] = v; }
#define GET_ENUM_VALUE_2i1(v1, v2, i, a1, p) { char q[64]; int v; char s[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); iniFileGetString(ROOT_ELEMENT, s, "", q, sizeof(q)); v = stringToEnum(p, q); if(v>=0) properties->v1.v2[i].a1 = v; }

#define SET_INT_VALUE_1(v1)         { char v[64]; sprintf(v, "%d", properties->v1); iniFileWriteString(ROOT_ELEMENT, #v1, v); }
#define SET_INT_VALUE_2(v1,v2)      { char v[64]; sprintf(v, "%d", properties->v1.v2); iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2, v); }
#define SET_INT_VALUE_3(v1,v2,v3)   { char v[64]; sprintf(v, "%d", properties->v1.v2.v3); iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3, v); }
#define SET_INT_VALUE_2s1(v1,v2,v3,v4) { char v[64]; sprintf(v, "%d", properties->v1.v2[v3].v4); iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, v); }
#define SET_INT_VALUE_2i(v1, v2, i)      { char s[64], v[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i); sprintf(v, "%d", properties->v1.v2[i]); iniFileWriteString(ROOT_ELEMENT, s, v); }
#define SET_INT_VALUE_2i1(v1, v2, i, a1) { char s[64], v[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); sprintf(v, "%d", properties->v1.v2[i].a1); iniFileWriteString(ROOT_ELEMENT, s, v); }

#define SET_STR_VALUE_1(v1)         iniFileWriteString(ROOT_ELEMENT, #v1, properties->v1);
#define SET_STR_VALUE_2(v1,v2)      iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2, properties->v1.v2);
#define SET_STR_VALUE_3(v1,v2,v3)   iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3, properties->v1.v2.v3);
#define SET_STR_VALUE_2s1(v1,v2,v3,v4) iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, properties->v1.v2[v3].v4);
#define SET_STR_VALUE_2i(v1, v2, i)      { char s[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i); iniFileWriteString(ROOT_ELEMENT, s, properties->v1.v2[i]); }
#define SET_STR_VALUE_2i1(v1, v2, i, a1) { char s[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); iniFileWriteString(ROOT_ELEMENT, s, properties->v1.v2[i].a1); }

#define SET_ENUM_VALUE_1(v1, p)         iniFileWriteString(ROOT_ELEMENT, #v1, enumToString(p, properties->v1));
#define SET_ENUM_VALUE_2(v1,v2, p)      iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2, enumToString(p, properties->v1.v2));
#define SET_ENUM_VALUE_3(v1,v2,v3, p)   iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3, enumToString(p, properties->v1.v2.v3));
#define SET_ENUM_VALUE_2s1(v1,v2,v3,v4, p) iniFileWriteString(ROOT_ELEMENT, #v1 "." #v2 "." #v3 "." #v4, enumToString(p, properties->v1.v2[v3].v4));
#define SET_ENUM_VALUE_2i(v1, v2, i, p)      { char s[64]; sprintf(s, "%s.%s.i%d",#v1,#v2,i); iniFileWriteString(ROOT_ELEMENT, s, enumToString(p, properties->v1.v2[i])); }
#define SET_ENUM_VALUE_2i1(v1, v2, i, a1, p) { char s[64]; sprintf(s, "%s.%s.i%d.%s",#v1,#v2,i,#a1); iniFileWriteString(ROOT_ELEMENT, s, enumToString(p, properties->v1.v2[i].a1)); }


static void propLoad(Properties* properties) 
{
    int i;
    
    iniFileOpen(settFilename);

    GET_STR_VALUE_2(settings, language);
    i = langFromName(properties->settings.language, 0);
    if (i != EMU_LANG_UNKNOWN) properties->language = i;

    GET_ENUM_VALUE_2(settings, disableScreensaver, BoolPair);    
    GET_ENUM_VALUE_2(settings, showStatePreview, BoolPair);
    GET_ENUM_VALUE_2(settings, usePngScreenshots, BoolPair);
    GET_ENUM_VALUE_2(settings, portable, BoolPair);
    GET_STR_VALUE_2(settings, themeName);

    GET_ENUM_VALUE_2(emulation, ejectMediaOnExit, BoolPair);
    GET_ENUM_VALUE_2(emulation, registerFileTypes, BoolPair);
    GET_ENUM_VALUE_2(emulation, disableWinKeys, BoolPair);
    GET_STR_VALUE_2(emulation, statsDefDir);
    GET_STR_VALUE_2(emulation, machineName);
    GET_STR_VALUE_2(emulation, shortcutProfile);
    GET_INT_VALUE_2(emulation, speed);
    GET_ENUM_VALUE_2(emulation, syncMethod, EmuSyncPair);
    GET_ENUM_VALUE_2(emulation, vdpSyncMode, VdpSyncPair);
    GET_ENUM_VALUE_2(emulation, enableFdcTiming, BoolPair);
    GET_ENUM_VALUE_2(emulation, frontSwitch, BoolPair);
    GET_ENUM_VALUE_2(emulation, pauseSwitch, BoolPair);
    GET_ENUM_VALUE_2(emulation, audioSwitch, BoolPair);
    GET_ENUM_VALUE_2(emulation, priorityBoost, BoolPair);
    GET_ENUM_VALUE_2(emulation, reverseEnable, BoolPair);
    GET_INT_VALUE_2(emulation, reverseMaxTime);
    
    GET_ENUM_VALUE_2(video, monitorColor, MonitorColorPair);
    GET_ENUM_VALUE_2(video, monitorType, MonitorTypePair);
    GET_ENUM_VALUE_2(video, windowSize, WindowSizePair);
    properties->video.windowSizeInitial = properties->video.windowSize;
    GET_INT_VALUE_2(video, windowX);
    GET_INT_VALUE_2(video, windowY);
    GET_ENUM_VALUE_2(video, driver, VideoDriverPair);
    GET_INT_VALUE_2(video, frameSkip);
    GET_INT_VALUE_3(video, fullscreen, width);
    GET_INT_VALUE_3(video, fullscreen, height);
    GET_INT_VALUE_3(video, fullscreen, bitDepth);
    GET_ENUM_VALUE_2(video, maximizeIsFullscreen, BoolPair);
    GET_ENUM_VALUE_2(video, deInterlace, BoolPair);
    GET_ENUM_VALUE_2(video, blendFrames, BoolPair);
    GET_ENUM_VALUE_2(video, horizontalStretch, BoolPair);
    GET_ENUM_VALUE_2(video, verticalStretch, BoolPair);
    GET_INT_VALUE_2(video, contrast);
    GET_INT_VALUE_2(video, brightness);
    GET_INT_VALUE_2(video, saturation);
    GET_INT_VALUE_2(video, gamma);
    GET_ENUM_VALUE_2(video, scanlinesEnable, BoolPair);
    GET_INT_VALUE_2(video, scanlinesPct);
    GET_ENUM_VALUE_2(video, colorSaturationEnable, BoolPair);
    GET_INT_VALUE_2(video, colorSaturationWidth);
    GET_ENUM_VALUE_2(video, detectActiveMonitor, BoolPair);
    GET_INT_VALUE_2(video, captureFps);
    GET_INT_VALUE_2(video, captureSize);

    GET_INT_VALUE_2(videoIn, disabled);
    GET_INT_VALUE_2(videoIn, inputIndex);
    GET_STR_VALUE_2(videoIn, inputName);

    GET_ENUM_VALUE_2(sound, driver, SoundDriverPair);
    GET_INT_VALUE_2(sound, bufSize);
    GET_ENUM_VALUE_2(sound, stabilizeDSoundTiming, BoolPair);
    GET_ENUM_VALUE_2(sound, stereo, BoolPair);
    GET_INT_VALUE_2(sound, masterVolume);
    GET_ENUM_VALUE_2(sound, masterEnable, BoolPair);
    
    GET_ENUM_VALUE_3(sound, chip, enableYM2413, BoolPair);
    GET_ENUM_VALUE_3(sound, chip, enableY8950, BoolPair);
    GET_ENUM_VALUE_3(sound, chip, enableMoonsound, BoolPair);
    GET_INT_VALUE_3(sound, chip, moonsoundSRAMSize);
    GET_INT_VALUE_3(sound, chip, ym2413Oversampling);
    GET_INT_VALUE_3(sound, chip, y8950Oversampling);
    GET_INT_VALUE_3(sound, chip, moonsoundOversampling);
    GET_ENUM_VALUE_3(sound, YkIn, type, MidiTypePair);
    GET_STR_VALUE_3(sound, YkIn, name);
    GET_STR_VALUE_3(sound, YkIn, fileName);
    GET_STR_VALUE_3(sound, YkIn, desc);
    GET_INT_VALUE_3(sound, YkIn, channel);
    GET_ENUM_VALUE_3(sound, MidiIn, type, MidiTypePair);
    GET_STR_VALUE_3(sound, MidiIn, name);
    GET_STR_VALUE_3(sound, MidiIn, fileName);
    GET_STR_VALUE_3(sound, MidiIn, desc);
    GET_ENUM_VALUE_3(sound, MidiOut, type, MidiTypePair);
    GET_STR_VALUE_3(sound, MidiOut, name);
    GET_STR_VALUE_3(sound, MidiOut, fileName);
    GET_STR_VALUE_3(sound, MidiOut, desc);
    GET_ENUM_VALUE_3(sound, MidiOut, mt32ToGm, BoolPair);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, volume);
    GET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, enable, BoolPair);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, pan);
    GET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, volume);
    
    GET_ENUM_VALUE_2(joystick, POV0isAxes, BoolPair);
    
    GET_STR_VALUE_2(joy1, type);
    properties->joy1.typeId = joystickPortNameToType(0, properties->joy1.type, 0);
    GET_ENUM_VALUE_2(joy1, autofire, OnOffPair);
    
    GET_STR_VALUE_2(joy2, type);
    properties->joy2.typeId = joystickPortNameToType(1, properties->joy2.type, 0);
    GET_ENUM_VALUE_2(joy2, autofire, OnOffPair);
    
    GET_STR_VALUE_2(keyboard, configFile);

    GET_ENUM_VALUE_3(ports, Lpt, type, PrinterTypePair);
    GET_ENUM_VALUE_3(ports, Lpt, emulation, PrinterEmulationPair);
    GET_STR_VALUE_3(ports, Lpt, name);
    GET_STR_VALUE_3(ports, Lpt, fileName);
    GET_STR_VALUE_3(ports, Lpt, portName);
    GET_ENUM_VALUE_3(ports, Com, type, ComTypePair);
    GET_STR_VALUE_3(ports, Com, name);
    GET_STR_VALUE_3(ports, Com, fileName);
    GET_STR_VALUE_3(ports, Com, portName);

    GET_INT_VALUE_3(ports, Eth, ethIndex);
    GET_INT_VALUE_3(ports, Eth, disabled);
    GET_STR_VALUE_3(ports, Eth, macAddress);
    
    
    GET_INT_VALUE_2(cartridge, defaultType);

    GET_ENUM_VALUE_2(diskdrive, cdromMethod, CdromDrvPair);
    GET_INT_VALUE_2(diskdrive, cdromDrive);
    
    GET_INT_VALUE_2(cassette, showCustomFiles);
    GET_ENUM_VALUE_2(cassette, readOnly, BoolPair);
    GET_ENUM_VALUE_2(cassette, rewindAfterInsert, BoolPair);
    
    GET_ENUM_VALUE_2(nowind, enableDos2, BoolPair);    
    GET_ENUM_VALUE_2(nowind, enableOtherDiskRoms, BoolPair);    
    GET_ENUM_VALUE_2(nowind, enablePhantomDrives, BoolPair);    
    GET_ENUM_VALUE_2(nowind, ignoreBootFlag, BoolPair);   
    GET_INT_VALUE_2(nowind,  partitionNumber);

    iniFileClose();
    
#ifndef NO_FILE_HISTORY
    iniFileOpen(histFilename);
    
    GET_STR_VALUE_2(cartridge, defDir);
    GET_INT_VALUE_2(cartridge, autoReset);
    GET_INT_VALUE_2(cartridge, quickStartDrive);

    GET_STR_VALUE_2(diskdrive, defDir);
    GET_STR_VALUE_2(diskdrive, defHdDir);
    GET_INT_VALUE_2(diskdrive, autostartA);
    GET_INT_VALUE_2(diskdrive, quickStartDrive);
    
    GET_STR_VALUE_2(cassette, defDir);

    for (i = 0; i < PROP_MAX_CARTS; i++) {
        GET_STR_VALUE_2i1(media, carts, i, fileName);
        GET_STR_VALUE_2i1(media, carts, i, fileNameInZip);
        GET_STR_VALUE_2i1(media, carts, i, directory);
        GET_INT_VALUE_2i1(media, carts, i, extensionFilter);
        GET_INT_VALUE_2i1(media, carts, i, type);
    }
    
    for (i = 0; i < PROP_MAX_DISKS; i++) {
        GET_STR_VALUE_2i1(media, disks, i, fileName);
        GET_STR_VALUE_2i1(media, disks, i, fileNameInZip);
        GET_STR_VALUE_2i1(media, disks, i, directory);
        GET_INT_VALUE_2i1(media, disks, i, extensionFilter);
        GET_INT_VALUE_2i1(media, disks, i, type);
    }
    
    for (i = 0; i < PROP_MAX_TAPES; i++) {
        GET_STR_VALUE_2i1(media, tapes, i, fileName);
        GET_STR_VALUE_2i1(media, tapes, i, fileNameInZip);
        GET_STR_VALUE_2i1(media, tapes, i, directory);
        GET_INT_VALUE_2i1(media, tapes, i, extensionFilter);
        GET_INT_VALUE_2i1(media, tapes, i, type);
    }
    
    for (i = 0; i < MAX_HISTORY; i++) {
        GET_STR_VALUE_2i(filehistory, cartridge[0], i);
        GET_INT_VALUE_2i(filehistory, cartridgeType[0], i);
        GET_STR_VALUE_2i(filehistory, cartridge[1], i);
        GET_INT_VALUE_2i(filehistory, cartridgeType[1], i);
        GET_STR_VALUE_2i(filehistory, diskdrive[0], i);
        GET_STR_VALUE_2i(filehistory, diskdrive[1], i);
        GET_STR_VALUE_2i(filehistory, cassette[0], i);
    }

    GET_STR_VALUE_2(filehistory, quicksave);
    GET_STR_VALUE_2(filehistory, videocap);
    GET_INT_VALUE_2(filehistory, count);

    for (i = 0; i < DLG_MAX_ID; i++) {
        GET_INT_VALUE_2i1(settings, windowPos, i, left);
        GET_INT_VALUE_2i1(settings, windowPos, i, top);
        GET_INT_VALUE_2i1(settings, windowPos, i, width);
        GET_INT_VALUE_2i1(settings, windowPos, i, height);
    }
    
    iniFileClose();
#endif
}

void propSave(Properties* properties) 
{
    int i;
    
    iniFileOpen(settFilename);

    strcpy(properties->settings.language, langToName(properties->language, 0));
    SET_STR_VALUE_2(settings, language);
    
    SET_ENUM_VALUE_2(settings, disableScreensaver, YesNoPair);    
    SET_ENUM_VALUE_2(settings, showStatePreview, YesNoPair);
    SET_ENUM_VALUE_2(settings, usePngScreenshots, YesNoPair);
    SET_ENUM_VALUE_2(settings, portable, YesNoPair);
    if (appConfigGetString("singletheme", NULL) == NULL) {
        SET_STR_VALUE_2(settings, themeName);
    }

    SET_ENUM_VALUE_2(emulation, ejectMediaOnExit, YesNoPair);
    SET_ENUM_VALUE_2(emulation, registerFileTypes, YesNoPair);
    SET_ENUM_VALUE_2(emulation, disableWinKeys, YesNoPair);
    SET_STR_VALUE_2(emulation, statsDefDir);
    if (appConfigGetString("singlemachine", NULL) == NULL) {
        SET_STR_VALUE_2(emulation, machineName);
    }
    SET_STR_VALUE_2(emulation, shortcutProfile);
    SET_INT_VALUE_2(emulation, speed);
    SET_ENUM_VALUE_2(emulation, syncMethod, EmuSyncPair);
    SET_ENUM_VALUE_2(emulation, vdpSyncMode, VdpSyncPair);
    SET_ENUM_VALUE_2(emulation, enableFdcTiming, YesNoPair);
    SET_ENUM_VALUE_2(emulation, frontSwitch, OnOffPair);
    SET_ENUM_VALUE_2(emulation, pauseSwitch, OnOffPair);
    SET_ENUM_VALUE_2(emulation, audioSwitch, OnOffPair);
    SET_ENUM_VALUE_2(emulation, priorityBoost, YesNoPair);
    SET_ENUM_VALUE_2(emulation, reverseEnable, BoolPair);
    SET_INT_VALUE_2(emulation, reverseMaxTime);
    
    SET_ENUM_VALUE_2(video, monitorColor, MonitorColorPair);
    SET_ENUM_VALUE_2(video, monitorType, MonitorTypePair);
    SET_INT_VALUE_2(video, contrast);
    SET_INT_VALUE_2(video, brightness);
    SET_INT_VALUE_2(video, saturation);
    SET_INT_VALUE_2(video, gamma);
    SET_ENUM_VALUE_2(video, scanlinesEnable, YesNoPair);
    SET_INT_VALUE_2(video, scanlinesPct);
    SET_ENUM_VALUE_2(video, colorSaturationEnable, YesNoPair);
    SET_INT_VALUE_2(video, colorSaturationWidth);
    SET_ENUM_VALUE_2(video, deInterlace, OnOffPair);
    SET_ENUM_VALUE_2(video, blendFrames, YesNoPair);
    SET_ENUM_VALUE_2(video, detectActiveMonitor, YesNoPair);
    SET_ENUM_VALUE_2(video, horizontalStretch, YesNoPair);
    SET_ENUM_VALUE_2(video, verticalStretch, YesNoPair);
    SET_INT_VALUE_2(video, frameSkip);
    if (properties->video.windowSizeChanged) {
    	SET_ENUM_VALUE_2(video, windowSize, WindowSizePair);
    }
    else {
    	int temp=properties->video.windowSize;
    	properties->video.windowSize=properties->video.windowSizeInitial;
    	SET_ENUM_VALUE_2(video, windowSize, WindowSizePair);
    	properties->video.windowSize=temp;
    }
    SET_INT_VALUE_2(video, windowX);
    SET_INT_VALUE_2(video, windowY);
    SET_INT_VALUE_3(video, fullscreen, width);
    SET_INT_VALUE_3(video, fullscreen, height);
    SET_INT_VALUE_3(video, fullscreen, bitDepth);
    SET_ENUM_VALUE_2(video, maximizeIsFullscreen, YesNoPair);
    SET_ENUM_VALUE_2(video, driver, VideoDriverPair);
    SET_INT_VALUE_2(video, captureFps);
    
    SET_INT_VALUE_2(video, captureSize);
    
    SET_INT_VALUE_2(videoIn, disabled);
    SET_INT_VALUE_2(videoIn, inputIndex);
    SET_STR_VALUE_2(videoIn, inputName);

    SET_ENUM_VALUE_2(sound, driver, SoundDriverPair);
    SET_INT_VALUE_2(sound, bufSize);
    SET_ENUM_VALUE_2(sound, stabilizeDSoundTiming, YesNoPair);
    SET_ENUM_VALUE_2(sound, stereo, YesNoPair);
    SET_INT_VALUE_2(sound, masterVolume);
    SET_ENUM_VALUE_2(sound, masterEnable, YesNoPair);
    
    SET_ENUM_VALUE_3(sound, chip, enableYM2413, YesNoPair);
    SET_ENUM_VALUE_3(sound, chip, enableY8950, YesNoPair);
    SET_ENUM_VALUE_3(sound, chip, enableMoonsound, YesNoPair);
    SET_INT_VALUE_3(sound, chip, moonsoundSRAMSize);
//    SET_INT_VALUE_3(sound, chip, ym2413Oversampling);
//    SET_INT_VALUE_3(sound, chip, y8950Oversampling);
//    SET_INT_VALUE_3(sound, chip, moonsoundOversampling);
    SET_ENUM_VALUE_3(sound, YkIn, type, MidiTypePair);
    SET_STR_VALUE_3(sound, YkIn, name);
//    SET_STR_VALUE_3(sound, YkIn, fileName);
//    SET_STR_VALUE_3(sound, YkIn, desc);
    SET_INT_VALUE_3(sound, YkIn, channel);
    SET_ENUM_VALUE_3(sound, MidiIn, type, MidiTypePair);
    SET_STR_VALUE_3(sound, MidiIn, name);
//    SET_STR_VALUE_3(sound, MidiIn, fileName);
//    SET_STR_VALUE_3(sound, MidiIn, desc);
    SET_ENUM_VALUE_3(sound, MidiOut, type, MidiTypePair);
    SET_STR_VALUE_3(sound, MidiOut, name);
//    SET_STR_VALUE_3(sound, MidiOut, fileName);
//    SET_STR_VALUE_3(sound, MidiOut, desc);
    SET_ENUM_VALUE_3(sound, MidiOut, mt32ToGm, YesNoPair);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PSG, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_SCC, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXMUSIC, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MSXAUDIO, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_KEYBOARD, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MOONSOUND, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_YAMAHA_SFG, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_PCM, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_IO, volume);
    SET_ENUM_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, enable, YesNoPair);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, pan);
    SET_INT_VALUE_2s1(sound, mixerChannel, MIXER_CHANNEL_MIDI, volume);
    
    SET_ENUM_VALUE_2(joystick, POV0isAxes, YesNoPair);
    
    strcpy(properties->joy1.type, joystickPortTypeToName(0, 0));
    SET_STR_VALUE_2(joy1, type);
    SET_ENUM_VALUE_2(joy1, autofire, OnOffPair);
    
    strcpy(properties->joy2.type, joystickPortTypeToName(1, 0));
    SET_STR_VALUE_2(joy2, type);
    SET_ENUM_VALUE_2(joy2, autofire, OnOffPair);
    
    SET_STR_VALUE_2(keyboard, configFile);
    
    SET_ENUM_VALUE_3(ports, Lpt, type, PrinterTypePair);
    SET_ENUM_VALUE_3(ports, Lpt, emulation, PrinterEmulationPair);
    SET_STR_VALUE_3(ports, Lpt, name);
    SET_STR_VALUE_3(ports, Lpt, fileName);
    SET_STR_VALUE_3(ports, Lpt, portName);
    SET_ENUM_VALUE_3(ports, Com, type, ComTypePair);
    SET_STR_VALUE_3(ports, Com, name);
    SET_STR_VALUE_3(ports, Com, fileName);
    SET_STR_VALUE_3(ports, Com, portName);

    SET_INT_VALUE_3(ports, Eth, ethIndex);
    SET_INT_VALUE_3(ports, Eth, disabled);
    SET_STR_VALUE_3(ports, Eth, macAddress);
    
    SET_INT_VALUE_2(cartridge, defaultType);

    SET_ENUM_VALUE_2(diskdrive, cdromMethod, CdromDrvPair);
    SET_INT_VALUE_2(diskdrive, cdromDrive);
    
    SET_INT_VALUE_2(cassette, showCustomFiles);
    SET_ENUM_VALUE_2(cassette, readOnly, YesNoPair);
    SET_ENUM_VALUE_2(cassette, rewindAfterInsert, YesNoPair);

    SET_ENUM_VALUE_2(nowind, enableDos2, YesNoPair);    
    SET_ENUM_VALUE_2(nowind, enableOtherDiskRoms, BoolPair);    
    SET_ENUM_VALUE_2(nowind, enablePhantomDrives, BoolPair);    
    SET_ENUM_VALUE_2(nowind, ignoreBootFlag, BoolPair);   
    SET_INT_VALUE_2(nowind,  partitionNumber);

    iniFileClose();

#ifndef NO_FILE_HISTORY
    iniFileOpen(histFilename);
    
    SET_STR_VALUE_2(cartridge, defDir);
    SET_INT_VALUE_2(cartridge, autoReset);
    SET_INT_VALUE_2(cartridge, quickStartDrive);

    SET_STR_VALUE_2(diskdrive, defDir);
    SET_STR_VALUE_2(diskdrive, defHdDir);
    SET_INT_VALUE_2(diskdrive, autostartA);
    SET_INT_VALUE_2(diskdrive, quickStartDrive);
    
    SET_STR_VALUE_2(cassette, defDir);

    for (i = 0; i < PROP_MAX_CARTS; i++) {
        SET_STR_VALUE_2i1(media, carts, i, fileName);
        SET_STR_VALUE_2i1(media, carts, i, fileNameInZip);
        SET_STR_VALUE_2i1(media, carts, i, directory);
        SET_INT_VALUE_2i1(media, carts, i, extensionFilter);
        SET_INT_VALUE_2i1(media, carts, i, type);
    }
    
    for (i = 0; i < PROP_MAX_DISKS; i++) {
        SET_STR_VALUE_2i1(media, disks, i, fileName);
        SET_STR_VALUE_2i1(media, disks, i, fileNameInZip);
        SET_STR_VALUE_2i1(media, disks, i, directory);
        SET_INT_VALUE_2i1(media, disks, i, extensionFilter);
        SET_INT_VALUE_2i1(media, disks, i, type);
    }
    
    for (i = 0; i < PROP_MAX_TAPES; i++) {
        SET_STR_VALUE_2i1(media, tapes, i, fileName);
        SET_STR_VALUE_2i1(media, tapes, i, fileNameInZip);
        SET_STR_VALUE_2i1(media, tapes, i, directory);
        SET_INT_VALUE_2i1(media, tapes, i, extensionFilter);
        SET_INT_VALUE_2i1(media, tapes, i, type);
    }
    
    for (i = 0; i < MAX_HISTORY; i++) {
        SET_STR_VALUE_2i(filehistory, cartridge[0], i);
        SET_INT_VALUE_2i(filehistory, cartridgeType[0], i);
        SET_STR_VALUE_2i(filehistory, cartridge[1], i);
        SET_INT_VALUE_2i(filehistory, cartridgeType[1], i);
        SET_STR_VALUE_2i(filehistory, diskdrive[0], i);
        SET_STR_VALUE_2i(filehistory, diskdrive[1], i);
        SET_STR_VALUE_2i(filehistory, cassette[0], i);
    }

    SET_STR_VALUE_2(filehistory, quicksave);
    SET_STR_VALUE_2(filehistory, videocap);
    SET_INT_VALUE_2(filehistory, count);

    for (i = 0; i < DLG_MAX_ID; i++) {
        SET_INT_VALUE_2i1(settings, windowPos, i, left);
        SET_INT_VALUE_2i1(settings, windowPos, i, top);
        SET_INT_VALUE_2i1(settings, windowPos, i, width);
        SET_INT_VALUE_2i1(settings, windowPos, i, height);
    }
    
    iniFileClose();
#endif
}

static Properties* globalProperties = NULL;

Properties* propGetGlobalProperties()
{
    return globalProperties;
}

void propertiesSetDirectory(const char* defDir, const char* altDir)
{
    FILE* f;

    sprintf(settFilename, "bluemsx.ini", defDir);
    f = fopen(settFilename, "r");
    if (f != NULL) {
        fclose(f);
    }
    else {
        sprintf(settFilename, "%s/bluemsx.ini", altDir);
    }

    sprintf(histFilename, "bluemsx_history.ini", defDir);
    f = fopen(histFilename, "r");
    if (f != NULL) {
        fclose(f);
    }
    else {
        sprintf(histFilename, "%s/bluemsx_history.ini", altDir);
    }
}


Properties* propCreate(int useDefault, int langType, PropKeyboardLanguage kbdLang, int syncMode, const char* themeName) 
{
    Properties* properties;

    properties = malloc(sizeof(Properties));

    if (globalProperties == NULL) {
        globalProperties = properties;
    }

    propInitDefaults(properties, langType, kbdLang, syncMode, themeName);

    if (!useDefault) {
        propLoad(properties);
    }
#ifndef WII
    // Verify machine name
    {
        char** machineNames = machineGetAvailable(1);
        
        while (*machineNames != NULL) {
            if (0 == strcmp(*machineNames, properties->emulation.machineName)) {
                break;
            }
            machineNames++;
        }

        if (*machineNames == NULL) {
            char** machineNames = machineGetAvailable(1);
            int foundMSX2 = 0;

            if (*machineNames != NULL) {
                strcpy(properties->emulation.machineName, *machineNames);
            }

            while (*machineNames != NULL) {
                if (0 == strcmp(*machineNames, "MSX2")) {
                    strcpy(properties->emulation.machineName, *machineNames);
                    foundMSX2 = 1;
                }
                if (!foundMSX2 && 0 == strncmp(*machineNames, "MSX2", 4)) {
                    strcpy(properties->emulation.machineName, *machineNames);
                    foundMSX2 = 1;
                }
                machineNames++;
            }
        }
    }
#endif
    return properties;
}


void propDestroy(Properties* properties) {
    propSave(properties);

    free(properties);
}

 
