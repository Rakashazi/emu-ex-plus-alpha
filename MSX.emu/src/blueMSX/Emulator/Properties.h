/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Properties.h,v $
**
** $Revision: 1.80 $
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
#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "AudioMixer.h"
#include "VideoRender.h"
#include "MediaDb.h"

#define PROP_MAX_DISKS  34
#define PROP_MAX_CARTS  2
#define PROP_MAX_TAPES  1

#define PROP_MAXPATH 512

#define CARTNAME_SNATCHER    "The Snatcher Cartridge"
#define CARTNAME_SDSNATCHER  "SD-Snatcher Cartridge"
#define CARTNAME_SCCMIRRORED "SCC Mirrored Cartridge"
#define CARTNAME_SCCEXPANDED "SCC Expanded Cartridge"
#define CARTNAME_SCC         "SCC Cartridge"
#define CARTNAME_SCCPLUS     "SCC-I Cartridge"
#define CARTNAME_FMPAC       "FM-PAC Cartridge"
#define CARTNAME_PAC         "PAC Cartridge"
#define CARTNAME_SONYHBI55   "Sony HBI-55"
#define CARTNAME_EXTRAM16KB  "16kB External RAM"
#define CARTNAME_EXTRAM32KB  "32kB External RAM"
#define CARTNAME_EXTRAM48KB  "48kB External RAM"
#define CARTNAME_EXTRAM64KB  "64kB External RAM"
#define CARTNAME_EXTRAM512KB "512kB External RAM"
#define CARTNAME_EXTRAM1MB   "1MB External RAM"
#define CARTNAME_EXTRAM2MB   "2MB External RAM"
#define CARTNAME_EXTRAM4MB   "4MB External RAM"
#define CARTNAME_MEGARAM128  "128kB MegaRAM"
#define CARTNAME_MEGARAM256  "256kB MegaRAM"
#define CARTNAME_MEGARAM512  "512kB MegaRAM"
#define CARTNAME_MEGARAM768  "768kB MegaRAM"
#define CARTNAME_MEGARAM2M   "2MB MegaRAM"
#define CARTNAME_GAMEREADER  "Game Reader"
#define CARTNAME_SUNRISEIDE  "Sunrise IDE"
#define CARTNAME_BEERIDE     "Beer IDE"
#define CARTNAME_GIDE        "GIDE"
#define CARTNAME_GOUDASCSI   "Gouda SCSI"
#define CARTNAME_NMS1210     "NMS1210"
#define CARTNAME_JOYREXPSG   "Joyrex PSG"
#define CARTNAME_MEGASCSI128 "128kB MEGA-SCSI"
#define CARTNAME_MEGASCSI256 "256kB MEGA-SCSI"
#define CARTNAME_MEGASCSI512 "512kB MEGA-SCSI"
#define CARTNAME_MEGASCSI1MB "1MB MEGA-SCSI"
#define CARTNAME_ESERAM128   "128kB Ese-RAM"
#define CARTNAME_ESERAM256   "256kB Ese-RAM"
#define CARTNAME_ESERAM512   "512kB Ese-RAM"
#define CARTNAME_ESERAM1MB   "1MB Ese-RAM"
#define CARTNAME_NOWINDDOS1  "Nowind MSXDOS1"
#define CARTNAME_NOWINDDOS2  "Nowind MSXDOS2"
#define CARTNAME_MEGAFLSHSCC "MegaFlashRomScc"
#define CARTNAME_WAVESCSI128 "128kB WAVE-SCSI"
#define CARTNAME_WAVESCSI256 "256kB WAVE-SCSI"
#define CARTNAME_WAVESCSI512 "512kB WAVE-SCSI"
#define CARTNAME_WAVESCSI1MB "1MB WAVE-SCSI"
#define CARTNAME_ESESCC128   "128kB Ese-SCC"
#define CARTNAME_ESESCC256   "256kB Ese-SCC"
#define CARTNAME_ESESCC512   "512kB Ese-SCC"

typedef enum { 
    PROP_EMULATION = 0, 
    PROP_VIDEO, 
    PROP_SOUND, 
    PROP_PERFORMANCE, 
    PROP_SETTINGS, 
    PROP_DISK,
    PROP_APEARANCE, 
    PROP_PORTS 
} PropPage;

typedef enum { 
    P_KBD_EUROPEAN = 0, 
    P_KBD_RUSSIAN, 
    P_KBD_JAPANESE, 
    P_KBD_KOREAN 
} PropKeyboardLanguage;

enum { 
    P_LPT_NONE = 0, 
    P_LPT_SIMPL, 
    P_LPT_FILE, 
    P_LPT_HOST 
};

enum { 
    P_COM_NONE = 0, 
    P_COM_FILE, 
    P_COM_HOST 
};

enum { 
    P_MIDI_NONE = 0, 
    P_MIDI_FILE, 
    P_MIDI_HOST 
};

enum { 
    P_LPT_RAW, 
    P_LPT_MSXPRN, 
    P_LPT_SVIPRN, 
    P_LPT_EPSONFX80 
};

enum { 
    P_EMU_SYNCIGNORE = -1, 
    P_EMU_SYNCNONE = 0, 
    P_EMU_SYNCAUTO, 
    P_EMU_SYNCFRAMES, 
    P_EMU_SYNCTOVBLANK, 
    P_EMU_SYNCTOVBLANKASYNC,
};

enum { 
    P_VDP_SYNCAUTO = 0, 
    P_VDP_SYNC50HZ, 
    P_VDP_SYNC60HZ 
};

enum { 
    P_VIDEO_COLOR = 0, 
    P_VIDEO_BW, 
    P_VIDEO_GREEN, 
    P_VIDEO_AMBER, 
    P_VIDEO_MONCOUNT 
};

enum { 
    P_VIDEO_PALNONE = 0, 
    P_VIDEO_PALMON, 
    P_VIDEO_PALYC, 
    P_VIDEO_PALNYC, 
    P_VIDEO_PALCOMP, 
    P_VIDEO_PALNCOMP, 
    P_VIDEO_PALSCALE2X, 
    P_VIDEO_PALHQ2X, 
    P_VIDEO_PALCOUNT 
};

enum { 
    P_VIDEO_SIZEX1 = 0, 
    P_VIDEO_SIZEX2, 
    P_VIDEO_SIZEFULLSCREEN 
};

enum { 
    P_VIDEO_FREQ_AUTO, 
    P_VIDEO_FREQ_50HZ, 
    P_VIDEO_FREQ_60HZ 
};

enum { 
    P_SOUND_DRVNONE = 0, 
    P_SOUND_DRVWMM, 
    P_SOUND_DRVDIRECTX 
};

enum { 
    P_VIDEO_DRVDIRECTX_VIDEO = 0, 
    P_VIDEO_DRVDIRECTX, 
    P_VIDEO_DRVGDI 
};

enum { 
    P_VIDEO_DRVSDLGL = 0, 
    P_VIDEO_DRVSDLGL_NODIRT,
    P_VIDEO_DRVSDL
};

enum {
    P_CDROM_DRVNONE = 0,
    P_CDROM_DRVIOCTL,
    P_CDROM_DRVASPI
};

#define MAX_HISTORY 30

typedef struct {
    char statsDefDir[PROP_MAXPATH];
    char machineName[PROP_MAXPATH];
    char shortcutProfile[PROP_MAXPATH];
    int  enableFdcTiming;
    int  frontSwitch;
    int  audioSwitch;
    int  pauseSwitch;
    int  speed;
    int  ejectMediaOnExit;
    int  registerFileTypes;
    int  disableWinKeys;
    int  priorityBoost;
    int  syncMethod;
    int  vdpSyncMode;
    int  reverseEnable;
    int  reverseMaxTime;
} EmulationProperties;

typedef struct {
    int monitorColor;
    int monitorType;
    int windowSize;
    int windowSizeInitial;
    int windowSizeChanged;
    int windowX;
    int windowY;
    int driver;
    int frameSkip;
    struct {
        int width;
        int height;
        int bitDepth;
    } fullscreen;
    int maximizeIsFullscreen;
    int frequency;
    int deInterlace;
    int blendFrames;
    int horizontalStretch;
    int verticalStretch;
    int contrast;
    int brightness;
    int saturation;
    int scanlinesEnable;
    int scanlinesPct;
    int colorSaturationEnable;
    int colorSaturationWidth;
    int gamma;
    int detectActiveMonitor;
    int captureFps;
    int captureSize;
} VideoProperties;

typedef struct {
    int disabled;
    int inputIndex;
    char inputName[256];
} VideoInProperties;

typedef struct {
    int enable;
    int volume;
    int pan;
} MixerChannel;

typedef struct {
    int enableY8950;
    int enableYM2413;
    int enableMoonsound;
    int moonsoundSRAMSize;
    int ym2413Oversampling;
    int y8950Oversampling;
    int moonsoundOversampling;
} SoundChip;

typedef struct {
    int  driver;
    int  bufSize;
    int  stabilizeDSoundTiming;
    SoundChip chip;
    int  stereo;
    int  masterVolume;
    int  masterEnable;
    MixerChannel mixerChannel[MIXER_CHANNEL_TYPE_COUNT];
    int  log[PROP_MAXPATH];
    struct {
        int  type;
        char name[256];
        char desc[256];
        char fileName[PROP_MAXPATH];
        int  channel;
    } YkIn;
    struct {
        int  type;
        char name[256];
        char desc[256];
        char fileName[PROP_MAXPATH];
    } MidiIn;
    struct {
        int  type;
        char name[256];
        char desc[256];
        char fileName[PROP_MAXPATH];
        int  mt32ToGm;
    } MidiOut;
} SoundProperties;

typedef struct {
	int POV0isAxes;
} JoystickGeneric;

typedef struct {
    char type[64];
    int  typeId;
    int  autofire;
}  JoystickProperties;

typedef struct {
    char configFile[PROP_MAXPATH];
} KeyboardProperties;

typedef struct {
    char fileName[PROP_MAXPATH];
    char fileNameInZip[PROP_MAXPATH];
    char directory[PROP_MAXPATH];
    int  extensionFilter;
    int  type;
} FileProperties;

typedef struct {
    FileProperties carts[PROP_MAX_CARTS];
    FileProperties disks[PROP_MAX_DISKS];
    FileProperties tapes[PROP_MAX_TAPES];
} Media;

typedef struct {
    int enableDos2;
    int enablePhantomDrives;
    int enableOtherDiskRoms;
    int partitionNumber;
    int ignoreBootFlag;
} NoWindProperties;

typedef struct {
    RomType defaultType;
    char    defDir[PROP_MAXPATH];
    int     autoReset;
    int     quickStartDrive;
} CartridgeProperties;

typedef struct {
    char defDir[PROP_MAXPATH];
    char defHdDir[PROP_MAXPATH];
    int  autostartA;
    int  quickStartDrive;
    int  cdromMethod;
    int  cdromDrive;
} DiskdriveProperties;

typedef struct {
    char defDir[PROP_MAXPATH];
    int showCustomFiles;
    int readOnly;
    int rewindAfterInsert;
} CassetteProperties;

typedef struct {
#ifndef NO_FILE_HISTORY
    int     count;
    char    cartridge[2][MAX_HISTORY][PROP_MAXPATH];
    RomType cartridgeType[2][MAX_HISTORY];
    char    diskdrive[2][MAX_HISTORY][PROP_MAXPATH];
    char    cassette[1][MAX_HISTORY][PROP_MAXPATH];
#endif
    char    quicksave[PROP_MAXPATH];
    char    videocap[PROP_MAXPATH];
} FileHistory;

typedef struct {
    struct {
        int  type;
        int  emulation;
        char name[256];
        char portName[PROP_MAXPATH];
        char fileName[PROP_MAXPATH];
    } Lpt;
    struct {
        int  type;
        char name[256];
        char portName[PROP_MAXPATH];
        char fileName[PROP_MAXPATH];
    } Com;
    struct {
        int disabled;
        int ethIndex;
        char macAddress[64];
    } Eth;
} PortProperties;

#define DLG_MAX_ID 32

typedef struct {
    char language[64];

    int portable;
    int disableScreensaver;
    int showStatePreview;
    int usePngScreenshots;
    char themeName[128];
    struct {  
        long left; 
        long top; 
        long width; 
        long height; 
    } windowPos[DLG_MAX_ID];
} Settings;

typedef struct Properties {
    EmulationProperties emulation;
    VideoProperties     video;
    VideoInProperties   videoIn;
    SoundProperties     sound;
    JoystickGeneric     joystick;
    JoystickProperties  joy1;
    JoystickProperties  joy2;
    KeyboardProperties  keyboard;
    CartridgeProperties cartridge;
    DiskdriveProperties diskdrive;
    Media               media;
    CassetteProperties  cassette;
    FileHistory         filehistory;
    PortProperties      ports;
    int                 language;
    Settings            settings;
    NoWindProperties    nowind;
} Properties;

Properties* propCreate(int useDefault, 
                       int langType, 
                       PropKeyboardLanguage kbdLang, 
                       int syncMode, 
                       const char* themeName);
void propSave(Properties* pProperties);
void propDestroy(Properties* pProperties);

void propertiesSetDirectory(const char* defDir, const char* altDir);

Properties* propGetGlobalProperties();

#endif
