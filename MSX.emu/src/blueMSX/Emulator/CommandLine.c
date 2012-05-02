/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Emulator/CommandLine.c,v $
**
** $Revision: 1.35 $
**
** $Date: 2008/08/31 06:13:13 $
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
#include "CommandLine.h"
#include "TokenExtract.h"
#include "IsFileExtension.h"
#include "MediaDb.h"
#include "ziphelper.h"
#include "Machine.h"
#include "Casette.h"
#include "Disk.h"
#include "FileHistory.h"
#include "LaunchFile.h"
#include "Emulator.h"
#include "StrcmpNoCase.h"
#include "AppConfig.h"
#include <stdlib.h>
#include <string.h>

static RomType romNameToType(char* name) {
    RomType romType = ROM_UNKNOWN;

    if (name == NULL) {
        return ROM_UNKNOWN;
    }

    romType = mediaDbStringToType(name);

    if (romType == ROM_UNKNOWN) {
        romType = atoi(name);
        if (romType < ROM_STANDARD || romType > ROM_MAXROMID) {
            romType = ROM_UNKNOWN;
        }
    }

    return romType;
}

static int isRomFileType(char* filename, char* inZip) {
    inZip[0] = 0;

    if (isFileExtension(filename, ".zip")) {
        int count;
        char* fileList;

        fileList = zipGetFileList(filename, ".rom", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".ri", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".mx1", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".mx2", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".col", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".sg", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".sc", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        return 0;
    }

    return isFileExtension(filename, ".rom") ||
           isFileExtension(filename, ".ri")  ||
           isFileExtension(filename, ".mx1") ||
           isFileExtension(filename, ".mx2") ||
           isFileExtension(filename, ".col") ||
           isFileExtension(filename, ".sg") ||
           isFileExtension(filename, ".sc");
}

static int isDskFileType(char* filename, char* inZip) {
    inZip[0] = 0;

    if (isFileExtension(filename, ".zip")) {
        int count;
        char* fileList;

        fileList = zipGetFileList(filename, ".dsk", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".di1", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".di2", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".360", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".720", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        fileList = zipGetFileList(filename, ".sf7", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }

        return 0;
    }

    return isFileExtension(filename, ".dsk") ||
           isFileExtension(filename, ".di1") ||
           isFileExtension(filename, ".di2") ||
           isFileExtension(filename, ".360") ||
           isFileExtension(filename, ".720") ||
           isFileExtension(filename, ".Sf7");
}

static int isCasFileType(char* filename, char* inZip) {
    inZip[0] = 0;

    if (isFileExtension(filename, ".zip")) {
        int count;
        char* fileList;

        fileList = zipGetFileList(filename, ".cas", &count);
        if (fileList) {
            strcpy(inZip, fileList);
            free(fileList);
            return 1;
        }
        return 0;
    }

    return isFileExtension(filename, ".cas");
}

static int checkArg(const char* arg, const char* value) {
    if (arg[0] != '/' && arg[0] != '-') {
        return 0;
    }

    return strcmpnocase(arg + 1, value) == 0;
}


int emuCheckResetArgument(char* cmdLine) {
    int i;
    char*   argument;

    for (i = 0; (argument = extractToken(cmdLine, i)) != NULL; i++) {
        if (checkArg(argument, "reset")) {
            return 1;
        }
        if (checkArg(argument, "resetregs")) {
            return 2;
        }
    }

    return 0;
}

char* emuCheckThemeArgument(char* cmdLine){
    static char themeName[PROP_MAXPATH];
    int i;
    char* argument;

    themeName[0] = 0;

    for (i = 0; (argument = extractToken(cmdLine, i)) != NULL; i++) {
        if (checkArg(argument, "theme")) {
            argument = extractToken(cmdLine, i + 1);
            if (argument != NULL) {
                strcat(themeName, argument);
            }
            return themeName;
        }
    }

    return NULL;
}

void emuCheckFullscreenArgument(Properties* properties, char* cmdLine){
    int i;
    char* argument;

    if (NULL == extractToken(cmdLine, 0)) {
        return;
    }

//    properties->video.windowSize = P_VIDEO_SIZEX2;

    for (i = 0; (argument = extractToken(cmdLine, i)) != NULL; i++) {
        if (checkArg(argument, "fullscreen")) {
            properties->video.windowSize = P_VIDEO_SIZEFULLSCREEN;
        }
    }
}

static int emuStartWithArguments(Properties* properties, char* commandLine, char *gamedir) {
    int i;
    char    cmdLine[512] = "";
    char*   argument;
    char    rom1[512] = "";
    char    rom2[512] = "";
    char    rom1zip[256] = "";
    char    rom2zip[256] = "";
    RomType romType1  = ROM_UNKNOWN;
    RomType romType2  = ROM_UNKNOWN;
    char    machineName[64] = "";
    char    diskA[512] = "";
    char    diskB[512] = "";
    char    diskAzip[256] = "";
    char    diskBzip[256] = "";
    char    ide1p[256] = "";
    char    ide1s[256] = "";
    char    cas[512] = "";
    char    caszip[256] = "";
    int     fullscreen = 0;
#ifdef WII
    int     startEmu = 1; // always start
#else
    int     startEmu = 0;
#endif

    if (commandLine[0] != '/' && commandLine[1] == ':') {
        char* ptr;
        strcat(cmdLine, "\"");
        strcat(cmdLine, commandLine);
        ptr = cmdLine + strlen(commandLine);
        while (ptr > cmdLine && *ptr == ' ') {
            *ptr-- = '\0';
        }
        strcat(cmdLine, "\"");
    }
    else {
        strcat(cmdLine, commandLine);
    }

    // If one argument, assume it is a rom or disk to run
    if (!extractToken(cmdLine, 1)) {
        argument = extractToken(cmdLine, 0);

        if (argument && *argument != '/') {
            if (*argument == '\"') argument++;

            if (*argument) {
                int i;

                for (i = 0; i < PROP_MAX_CARTS; i++) {
                    properties->media.carts[i].fileName[0] = 0;
                    properties->media.carts[i].fileNameInZip[0] = 0;
                    properties->media.carts[i].type = ROM_UNKNOWN;
                    updateExtendedRomName(i, properties->media.carts[i].fileName, properties->media.carts[i].fileNameInZip);
                }

                for (i = 0; i < PROP_MAX_DISKS; i++) {
                    properties->media.disks[i].fileName[0] = 0;
                    properties->media.disks[i].fileNameInZip[0] = 0;
                    updateExtendedDiskName(i, properties->media.disks[i].fileName, properties->media.disks[i].fileNameInZip);
                }

                return tryLaunchUnknownFile(properties, argument, 1);
            }
            return 0;
        }
    }

    // If more than one argument, check arguments,
    // set configuration and then run

    for (i = 0; (argument = extractToken(cmdLine, i)) != NULL; i++) {
        if (checkArg(argument, "rom1")) {
            argument = extractTokenEx(cmdLine, ++i, gamedir);
            if (argument == NULL || !isRomFileType(argument, rom1zip)) return 0; // Invaid argument
            strcpy(rom1, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "rom1zip")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0;
            strcpy(rom1zip, argument);
        }
        if (checkArg(argument, "romtype1")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0; // Invaid argument
            romType1 = romNameToType(argument);
            startEmu = 1;
        }
        if (checkArg(argument, "rom2")) {
            argument = extractTokenEx(cmdLine, ++i, gamedir);
            if (argument == NULL || !isRomFileType(argument, rom2zip)) return 0; // Invaid argument
            strcpy(rom2, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "rom2zip")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0;
            strcpy(rom2zip, argument);
        }
        if (checkArg(argument, "romtype2")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0; // Invaid argument
            romType2 = romNameToType(argument);
            startEmu = 1;
        }
        if (checkArg(argument, "diskA")) {
            argument = extractTokenEx(cmdLine, ++i, gamedir);
            if (argument == NULL || !isDskFileType(argument, diskAzip)) return 0; // Invaid argument
            strcpy(diskA, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "diskAzip")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0;
            strcpy(diskAzip, argument);
        }
        if (checkArg(argument, "diskB")) {
            argument = extractTokenEx(cmdLine, ++i, gamedir);
            if (argument == NULL || !isDskFileType(argument, diskBzip)) return 0; // Invaid argument
            strcpy(diskB, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "diskBzip")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0;
            strcpy(diskBzip, argument);
        }
        if (checkArg(argument, "cas")) {
            argument = extractTokenEx(cmdLine, ++i, gamedir);
            if (argument == NULL || !isCasFileType(argument, caszip)) return 0; // Invaid argument
            strcpy(cas, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "ide1primary")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0; // Invaid argument
            strcpy(ide1p, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "ide1secondary")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0; // Invaid argument
            strcpy(ide1s, argument);
            startEmu = 1;
        }
        if (checkArg(argument, "caszip")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0;
            strcpy(caszip, argument);
        }
        if (checkArg(argument, "machine")) {
            argument = extractToken(cmdLine, ++i);
            if (argument == NULL) return 0; // Invaid argument
            strcpy(machineName, argument);
            if (!machineIsValid(machineName, 1)) return 0;
            startEmu = 1;
        }
        if (checkArg(argument, "fullscreen")) {
            fullscreen = 1;
        }
    }

    if (!startEmu) {
        return 1;
    }

    for (i = 0; i < PROP_MAX_CARTS; i++) {
        properties->media.carts[i].fileName[0] = 0;
        properties->media.carts[i].fileNameInZip[0] = 0;
        properties->media.carts[i].type = ROM_UNKNOWN;
        updateExtendedRomName(i, properties->media.carts[i].fileName, properties->media.carts[i].fileNameInZip);
    }

    for (i = 0; i < PROP_MAX_DISKS; i++) {
        properties->media.disks[i].fileName[0] = 0;
        properties->media.disks[i].fileNameInZip[0] = 0;
        updateExtendedDiskName(i, properties->media.disks[i].fileName, properties->media.disks[i].fileNameInZip);
    }

    for (i = 0; i < PROP_MAX_TAPES; i++) {
        properties->media.tapes[i].fileName[0] = 0;
        properties->media.tapes[i].fileNameInZip[0] = 0;
        updateExtendedCasName(i, properties->media.tapes[i].fileName, properties->media.tapes[i].fileNameInZip);
    }

    if (!strlen(rom1)) {
        switch (romType1) {
        case ROM_SCC:         strcat(rom1, CARTNAME_SCC); romType1 = ROM_SCC; break;
        case ROM_SCCPLUS:     strcat(rom1, CARTNAME_SCCPLUS); romType1 = ROM_SCCPLUS; break;
        case ROM_SNATCHER:    strcat(rom1, CARTNAME_SNATCHER); break;
        case ROM_SDSNATCHER:  strcat(rom1, CARTNAME_SDSNATCHER); break;
        case ROM_SCCMIRRORED: strcat(rom1, CARTNAME_SCCMIRRORED); break;
        case ROM_SCCEXTENDED: strcat(rom1, CARTNAME_SCCEXPANDED); break;
        case ROM_FMPAC:       strcat(rom1, CARTNAME_FMPAC); break;
        case ROM_PAC:         strcat(rom1, CARTNAME_PAC); break;
        case ROM_GAMEREADER:  strcat(rom1, CARTNAME_GAMEREADER); break;
        case ROM_SUNRISEIDE:  strcat(rom1, CARTNAME_SUNRISEIDE); break;
        case ROM_NOWIND:      strcat(rom1, CARTNAME_NOWINDDOS1); break;
        case ROM_BEERIDE:     strcat(rom1, CARTNAME_BEERIDE); break;
        case ROM_GIDE:        strcat(rom1, CARTNAME_GIDE); break;
        case ROM_GOUDASCSI:   strcat(rom1, CARTNAME_GOUDASCSI); break;
        case ROM_NMS1210:     strcat(rom1, CARTNAME_NMS1210); break;
        case ROM_SONYHBI55:   strcat(rom1, CARTNAME_SONYHBI55); break;
        }
    }

    if (!strlen(rom2)) {
        switch (romType2) {
        case ROM_SCC:         strcat(rom2, CARTNAME_SCC); romType2 = ROM_SCC; break;
        case ROM_SCCPLUS:     strcat(rom2, CARTNAME_SCCPLUS); romType2 = ROM_SCCPLUS; break;
        case ROM_SNATCHER:    strcat(rom2, CARTNAME_SNATCHER); break;
        case ROM_SDSNATCHER:  strcat(rom2, CARTNAME_SDSNATCHER); break;
        case ROM_SCCMIRRORED: strcat(rom2, CARTNAME_SCCMIRRORED); break;
        case ROM_SCCEXTENDED: strcat(rom2, CARTNAME_SCCEXPANDED); break;
        case ROM_FMPAC:       strcat(rom2, CARTNAME_FMPAC); break;
        case ROM_PAC:         strcat(rom2, CARTNAME_PAC); break;
        case ROM_GAMEREADER:  strcat(rom2, CARTNAME_GAMEREADER); break;
        case ROM_SUNRISEIDE:  strcat(rom2, CARTNAME_SUNRISEIDE); break;
        case ROM_NOWIND:      strcat(rom2, CARTNAME_NOWINDDOS1); break;
        case ROM_BEERIDE:     strcat(rom2, CARTNAME_BEERIDE); break;
        case ROM_GIDE:        strcat(rom2, CARTNAME_GIDE); break;
        case ROM_GOUDASCSI:   strcat(rom2, CARTNAME_GOUDASCSI); break;
        case ROM_NMS1210:     strcat(rom2, CARTNAME_NMS1210); break;
        case ROM_SONYHBI55:   strcat(rom2, CARTNAME_SONYHBI55); break;
        }
    }

    if (properties->cassette.rewindAfterInsert) tapeRewindNextInsert();

    if (strlen(rom1)  && !insertCartridge(properties, 0, rom1, *rom1zip ? rom1zip : NULL, romType1, -1)) return 0;
    if (strlen(rom2)  && !insertCartridge(properties, 1, rom2, *rom2zip ? rom2zip : NULL, romType2, -1)) return 0;
    if (strlen(diskA) && !insertDiskette(properties, 0, diskA, *diskAzip ? diskAzip : NULL, -1)) return 0;
    if (strlen(diskB) && !insertDiskette(properties, 1, diskB, *diskBzip ? diskBzip : NULL, -1)) return 0;
    if (strlen(ide1p) && !insertDiskette(properties, diskGetHdDriveId(0, 0), ide1p, NULL, -1)) return 0;
    if (strlen(ide1s) && !insertDiskette(properties, diskGetHdDriveId(0, 1), ide1s, NULL, -1)) return 0;
    if (strlen(cas)   && !insertCassette(properties, 0, cas, *caszip ? caszip : NULL, -1)) return 0;

    if (strlen(machineName)) strcpy(properties->emulation.machineName, machineName);
#ifdef WII
    else strcpy(properties->emulation.machineName, "MSX2 - No Moonsound"); /* If not specified, use MSX2 without moonsound as default */
#endif

    emulatorStop();
    emulatorStart(NULL);

    return 1;
}

int emuTryStartWithArguments(Properties* properties, char* cmdLine, char *gamedir) {
    if (cmdLine == NULL || *cmdLine == 0) {
        if (appConfigGetInt("autostart", 0) != 0) {
            emulatorStop();
            emulatorStart(properties->filehistory.quicksave);
        }
        return 0;
    }

    if (*cmdLine) {
        int success;
        if (0 == strncmp(cmdLine, "/onearg ", 8)) {
            char args[2048];
            char* ptr;
            sprintf(args, "\"%s", cmdLine + 8);
            ptr = args + strlen(args);
            while(*--ptr == ' ') {
                *ptr = 0;
            }
            strcat(args, "\"");
            success = emuStartWithArguments(properties, args, gamedir);
        }
        else {
            success = emuStartWithArguments(properties, cmdLine, gamedir);
        }
        if (!success) {
            return -1;
        }
    }

    return 1;
}
