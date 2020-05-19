/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/Machine.c,v $
**
** $Revision: 1.69 $
**
** $Date: 2008-11-23 20:26:12 $
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
#include "Machine.h"
#include "SaveState.h"
#include "IniFileParser.h"
#include "ArchGlob.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "ArchFile.h"
#include "MediaDb.h"
#include "TokenExtract.h"
#include "Disk.h"

#include "AppConfig.h"

#include "RomLoader.h"
#include "MSXMidi.h"
#include "ramMapper.h"
#include "ramMapperIo.h"
#include "ramNormal.h"
#include "romMapperNormal.h"
#include "romMapperKanji.h"
#include "romMapperKanji12.h"
#include "romMapperBasic.h"
#include "romMapperCasette.h"
#include "romMapperStandard.h"
#include "romMapperMsxDos2.h"
#include "romMapperKonami5.h"
#include "romMapperKonami4.h"
#include "romMapperKoei.h"
#include "romMapperHolyQuran.h"
#include "romMapperMegaRAM.h"
#include "romMapperASCII8.h"
#include "romMapperASCII16.h"
#include "romMapperDisk.h"
#include "romMapperTC8566AF.h"
#include "romMapperMicrosol.h"
#include "romMapperNationalFdc.h"
#include "romMapperPhilipsFdc.h"
#include "romMapperSvi738Fdc.h"
#include "romMapperGameMaster2.h"
#include "romMapperASCII8sram.h"
#include "romMapperASCII16sram.h"
#include "romMapperASCII16nf.h"
#include "romMapperKonami4nf.h"
#include "romMapperPlain.h"
#include "romMapperHarryFox.h"
#include "romMapperHalnote.h"
#include "romMapperMsxAudio.h"
#include "romMapperRType.h"
#include "romMapperCrossBlaim.h"
#include "romMapperKorean80.h"
#include "romMapperKorean90.h"
#include "romMapperKorean126.h"
#include "romMapperPAC.h"
#include "romMapperFMPAC.h"
#include "romMapperFMPAK.h"
#include "romMapperLodeRunner.h"
#include "romMapperSCCplus.h"
#include "romMapperPanasonic.h"
#include "romMapperNational.h"
#include "sramMapperMatsuchita.h"
#include "romMapperKonamiSynth.h"
#include "romMapperKonamiKeyboardMaster.h"
#include "romMapperKonamiWordPro.h"
#include "romMapperMajutsushi.h"
#include "sramMapperS1985.h"
#include "romMapperS1990.h"
#include "romMapperF4device.h"
#include "romMapperBunsetu.h"
#include "romMapperTurboRTimer.h"
#include "romMapperTurboRPcm.h"
#include "TurboRIO.h"
#include "romMapperSonyHBI55.h"
#include "romMapperMsxMusic.h"
#include "romMapperMsxPrn.h"
#include "romMapperMoonsound.h"
#include "romMapperGameReader.h"
#include "romMapperSvi328Prn.h"
#include "romMapperSvi328Rs232.h"
#include "romMapperSvi328Fdc.h"
#include "romMapperSvi727.h"
#include "ram1kBMirrored.h"
#include "romMapperSunriseIDE.h"
#include "romMapperBeerIDE.h"
#include "romMapperGIDE.h"
#include "romMapperSvi328RsIDE.h"
#include "romMapperMicrosolVmx80.h"
#include "romMapperNms8280VideoDa.h"
#include "romMapperSonyHBIV1.h"
#include "romMapperFmDas.h"
#include "romMapperSfg05.h"
#include "romMapperSf7000Ipl.h"
#include "romMapperPlayBall.h"
#include "romMapperObsonet.h"
#include "romMapperSg1000Castle.h"
#include "romMapperSg1000.h"
#include "romMapperSegaBasic.h"
#include "romMapperDumas.h"
#include "sramMapperMegaSCSI.h"
#include "sramMapperEseSCC.h"
#include "romMapperNoWind.h"
#include "romMapperGoudaSCSI.h"
#include "romMapperMegaFlashRomScc.h"
#include "romMapperForteII.h"
#include "romMapperA1FMModem.h"
#include "romMapperA1FM.h"
#include "romMapperDRAM.h"
#include "romMapperMatraINK.h"
#include "romMapperNettouYakyuu.h"
#include "romMapperNet.h"
#include "romMapperJoyrexPsg.h"
#include "romMapperOpcodePsg.h"
#include "romMapperArc.h"
#include "romMapperOpcodeBios.h"
#include "romMapperOpcodeMegaRam.h"
#include "romMapperOpcodeSaveRam.h"
#include "romMapperOpcodeSlotManager.h"


// PacketFileSystem.h Need to be included after all other includes
#include "PacketFileSystem.h"


#include "romExclusion.h"


int toint(char* buffer) 
{
    int i;

    if (buffer == NULL) {
        return -1;
    }

    for (i = 0; buffer[i]; i++) {
        if (!isdigit(buffer[i])) return -1;
    }

    return atoi(buffer);
}

static int readMachine(Machine* machine, const char* machineName, const char* file)
{
    static char buffer[10000];
    char* slotBuf;
    int value;
    int i = 0;

    strcpy(machine->name, machineName);

    iniFileOpen(file);

    // Read board info
    iniFileGetString("Board", "type", "none", buffer, 10000);
    if      (0 == strcmp(buffer, "MSX"))          machine->board.type = BOARD_MSX;
    else if (0 == strcmp(buffer, "MSX-S3527"))    machine->board.type = BOARD_MSX_S3527;
    else if (0 == strcmp(buffer, "MSX-S1985"))    machine->board.type = BOARD_MSX_S1985;
    else if (0 == strcmp(buffer, "MSX-T9769B"))   machine->board.type = BOARD_MSX_T9769B;
    else if (0 == strcmp(buffer, "MSX-T9769C"))   machine->board.type = BOARD_MSX_T9769C;
    else if (0 == strcmp(buffer, "MSX-ForteII"))  machine->board.type = BOARD_MSX_FORTE_II;
    else if (0 == strcmp(buffer, "SVI"))          machine->board.type = BOARD_SVI;
    else if (0 == strcmp(buffer, "ColecoVision")) machine->board.type = BOARD_COLECO;
    else if (0 == strcmp(buffer, "ColecoAdam"))   machine->board.type = BOARD_COLECOADAM;
    else if (0 == strcmp(buffer, "SG-1000"))      machine->board.type = BOARD_SG1000;
    else if (0 == strcmp(buffer, "SF-7000"))      machine->board.type = BOARD_SF7000;
    else if (0 == strcmp(buffer, "SC-3000"))      machine->board.type = BOARD_SC3000;
    else                                          machine->board.type = BOARD_MSX;

    // Read video info
    iniFileGetString("Video", "version", "none", buffer, 10000);
    if      (0 == strcmp(buffer, "V9938"))    machine->video.vdpVersion = VDP_V9938;
    else if (0 == strcmp(buffer, "V9958"))    machine->video.vdpVersion = VDP_V9958;
    else if (0 == strcmp(buffer, "TMS9929A")) machine->video.vdpVersion = VDP_TMS9929A;
    else if (0 == strcmp(buffer, "TMS99x8A")) machine->video.vdpVersion = VDP_TMS99x8A;
    else { iniFileClose(); return 0; }

    iniFileGetString("Video", "vram size", "none", buffer, 10000);
    if (0 == strcmp(buffer, "16kB")) machine->video.vramSize = 16 * 1024;
    else if (0 == strcmp(buffer, "64kB")) machine->video.vramSize = 64 * 1024;
    else if (0 == strcmp(buffer, "128kB")) machine->video.vramSize = 128 * 1024;
    else if (0 == strcmp(buffer, "192kB")) machine->video.vramSize = 192 * 1024;
    else { iniFileClose(); return 0; }

    // Read CMOS info
    iniFileGetString("CMOS", "Enable CMOS", "none", buffer, 10000);
    if (0 == strcmp(buffer, "none")) machine->cmos.enable = 1;
    else if (0 == strcmp(buffer, "0")) machine->cmos.enable = 0;
    else if (0 == strcmp(buffer, "1")) machine->cmos.enable = 1;
    else { iniFileClose(); return 0; }

    // Read CPU info
    iniFileGetString("CPU", "Z80 Frequency", "none", buffer, 10000);
    if (0 == sscanf(buffer, "%dHz", &value)) {
        value = 3579545;
    }
    machine->cpu.freqZ80 = value;

    // Read CPU info
    iniFileGetString("CPU", "R800 Frequency", "none", buffer, 10000);
    if (0 == sscanf(buffer, "%dHz", &value)) {
        value = 7159090;
    }
    machine->cpu.freqR800 = value;

    // Read FDC info
    iniFileGetString("FDC", "Count", "none", buffer, 10000);
    if (0 == strcmp(buffer, "none")) machine->fdc.count = 2;
    else if (0 == strcmp(buffer, "0")) machine->fdc.count = 0;
    else if (0 == strcmp(buffer, "1")) machine->fdc.count = 1;
    else if (0 == strcmp(buffer, "2")) machine->fdc.count = 2;
    else if (0 == strcmp(buffer, "3")) machine->fdc.count = 3;
    else if (0 == strcmp(buffer, "4")) machine->fdc.count = 4;
    else { iniFileClose(); return 0; }
    
    iniFileGetString("CMOS", "Battery Backed", "none", buffer, 10000);
    if (0 == strcmp(buffer, "none")) machine->cmos.batteryBacked = 1;
    else if (0 == strcmp(buffer, "0")) machine->cmos.batteryBacked = 0;
    else if (0 == strcmp(buffer, "1")) machine->cmos.batteryBacked = 1;
    else { iniFileClose(); return 0; }

    // Read audio info
    iniFileGetString("AUDIO", "PSG Stereo", "none", buffer, 10000);
    if (0 == strcmp(buffer, "none")) machine->audio.psgstereo = 0;
    else if (0 == strcmp(buffer, "0")) machine->audio.psgstereo = 0;
    else if (0 == strcmp(buffer, "1")) machine->audio.psgstereo = 1;
    else { iniFileClose(); return 0; }

    for (i = 0; i < sizeof(machine->audio.psgpan) / sizeof(machine->audio.psgpan[0]); i++) {
        char s[32];
        sprintf(s, "PSG Pan channel %d", i);
        iniFileGetString("AUDIO", s, "none", buffer, 10000);
        if (0 == strcmp(buffer, "none")) machine->audio.psgpan[i] = i == 0 ? 0 : i == 1 ? -1 : 1;
        else if (0 == strcmp(buffer, "left")) machine->audio.psgpan[i] = -1;
        else if (0 == strcmp(buffer, "center")) machine->audio.psgpan[i] = 0;
        else if (0 == strcmp(buffer, "right")) machine->audio.psgpan[i] = 1;
        else { iniFileClose(); return 0; }
    }

    // Read subslot info
    iniFileGetString("Subslotted Slots", "slot 0", "none", buffer, 10000);
    if (0 == strcmp(buffer, "0")) machine->slot[0].subslotted = 0;
    else if (0 == strcmp(buffer, "1")) machine->slot[0].subslotted = 1;
    else { iniFileClose(); return 0; }
    
    iniFileGetString("Subslotted Slots", "slot 1", "none", buffer, 10000);
    if (0 == strcmp(buffer, "0")) machine->slot[1].subslotted = 0;
    else if (0 == strcmp(buffer, "1")) machine->slot[1].subslotted = 1;
    else { iniFileClose(); return 0; }
    
    iniFileGetString("Subslotted Slots", "slot 2", "none", buffer, 10000);
    if (0 == strcmp(buffer, "0")) machine->slot[2].subslotted = 0;
    else if (0 == strcmp(buffer, "1")) machine->slot[2].subslotted = 1;
    else { iniFileClose(); return 0; }
    
    iniFileGetString("Subslotted Slots", "slot 3", "none", buffer, 10000);
    if (0 == strcmp(buffer, "0")) machine->slot[3].subslotted = 0;
    else if (0 == strcmp(buffer, "1")) machine->slot[3].subslotted = 1;
    else { iniFileClose(); return 0; }

    // Read external slot info
    iniFileGetString("External Slots", "slot A", "none", buffer, 10000);
    machine->cart[0].slot = toint(extractToken(buffer, 0));        
    machine->cart[0].subslot = toint(extractToken(buffer, 1));    
    if (machine->cart[0].slot < 0 || machine->cart[0].slot >= 4) { iniFileClose(); return 0; }
    if (machine->cart[0].subslot < 0 || machine->cart[0].subslot >= 4) { iniFileClose(); return 0; }  

    iniFileGetString("External Slots", "slot B", "none", buffer, 10000);
    machine->cart[1].slot = toint(extractToken(buffer, 0));        
    machine->cart[1].subslot = toint(extractToken(buffer, 1));    
    if (machine->cart[1].slot < 0 || machine->cart[1].slot >= 4) { iniFileClose(); return 0; }
    if (machine->cart[1].subslot < 0 || machine->cart[1].subslot >= 4) { iniFileClose(); return 0; }   

    // Read slots
    iniFileGetSection("Slots", buffer, 10000);

    slotBuf = buffer;
    machine->cpu.hasR800 = 0;
    machine->fdc.enabled = 0;
    for (i = 0; i < sizeof(machine->slotInfo) / sizeof(SlotInfo) && *slotBuf; i++) {
        char* arg;

        machine->slotInfo[i].slot = toint(extractToken(slotBuf, 0));    
        machine->slotInfo[i].subslot = toint(extractToken(slotBuf, 1));
        machine->slotInfo[i].startPage = toint(extractToken(slotBuf, 2));
        machine->slotInfo[i].pageCount = toint(extractToken(slotBuf, 3));
        machine->slotInfo[i].romType = toint(extractToken(slotBuf, 4));
        machine->cpu.hasR800 |= machine->slotInfo[i].romType == ROM_S1990;
        machine->fdc.enabled |= machine->slotInfo[i].romType == ROM_DISKPATCH   ||
                                machine->slotInfo[i].romType == ROM_TC8566AF    ||
                                machine->slotInfo[i].romType == ROM_TC8566AF_TR ||
                                machine->slotInfo[i].romType == ROM_MICROSOL    ||
                                machine->slotInfo[i].romType == ROM_NATIONALFDC ||
                                machine->slotInfo[i].romType == ROM_PHILIPSFDC  ||
                                machine->slotInfo[i].romType == ROM_SVI328FDC   ||
                                machine->slotInfo[i].romType == ROM_SVI738FDC;

        arg = extractToken(slotBuf, 5);
        strcpy(machine->slotInfo[i].name, arg ? arg : "");

        arg = extractToken(slotBuf, 6);
        strcpy(machine->slotInfo[i].inZipName, arg ? arg : "");

        if (machine->slotInfo[i].slot < 0 || machine->slotInfo[i].slot >= 4) { iniFileClose(); return 0; }
        if (machine->slotInfo[i].subslot < 0 || machine->slotInfo[i].subslot >= 4) { iniFileClose(); return 0; }
        if (machine->slotInfo[i].startPage < 0 || machine->slotInfo[i].startPage >= 8) { iniFileClose(); return 0; }
        if (machine->slotInfo[i].pageCount == -1) { iniFileClose(); return 0; }
        if (machine->slotInfo[i].romType < 1 || machine->slotInfo[i].romType > ROM_MAXROMID) { iniFileClose(); return 0; }

        slotBuf += strlen(slotBuf) + 1;
    }

    machine->slotInfoCount = i;
 
    iniFileClose();

    return 1;
}

void machineSave(Machine* machine)
{
    char dir[512];
    char file[512];
    char buffer[10000];
    int size = 0;
    int i;

    sprintf(dir, "Machines/%s", machine->name);
    archCreateDirectory(dir);

    sprintf(file, "Machines/%s/config.ini", machine->name);

    iniFileOpen(file);

    // Write CMOS info
    iniFileWriteString("CMOS", "Enable CMOS", machine->cmos.enable ? "1" : "0");
    iniFileWriteString("CMOS", "Battery Backed", machine->cmos.batteryBacked ? "1" : "0");

    // Write Audio info
    iniFileWriteString("AUDIO", "PSG Stereo", machine->audio.psgstereo ? "1" : "0");
    
    if (machine->audio.psgstereo) {
        for (i = 0; i < sizeof(machine->audio.psgpan) / sizeof(machine->audio.psgpan[0]); i++) {
            char s[32];
            int pan = machine->audio.psgpan[i];
            sprintf(s, "PSG Pan channel %d", i);
            iniFileWriteString("AUDIO", s,  pan < 0 ? "left" : pan > 0 ? "right" : "center");
        }
    }

    // Write FDC info
    sprintf(buffer, "%d", machine->fdc.count);
    iniFileWriteString("FDC", "Count", buffer);

    // Write CPU info
    sprintf(buffer, "%dHz", machine->cpu.freqZ80);
    iniFileWriteString("CPU", "Z80 Frequency", buffer);
    if (machine->cpu.hasR800) {
        sprintf(buffer, "%dHz", machine->cpu.freqR800);
        iniFileWriteString("CPU", "R800 Frequency", buffer);
    }

    // Write Board info
    switch (machine->board.type) {
    case BOARD_MSX:          iniFileWriteString("Board", "type", "MSX"); break;
    case BOARD_MSX_S3527:    iniFileWriteString("Board", "type", "MSX-S3527"); break;
    case BOARD_MSX_S1985:    iniFileWriteString("Board", "type", "MSX-S1985"); break;
    case BOARD_MSX_T9769B:   iniFileWriteString("Board", "type", "MSX-T9769B"); break;
    case BOARD_MSX_T9769C:   iniFileWriteString("Board", "type", "MSX-T9769C"); break;
    case BOARD_MSX_FORTE_II: iniFileWriteString("Board", "type", "MSX-ForteII"); break;
    case BOARD_SVI:          iniFileWriteString("Board", "type", "SVI"); break;
    case BOARD_COLECO:       iniFileWriteString("Board", "type", "ColecoVision"); break;
    case BOARD_COLECOADAM:   iniFileWriteString("Board", "type", "ColecoAdam"); break;
    case BOARD_SG1000:       iniFileWriteString("Board", "type", "SG-1000"); break;
    case BOARD_SF7000:       iniFileWriteString("Board", "type", "SF-7000"); break;
    case BOARD_SC3000:       iniFileWriteString("Board", "type", "SC-3000"); break;
    }

    // Write video info
    switch (machine->video.vdpVersion) {
    case VDP_V9958:     iniFileWriteString("Video", "version", "V9958"); break;
    case VDP_V9938:     iniFileWriteString("Video", "version", "V9938"); break;
    case VDP_TMS9929A:  iniFileWriteString("Video", "version", "TMS9929A"); break;
    case VDP_TMS99x8A:  iniFileWriteString("Video", "version", "TMS99x8A"); break;
    }

    sprintf(buffer, "%dkB", machine->video.vramSize / 0x400);
    iniFileWriteString("Video", "vram size", buffer);

    // Write subslot info
    iniFileWriteString("Subslotted Slots", "slot 0", machine->slot[0].subslotted ? "1" : "0");
    iniFileWriteString("Subslotted Slots", "slot 1", machine->slot[1].subslotted ? "1" : "0");
    iniFileWriteString("Subslotted Slots", "slot 2", machine->slot[2].subslotted ? "1" : "0");
    iniFileWriteString("Subslotted Slots", "slot 3", machine->slot[3].subslotted ? "1" : "0");

    // Write external slot info
    sprintf(buffer, "%d %d", machine->cart[0].slot, machine->cart[0].subslot);
    iniFileWriteString("External Slots", "slot A", buffer);
    sprintf(buffer, "%d %d", machine->cart[1].slot, machine->cart[1].subslot);
    iniFileWriteString("External Slots", "slot B", buffer);

    // Write slots
    for (i = 0; i < machine->slotInfoCount; i++) {
        size += sprintf(buffer + size, "%d %d %d %d %d \"%s\" \"%s\"",
                        machine->slotInfo[i].slot,
                        machine->slotInfo[i].subslot,
                        machine->slotInfo[i].startPage,
                        machine->slotInfo[i].pageCount,
                        machine->slotInfo[i].romType,
                        machine->slotInfo[i].name,
                        machine->slotInfo[i].inZipName);
        buffer[size++] = 0;
    }

    buffer[size++] = 0;
    buffer[size++] = 0;

//    iniFileWriteString("Slots", NULL, NULL);
    iniFileWriteSection("Slots", buffer);

    iniFileClose();
}

Machine* machineCreate(const char* machineName)
{
    char fileName[512];
    Machine* machine;
    int success;

    machine = malloc(sizeof(Machine));

    sprintf(fileName, "Machines/%s/config.ini", machineName);
    success = readMachine(machine, machineName, fileName);
    if (!success) {
        free(machine);
        return NULL;
    }

    machineUpdate(machine);

    return machine;
}

void machineDestroy(Machine* machine)
{
    free(machine);
}


int machineIsValid(const char* machineName, int checkRoms)
{
    Machine* machine = machineCreate(machineName);
    int i;
    int success = 1;

    if (machine == NULL) {
        return 0;
    }

    if (!checkRoms) {
        return 1;
    }

    for (i = 0; i < machine->slotInfoCount; i++) {
        if (strlen(machine->slotInfo[i].name) || 
            strlen(machine->slotInfo[i].inZipName))
        {        
            FILE* file = fopen(machine->slotInfo[i].name, "r");
            if (file == NULL) {
                if (success) {
//                    printf("\n%s: Cant find rom:\n", machineName);
                }
//                printf("     %s\n", machine->slotInfo[i].name);
                success = 0;
                continue;
            }
            fclose(file);
        }
    }

    free(machine);

    return success;
}

char** machineGetAvailable(int checkRoms)
{
    const char* machineName = appConfigGetString("singlemachine", NULL);

    if (machineName != NULL) {
        char filename[128];
        static char* machineNames[256];
        static char  names[256][64];
        int index = 0;

        FILE* file;

        sprintf(filename, "Machines/%s/config.ini", machineName);
        file = fopen(filename, "rb");
        if (file != NULL) {
            if (machineIsValid(machineName, checkRoms)) {
                strcpy(names[index], machineName);
                machineNames[index] = names[index];
                index++;
            }
            fclose(file);
        }
        
        machineNames[index] = NULL;

        return machineNames;
    }
    else {
        static char* machineNames[256];
        static char  names[256][64];
        ArchGlob* glob = archGlob("Machines/*", ARCH_GLOB_DIRS);
        int index = 0;
        int i;

        if (glob == NULL) {
            machineNames[0] = NULL;
            return machineNames;
        }

        for (i = 0; i < glob->count; i++) {
            char fileName[512];
            FILE* file;
		    sprintf(fileName, "%s/config.ini", glob->pathVector[i]);
            file = fopen(fileName, "rb");
            if (file != NULL) {
                const char* name = strrchr(glob->pathVector[i], '/');
                if (name == NULL) {
                    name = strrchr(glob->pathVector[i], '\\');
                }
                if (name == NULL) {
                    name = glob->pathVector[i] - 1;
                }
                name++;
                if (machineIsValid(name, checkRoms)) {
                    strcpy(names[index], name);
                    machineNames[index] = names[index];
                    index++;
                }
                fclose(file);
            }
        }

        archGlobFree(glob);
        
        machineNames[index] = NULL;

        return machineNames;
    }
}


void machineUpdate(Machine* machine)
{
    int entry;
    int i;

    for (entry = 0; entry < machine->slotInfoCount; entry++) {
        machine->slotInfo[entry].error = 0;
        if (machine->slotInfo[entry].subslot && !machine->slot[machine->slotInfo[entry].slot].subslotted) {
            machine->slotInfo[entry].error = 1;
        }
        if (machine->slotInfo[entry].pageCount > 0) {
            for (i = 0; i < entry; i++) {
                if (machine->slotInfo[i].slot    == machine->slotInfo[entry].slot &&
                    machine->slotInfo[i].subslot == machine->slotInfo[entry].subslot &&
                    machine->slotInfo[i].pageCount > 0)
                {
                    int tstStart = machine->slotInfo[entry].startPage;
                    int tstEnd   = tstStart + machine->slotInfo[entry].pageCount;
                    int start    = machine->slotInfo[i].startPage;
                    int end      = start + machine->slotInfo[i].pageCount;
                    if (tstStart >= start && tstStart < end) {
                        machine->slotInfo[entry].error = 1;
                    }
                    if (tstEnd > start && tstEnd <= end) {
                        machine->slotInfo[entry].error = 1;
                    }
                }
            }
        }
    }

    for (i = 0; i < machine->slotInfoCount; i++) {
        for (entry = 0; entry < machine->slotInfoCount - 1; entry++) {
            SlotInfo* si1 = &machine->slotInfo[entry];
            SlotInfo* si2 = &machine->slotInfo[entry + 1];
            int tst1 = (si1->slot << 24) + (si1->subslot << 20) + 
                       (si1->startPage << 12) + si1->pageCount;
            int tst2 = (si2->slot << 24) + (si2->subslot << 20) + 
                       (si2->startPage << 12) + si2->pageCount;

            if (tst2 < tst1) {
                SlotInfo tmp;
                memcpy(&tmp, si1, sizeof(SlotInfo));
                memcpy(si1, si2, sizeof(SlotInfo));
                memcpy(si2, &tmp, sizeof(SlotInfo));
            }
        }
    }

    // Check R800 and FDC enable
    machine->cpu.hasR800 = 0;
    machine->fdc.enabled = 0;
    for (i = 0; i < machine->slotInfoCount; i++) {
        machine->cpu.hasR800 |= machine->slotInfo[i].romType == ROM_S1990;
        machine->fdc.enabled |= machine->slotInfo[i].romType == ROM_DISKPATCH     ||
                                machine->slotInfo[i].romType == ROM_TC8566AF      ||
                                machine->slotInfo[i].romType == ROM_TC8566AF_TR   ||
                                machine->slotInfo[i].romType == ROM_MICROSOL      ||
                                machine->slotInfo[i].romType == ROM_NATIONALFDC   ||
                                machine->slotInfo[i].romType == ROM_PHILIPSFDC    ||
                                machine->slotInfo[i].romType == ROM_SVI328FDC     ||
                                machine->slotInfo[i].romType == ROM_SVI738FDC;
    }

    // Check VRAM size
    if (machine->video.vdpVersion == VDP_V9938) {
        if (machine->video.vramSize >= 128 * 1024) {
            machine->video.vramSize = 128 * 1024;
        }
        else if (machine->video.vramSize >= 64 * 1024) {
            machine->video.vramSize = 64 * 1024;
        }
        else if (machine->video.vramSize >= 32 * 1024) {
            machine->video.vramSize = 32 * 1024;
        }
        else {
            machine->video.vramSize = 16 * 1024;
        }
    }
    else if (machine->video.vdpVersion == VDP_V9958) {
        if (machine->video.vramSize >= 192 * 1024) {
            machine->video.vramSize = 192 * 1024;
        }
        else {
            machine->video.vramSize = 128 * 1024;
        }
    }
    else {
        machine->video.vramSize = 16 * 1024;
    }
}


void machineLoadState(Machine* machine)
{
    SaveState* state = saveStateOpenForRead("machine");
    int hasR800 = 0;
    int i;

    saveStateGetBuffer(state, "name", machine->name, sizeof(machine->name));

    machine->board.type = saveStateGet(state, "boardType", BOARD_MSX);

    machine->slot[0].subslotted    = saveStateGet(state, "subslotted00",      0);
    machine->slot[1].subslotted    = saveStateGet(state, "subslotted01",      0);
    machine->slot[2].subslotted    = saveStateGet(state, "subslotted02",      0);
    machine->slot[3].subslotted    = saveStateGet(state, "subslotted03",      0);

    machine->cart[0].slot          = saveStateGet(state, "cartSlot00",        0);
    machine->cart[0].subslot       = saveStateGet(state, "cartSubslot00",     0);
    machine->cart[1].slot          = saveStateGet(state, "cartSlot01",        0);
    machine->cart[1].subslot       = saveStateGet(state, "cartSubslot01",     0);
    
    machine->video.vdpVersion      = saveStateGet(state, "videoVersion",      0);
    machine->video.vramSize        = saveStateGet(state, "videoVramSize",     0x10000);
    
    machine->cmos.enable           = saveStateGet(state, "cmosEnable",        0);
    machine->cmos.batteryBacked    = saveStateGet(state, "cmosBatteryBacked", 0);

    machine->audio.psgstereo       = saveStateGet(state, "audioPsgStereo", 0);

    for (i = 0; i < sizeof(machine->audio.psgpan) / sizeof(machine->audio.psgpan[0]); i++) {
        char s[32];
        sprintf(s, "audioPsgStereo%d", i);
        machine->audio.psgpan[i] = saveStateGet(state, s, i == 0 ? 0 : i == 1 ? -1 : 1);
    }

    machine->fdc.count             = saveStateGet(state, "fdcCount",          2);
    machine->cpu.freqZ80           = saveStateGet(state, "cpuFreqZ80",        3579545);
    machine->cpu.freqR800          = saveStateGet(state, "cpuFreqR800",       7159090);
    
    machine->slotInfoCount         = saveStateGet(state, "slotInfoCount",     0);

    for (i = 0; i < sizeof(machine->slotInfo) / sizeof(machine->slotInfo[0]); i++) {
        char tag[32];
        
        sprintf(tag, "slotRomType%.2d", i);
        machine->slotInfo[i].romType = saveStateGet(state, tag, 0);
        
        hasR800 |= machine->slotInfo[i].romType == ROM_S1990;

        sprintf(tag, "slot%.2d", i);
        machine->slotInfo[i].slot = saveStateGet(state, tag, 0);
        
        sprintf(tag, "subslot%.2d", i);
        machine->slotInfo[i].subslot = saveStateGet(state, tag, 0);
        
        sprintf(tag, "slotStartPage%.2d", i);
        machine->slotInfo[i].startPage = saveStateGet(state, tag, 0);
        
        sprintf(tag, "slotPageCount%.2d", i);
        machine->slotInfo[i].pageCount = saveStateGet(state, tag, 0);
        
        sprintf(tag, "slotError%.2d", i);
        machine->slotInfo[i].error = saveStateGet(state, tag, 0);
        
        sprintf(tag, "slotName%.2d", i);
        saveStateGetBuffer(state, tag, machine->slotInfo[i].name, 512);
        
        sprintf(tag, "slotInZipName%.2d", i);
        saveStateGetBuffer(state, tag, machine->slotInfo[i].inZipName, 128);
    }
    
    machine->cpu.hasR800 = hasR800;

    saveStateClose(state);

    machineUpdate(machine);
}

void machineSaveState(Machine* machine)
{
    SaveState* state = saveStateOpenForWrite("machine");
    int i;

    saveStateSetBuffer(state, "name", machine->name, sizeof(machine->name));

    saveStateSet(state, "boardType",         machine->board.type);

    saveStateSet(state, "subslotted00",      machine->slot[0].subslotted);
    saveStateSet(state, "subslotted01",      machine->slot[1].subslotted);
    saveStateSet(state, "subslotted02",      machine->slot[2].subslotted);
    saveStateSet(state, "subslotted03",      machine->slot[3].subslotted);

    saveStateSet(state, "cartSlot00",        machine->cart[0].slot);
    saveStateSet(state, "cartSubslot00",     machine->cart[0].subslot);
    saveStateSet(state, "cartSlot01",        machine->cart[1].slot);
    saveStateSet(state, "cartSubslot01",     machine->cart[1].subslot);
    
    saveStateSet(state, "videoVersion",      machine->video.vdpVersion);
    saveStateSet(state, "videoVramSize",     machine->video.vramSize);
    

    saveStateSet(state, "cmosEnable",        machine->cmos.enable);
    saveStateSet(state, "cmosBatteryBacked", machine->cmos.batteryBacked);

    saveStateSet(state, "audioPsgStereo",    machine->audio.psgstereo);

    for (i = 0; i < sizeof(machine->audio.psgpan) / sizeof(machine->audio.psgpan[0]); i++) {
        char s[32];
        sprintf(s, "audioPsgStereo%d", i);
        saveStateSet(state, s, machine->audio.psgpan[i]);
    }

    
    saveStateSet(state, "fdcCount",          machine->fdc.count);
    saveStateSet(state, "cpuFreqZ80",        machine->cpu.freqZ80);
    saveStateSet(state, "cpuFreqR800",       machine->cpu.freqR800);
    
    saveStateSet(state, "slotInfoCount",     machine->slotInfoCount);

    for (i = 0; i < sizeof(machine->slotInfo) / sizeof(machine->slotInfo[0]); i++) {
        char tag[32];
        
        sprintf(tag, "slotRomType%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].romType);
        
        sprintf(tag, "slot%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].slot);
        
        sprintf(tag, "subslot%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].subslot);
        
        sprintf(tag, "slotStartPage%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].startPage);
        
        sprintf(tag, "slotPageCount%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].pageCount);
        
        sprintf(tag, "slotError%.2d", i);
        saveStateSet(state, tag, machine->slotInfo[i].error);
        
        sprintf(tag, "slotName%.2d", i);
        saveStateSetBuffer(state, tag, machine->slotInfo[i].name, 512);
        
        sprintf(tag, "slotInZipName%.2d", i);
        saveStateSetBuffer(state, tag, machine->slotInfo[i].inZipName, 128);
    }

    saveStateClose(state);
}

int machineInitialize(Machine* machine, UInt8** mainRam, UInt32* mainRamSize, UInt32* mainRamStart)
{
    UInt8* ram       = NULL;
    UInt32 ramSize   = 0;
    UInt32 ramStart  = 0;
    UInt8* ram2      = NULL;
    UInt32 ram2Size  = 0;
    UInt32 ram2Start = 0;
    void* jisyoRom   = NULL;
    int jisyoRomSize = 0;
    int success = 1;
    int hdId = FIRST_INTERNAL_HD_INDEX;
    UInt8* buf;
    int size;
    int i;

    // Prioritize 1kB Mirrored ram as main ram (works good with coleco style
    // systems with expansion ram but maybe main ram type should be an arg instead?).
    for (i = 0; i < machine->slotInfoCount; i++) {
        int slot;
        int subslot;
        int startPage;
        char* romName;
        
        // Don't map slots with error
        if (machine->slotInfo[i].error) {
            continue;
        }

        romName   = strlen(machine->slotInfo[i].inZipName) ? machine->slotInfo[i].inZipName : machine->slotInfo[i].name;
        slot      = machine->slotInfo[i].slot;
        subslot   = machine->slotInfo[i].subslot;
        startPage = machine->slotInfo[i].startPage;
        size      = 0x2000 * machine->slotInfo[i].pageCount;

        if (machine->slotInfo[i].romType == RAM_1KB_MIRRORED) {
            success &= ramMirroredCreate(size, slot, subslot, startPage, 0x400, &ram, &ramSize);
            ramStart = startPage * 0x2000;
            continue;
        }

        if (machine->slotInfo[i].romType == RAM_2KB_MIRRORED) {
            success &= ramMirroredCreate(size, slot, subslot, startPage, 0x800, &ram, &ramSize);
            ramStart = startPage * 0x2000;
            continue;
        }
    }

    /* Initialize RAM and 'imlicit' rom types */
    for (i = 0; i < machine->slotInfoCount; i++) {
        int slot;
        int subslot;
        int startPage;
        char* romName;
        
        // Don't map slots with error
        if (machine->slotInfo[i].error) {
            continue;
        }

        romName   = strlen(machine->slotInfo[i].inZipName) ? machine->slotInfo[i].inZipName : machine->slotInfo[i].name;
        slot      = machine->slotInfo[i].slot;
        subslot   = machine->slotInfo[i].subslot;
        startPage = machine->slotInfo[i].startPage;
        size      = 0x2000 * machine->slotInfo[i].pageCount;

        if (machine->slotInfo[i].romType == RAM_NORMAL) {
            if (ram == NULL && startPage == 0) {
                success &= ramNormalCreate(size, slot, subslot, startPage, &ram, &ramSize);
                ramStart = startPage * 0x2000;
            }
            else {
                success &= ramNormalCreate(size, slot, subslot, startPage, &ram2, &ram2Size);
                ram2Start = startPage * 0x2000;
            }
            continue;
        }

        if (machine->slotInfo[i].romType == RAM_MAPPER) {
            if (ram == NULL && startPage == 0) {
                success &= ramMapperCreate(size, slot, subslot, startPage, &ram, &ramSize);
            }
            else {
                success &= ramMapperCreate(size, slot, subslot, startPage, NULL, NULL);
            }
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_JISYO) {
            if (jisyoRom == NULL) {
                jisyoRom = romLoad(machine->slotInfo[i].name, machine->slotInfo[i].inZipName, &jisyoRomSize);

                if (jisyoRom == NULL) {
                    success = 0;
                    continue;
                }
            }
            continue;
        }
    }

    if (ram == NULL) {
        ram = ram2;
        ramSize = ram2Size;
        ramStart = ram2Start;
    }

    if (ram == NULL) {
        return 0;
    }

    for (i = 0; i < machine->slotInfoCount; i++) {
        int slot;
        int subslot;
        int startPage;
        char* romName;
        
        // Don't map slots with error
        if (machine->slotInfo[i].error) {
            continue;
        }

        romName   = strlen(machine->slotInfo[i].inZipName) ? machine->slotInfo[i].inZipName : machine->slotInfo[i].name;
        slot      = machine->slotInfo[i].slot;
        subslot   = machine->slotInfo[i].subslot;
        startPage = machine->slotInfo[i].startPage;
        size      = 0x2000 * machine->slotInfo[i].pageCount;
        
        if (machine->slotInfo[i].romType == RAM_1KB_MIRRORED) {
            continue;
        }
        
        if (machine->slotInfo[i].romType == RAM_2KB_MIRRORED) {
            continue;
        }

        if (machine->slotInfo[i].romType == RAM_NORMAL) {
            continue;
        }
        
        if (machine->slotInfo[i].romType == RAM_MAPPER) {
            continue;
        }
        
        if (machine->slotInfo[i].romType == ROM_JISYO) {
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SNATCHER) {
            success &= romMapperSCCplusCreate(NULL, NULL, 0, slot, subslot, startPage, SCC_SNATCHER);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SDSNATCHER) {
            success &= romMapperSCCplusCreate(NULL, NULL, 0, slot, subslot, startPage, SCC_SDSNATCHER);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SCCMIRRORED) {
            success &= romMapperSCCplusCreate(NULL, NULL, 0, slot, subslot, startPage, SCC_MIRRORED);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SCCEXTENDED) {
            success &= romMapperSCCplusCreate(NULL, NULL, 0, slot, subslot, startPage, SCC_EXTENDED);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_PAC) {
            success &= romMapperPACCreate("Pac.rom", NULL, 0, slot, subslot, startPage);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_EXTRAM) {
            success &= ramMapperCreate(size, slot, subslot, startPage, NULL, NULL);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_MEGARAM) {
            success &= romMapperMegaRAMCreate(size, slot, subslot, startPage);
            continue;
        }

        if (machine->slotInfo[i].romType == SRAM_MATSUCHITA) {
            success &= sramMapperMatsushitaCreate(0);
            continue;
        }

        if (machine->slotInfo[i].romType == SRAM_MATSUCHITA_INV) {
            success &= sramMapperMatsushitaCreate(1);
            continue;
        }

        if (machine->slotInfo[i].romType == SRAM_S1985) {
            success &= sramMapperS1985Create();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_S1990) {
            success &= romMapperS1990Create();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_TURBORTIMER) {
            success &= romMapperTurboRTimerCreate(0);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_TURBORIO) {
            success &= romMapperTurboRIOCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_F4DEVICE) {
            success &= romMapperF4deviceCreate(0);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_MSXMIDI) {
            success &= MSXMidiCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_F4INVERTED) {
            success &= romMapperF4deviceCreate(1);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_NMS8280DIGI) {
            success &= romMapperNms8280VideoDaCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_GIDE) {
            success &= romMapperGIdeCreate(hdId++);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SONYHBI55) {
            success &= romMapperSonyHBI55Create();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_MSXAUDIODEV) {
            success &= romMapperMsxAudioCreate(NULL, NULL, 0, 0, 0, 0);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_JOYREXPSG) {
            success &= romMapperJoyrexPsgCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_OPCODEPSG) {
            success &= romMapperOpcodePsgCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_TURBORPCM) {
            success &= romMapperTurboRPcmCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_MSXPRN) {
            success &= romMapperMsxPrnCreate();
            continue;
        }

        // --------- ColecoVision Super Expansion Module specific mappers
        if (machine->slotInfo[i].romType == ROM_OPCODEMEGA) {
            success &= romMapperOpcodeMegaRamCreate(slot, subslot, startPage);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_OPCODESAVE) {
            success &= romMapperOpcodeSaveRamCreate(slot, subslot, startPage);
            continue;
        }
        
        if (machine->slotInfo[i].romType == ROM_OPCODESLOT) {
            success &= romMapperOpcodeSlotManagerCreate();
            continue;
        }

        // --------- SVI specific mappers
        if (machine->slotInfo[i].romType == ROM_SVI328FDC) {
            success &= svi328FdcCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SVI328PRN) {
            success &= romMapperSvi328PrnCreate();
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SVI328RS232) {
            success &= romMapperSvi328Rs232Create(SVI328_RS232);
            continue;
        }

        if (machine->slotInfo[i].romType == ROM_SVI328RSIDE) {
            success &= romMapperSvi328RsIdeCreate(hdId++);
            continue;
        }
        
        // -------------------------------
        
        // MEGA-SCSI etc
        switch (machine->slotInfo[i].romType) {
        case SRAM_MEGASCSI:
        case SRAM_ESERAM:
        case SRAM_WAVESCSI:
        case SRAM_ESESCC:
            buf = NULL;
            if (strlen(romName)) {
                buf = romLoad(machine->slotInfo[i].name, machine->slotInfo[i].inZipName, &size);
                if (buf == NULL) {
                    success = 0;
                    continue;
                }
            }
            {
                int mode = strlen(machine->slotInfo[i].inZipName) ? 0x80 : 0;
                if (machine->slotInfo[i].romType == SRAM_MEGASCSI ||
                    machine->slotInfo[i].romType == SRAM_WAVESCSI) {
                    mode++;
                }
                if (machine->slotInfo[i].romType == SRAM_WAVESCSI ||
                    machine->slotInfo[i].romType == SRAM_ESESCC) {
                    success &= sramMapperEseSCCCreate
                                (romName, buf, size, slot, subslot, startPage,
                                machine->slotInfo[i].romType == SRAM_WAVESCSI ? hdId++ : 0, mode);
                } else {
                    success &= sramMapperMegaSCSICreate
                                (romName, buf, size, slot, subslot, startPage,
                                machine->slotInfo[i].romType == SRAM_MEGASCSI ? hdId++ : 0, mode);
                }
                if (buf) free(buf);
            }
            continue;
        }
        // -------------------------------

        buf = romLoad(machine->slotInfo[i].name, machine->slotInfo[i].inZipName, &size);

        if (buf == NULL) {

            switch (machine->slotInfo[i].romType) {
            case ROM_MEGAFLSHSCC:
                success &= romMapperMegaFlashRomSccCreate("Manbow2.rom", NULL, 0, slot, subslot, startPage, 0);
                break;
            default:
                success = 0;
                break;
            }
            continue;
        }

        switch (machine->slotInfo[i].romType) {
        case ROM_0x4000:
            success &= romMapperNormalCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_0xC000:
            success &= romMapperNormalCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_BASIC:
            success &= romMapperBasicCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_PLAIN:
            success &= romMapperPlainCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_NETTOUYAKYUU:
            success &= romMapperNettouYakyuuCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_MATRAINK:
            success &= romMapperMatraINKCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_FORTEII:
            success &= romMapperForteIICreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_FMDAS:
            success &= romMapperFmDasCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_STANDARD:
            success &= romMapperStandardCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_MSXDOS2:
            success &= romMapperMsxDos2Create(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_KONAMI5:
            success &= romMapperKonami5Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_MANBOW2:
            if (size > 0x70000) size = 0x70000;
            success &= romMapperMegaFlashRomSccCreate(romName, buf, size, slot, subslot, startPage, 0x7f);
            break;

        case ROM_MEGAFLSHSCC:
            success &= romMapperMegaFlashRomSccCreate(romName, buf, size, slot, subslot, startPage, 0);
            break;

        case ROM_OBSONET:
            success &= romMapperObsonetCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_NOWIND:
            success &= romMapperNoWindCreate(hdId++, romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_DUMAS:
            {
                char eepromName[512];
                int eepromSize = 0;
                UInt8* eepromData;
                int j;

                strcpy(eepromName, machine->slotInfo[i].name);
                for (j = strlen(eepromName); j > 0 && eepromName[j] != '.'; j--);
                eepromName[j] = 0;
                strcat(eepromName, "_eeprom.rom");
                    
                eepromData = romLoad(eepromName, NULL, &eepromSize);
                success &= romMapperDumasCreate(romName, buf, size, slot, subslot, startPage,
                                                eepromData, eepromSize);
                if (eepromData != NULL) {
                    free(eepromData);
                }
            }
            break;

        case ROM_MOONSOUND:
            success &= romMapperMoonsoundCreate(romName, buf, size, 640);
            buf = NULL; // Ownership transferred to emulation of moonsound
            break;

        case ROM_SCC:
            success &= romMapperSCCplusCreate(romName, buf, size, slot, subslot, startPage, SCC_EXTENDED);
            break;

        case ROM_SCCPLUS:
            success &= romMapperSCCplusCreate(romName, buf, size, slot, subslot, startPage, SCCP_EXTENDED);
            break;
            
        case ROM_KONAMI4:
            success &= romMapperKonami4Create(romName, buf, size, slot, subslot, startPage);
            break;

#ifdef WIN32
        case ROM_GAMEREADER:
            success &= romMapperGameReaderCreate(0, slot, subslot);
            break;
#endif

        case ROM_MAJUTSUSHI:
            success &= romMapperMajutsushiCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_HOLYQURAN:
            success &= romMapperHolyQuranCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_KONAMISYNTH:
            success &= romMapperKonamiSynthCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_KONWORDPRO:
            success &= romMapperKonamiWordProCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_KONAMKBDMAS:
            {
                char voiceName[512];
                int voiceSize = 0;
                UInt8* voiceData;
                int j;

                strcpy(voiceName, machine->slotInfo[i].name);
                for (j = strlen(voiceName); j > 0 && voiceName[j] != '.'; j--);
                voiceName[j] = 0;
                strcat(voiceName, "_voice.rom");
                    
                voiceData = romLoad(voiceName, NULL, &voiceSize);
                success &= romMapperKonamiKeyboardMasterCreate(romName, buf, size, 
                                                               slot, subslot, startPage,
                                                               voiceData, voiceSize);
                if (voiceData != NULL) {
                    free(voiceData);
                }
            }
            break;
            
        case ROM_ASCII8:
            success &= romMapperASCII8Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_ASCII16:
            success &= romMapperASCII16Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_PANASONIC8:
            success &= romMapperA1FMCreate(romName, buf, size, slot, subslot, startPage, 0x2000);
            break;

        case ROM_PANASONICWX16:
            success &= romMapperPanasonicCreate(romName, buf, size, slot, subslot, startPage, 0x4000, 6);
            break;

        case ROM_PANASONIC16:
            success &= romMapperPanasonicCreate(romName, buf, size, slot, subslot, startPage, 0x4000, 8);
            break;

        case ROM_PANASONIC32:
            success &= romMapperPanasonicCreate(romName, buf, size, slot, subslot, startPage, 0x8000, 8);
            break;

        case ROM_FSA1FMMODEM:
            success &= romMapperA1FMModemCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_ASCII8SRAM:
            success &= romMapperASCII8sramCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_ASCII16SRAM:
            success &= romMapperASCII16sramCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_MSXAUDIO:
            success &= romMapperMsxAudioCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_YAMAHASFG01:
            success &= romMapperSfg05Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_YAMAHASFG05:
            success &= romMapperSfg05Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_YAMAHANET:
            success &= romMapperNetCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SF7000IPL:
            success &= romMapperSf7000IplCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_KOEI:
            success &= romMapperKoeiCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_NATIONAL:
            success &= romMapperNationalCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_KONAMI4NF:
            success &= romMapperKonami4nfCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_ASCII16NF:
            success &= romMapperASCII16nfCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_GAMEMASTER2:
            success &= romMapperGameMaster2Create(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_HARRYFOX:
            success &= romMapperHarryFoxCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_HALNOTE:
            success &= romMapperHalnoteCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_RTYPE:
            success &= romMapperRTypeCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_CROSSBLAIM:
            success &= romMapperCrossBlaimCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_LODERUNNER:
            success &= romMapperLodeRunnerCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_KOREAN80:
            success &= romMapperKorean80Create(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_KOREAN90:
            success &= romMapperKorean90Create(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_KOREAN126:
            success &= romMapperKorean126Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_FMPAK:
            success &= romMapperFMPAKCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_FMPAC:
            success &= romMapperFMPACCreate(romName, buf, size, slot, subslot, startPage);
            break;
            
        case ROM_MSXMUSIC:
            success &= romMapperMsxMusicCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_NORMAL:
            success &= romMapperNormalCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_DRAM:
            success &= romMapperDramCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SG1000:
        case ROM_SC3000:
            success &= romMapperSg1000Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SG1000CASTLE:
            success &= romMapperSg1000CastleCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SEGABASIC:
            success &= romMapperSegaBasicCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_CASPATCH:
            success &= romMapperCasetteCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_DISKPATCH:
            success &= romMapperDiskCreate(romName, buf, size, slot, subslot, startPage);
           break;

        case ROM_TC8566AF:
            success &= romMapperTC8566AFCreate(romName, buf, size, slot, subslot, startPage, ROM_TC8566AF);
            break;
        case ROM_TC8566AF_TR:
            success &= romMapperTC8566AFCreate(romName, buf, size, slot, subslot, startPage, ROM_TC8566AF_TR);
            break;

        case ROM_MICROSOL:
            success &= romMapperMicrosolCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_ARC:
            success &= romMapperArcCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_NATIONALFDC:
            success &= romMapperNationalFdcCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_PHILIPSFDC:
            success &= romMapperPhilipsFdcCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SVI738FDC:
            success &= romMapperSvi738FdcCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_KANJI:
            success &= romMapperKanjiCreate(buf, size);
            break;

        case ROM_KANJI12:
            success &= romMapperKanji12Create(buf, size);
            break;

        case ROM_BUNSETU:
            success &= romMapperBunsetuCreate(romName, buf, size, slot, subslot, startPage, jisyoRom, jisyoRomSize);
            break;

        case ROM_SUNRISEIDE:
            success &= romMapperSunriseIdeCreate(hdId++, romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_BEERIDE:
            success &= romMapperBeerIdeCreate(hdId++, romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_GOUDASCSI:
            success &= romMapperGoudaSCSICreate(hdId++, romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_SONYHBIV1:
            success &= romMapperSonyHbiV1Create(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_PLAYBALL:
            success &= romMapperPlayBallCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_OPCODEBIOS:
            success &= romMapperOpcodeBiosCreate(romName, buf, size, slot, subslot, startPage);
            break;

        case ROM_MICROSOL80:
            {
                char charName[512];
                int charSize = 0;
                UInt8* charData;
                int j;

                strcpy(charName, machine->slotInfo[i].name);
                for (j = strlen(charName); j > 0 && charName[j] != '.'; j--);
                charName[j] = 0;
                strcat(charName, "_char.rom");
                    
                charData = romLoad(charName, NULL, &charSize);
                success &= romMapperMicrosolVmx80Create(romName, buf, size, 
                                                        slot, subslot, startPage,
                                                        charData, charSize);
                if (charData != NULL) {
                    free(charData);
                }
            }
            break;

        case ROM_SVI727:
            success &= romMapperSvi727Create(romName, buf, size, slot, subslot, startPage);
            break;
        }
        if( buf != NULL ) {
            free(buf);
        }
    }

    if (jisyoRom != NULL) {
        free(jisyoRom);
    }

    if (mainRam != NULL) {
        *mainRam = ram;
    }
    
    if (mainRamSize != NULL) {
        *mainRamSize = ramSize;
    }

    if (mainRamStart != NULL) {
        *mainRamStart = ramStart;
    }

    return success;
}

