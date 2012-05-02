/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/LaunchFile.c,v $
**
** $Revision: 1.32 $
**
** $Date: 2009-04-30 03:53:28 $
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
#include "LaunchFile.h"
#include "IsFileExtension.h"
#include "ziphelper.h"
#include "RomLoader.h"
#include "FileHistory.h"
#include "Emulator.h"
#include "Board.h"
#include "ArchFile.h"
#include "Disk.h"
#include "Casette.h"
#include "ArchDialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ArchDialog.h"


// PacketFileSystem.h Need to be included after all other includes
#include "PacketFileSystem.h"


void archUpdateMenu(int show);

int insertCartridge(Properties* properties, int drive, const char* fname, const char* inZipFile, RomType romType, int forceAutostart) {
    int autostart = forceAutostart == 1 || properties->cartridge.autoReset;
    int noautostart = forceAutostart == -1;
    char romName[512] = "";
    char filename[512] = "";
    int isZip = isFileExtension(fname, ".zip");

    if (fname) strcpy(filename, fname);
    
    emulatorResetMixer();

    if (isZip) {
        if (inZipFile != NULL) {
            strcpy(romName, inZipFile);
        }
        else {
            // This is really ugly and should be done nicer...
            // The idea is to get files of three types and merge them into
            // one list. Maybe it should be done in zipGetFileList?
            int i;
            char* fileList;
            int countRom;
            int countRi;
            int countMx1;
            int countMx2;
            int countCol;
            int countSg;
            int countSc;
            char* fileListRom = zipGetFileList(filename, ".rom", &countRom);
            char* fileListRi  = zipGetFileList(filename, ".ri",  &countRi);
            char* fileListMx1 = zipGetFileList(filename, ".mx1", &countMx1);
            char* fileListMx2 = zipGetFileList(filename, ".mx2", &countMx2);
            char* fileListCol = zipGetFileList(filename, ".col", &countCol);
            char* fileListSg  = zipGetFileList(filename, ".sg",  &countSg);
            char* fileListSc  = zipGetFileList(filename, ".sc",  &countSc);
            int count = countRom + countRi + countMx1 + countMx2 + countCol + countSg + countSc;
            int sizeRom = 0;
            int sizeRi  = 0;
            int sizeMx1 = 0;
            int sizeMx2 = 0;
            int sizeCol = 0;
            int sizeSg = 0;
            int sizeSc = 0;

            for (i = 0; i < countRom; i++) {
                sizeRom += strlen(fileListRom + sizeRom) + 1;
            }
            for (i = 0; i < countRi; i++) {
                sizeRi += strlen(fileListRi + sizeRi) + 1;
            }
            for (i = 0; i < countMx1; i++) {
                sizeMx1 += strlen(fileListMx1 + sizeMx1) + 1;
            }
            for (i = 0; i < countMx2; i++) {
                sizeMx2 += strlen(fileListMx2 + sizeMx2) + 1;
            }
            for (i = 0; i < countCol; i++) {
                sizeCol += strlen(fileListCol + sizeCol) + 1;
            }
            for (i = 0; i < countSg; i++) {
                sizeSg += strlen(fileListSg + sizeSg) + 1;
            }
            for (i = 0; i < countSc; i++) {
                sizeSc += strlen(fileListSc + sizeSc) + 1;
            }

            fileList = malloc(sizeRom + sizeMx1 + sizeMx2 + sizeCol + sizeSg + sizeSc);
            memcpy(fileList, fileListRom, sizeRom);
            memcpy(fileList + sizeRom, fileListMx1, sizeMx1);
            memcpy(fileList + sizeRom + sizeMx1, fileListMx2, sizeMx2);
            memcpy(fileList + sizeRom + sizeMx1 + sizeMx2, fileListCol, sizeCol);
            memcpy(fileList + sizeRom + sizeMx1 + sizeMx2 + sizeCol, fileListRi, sizeRi);
            memcpy(fileList + sizeRom + sizeMx1 + sizeMx2 + sizeCol + sizeRi, fileListSg, sizeSg);
            memcpy(fileList + sizeRom + sizeMx1 + sizeMx2 + sizeCol + sizeRi + sizeSg, fileListSc, sizeSc);

            if (count == 0) {
                archShowNoRomInZipDialog();
                return 0;
            }

            if (count == 1) {
                strcpy(romName, fileList);
            }
            else {
                char* filename = archFilenameGetOpenRomZip(properties, drive, fname, fileList, count, &autostart, &romType);
                if (filename == NULL) {
                    free(fileList);
                    return 0;
                }
                strcpy(romName, filename);
            }

            if(fileListRom) free(fileListRom);
            if(fileListRi)  free(fileListRi);
            if(fileListMx1) free(fileListMx1);
            if(fileListMx2) free(fileListMx2);
            if(fileListCol) free(fileListCol);
            if(fileListSg)  free(fileListSg);
            if(fileListSc)  free(fileListSc);
            free(fileList);
        }
    }
    {
        int size;
        char* buf = romLoad(filename, isZip ? romName : NULL, &size);
        char prettyRomName[256];

        if (buf != NULL) {
            MediaType* mediaType  = mediaDbGuessRom(buf, size);
            RomType    chkRomType = mediaDbGetRomType(mediaType);
            strcpy(prettyRomName, mediaDbGetPrettyString(mediaType));
            free(buf);
            
            if (prettyRomName[0] != 0) {
                setExtendedRomName(drive, prettyRomName);
            }
            else {
                setExtendedRomName(drive, stripPathExt(isZip ? romName : filename));
            }

            if (romType == ROM_UNKNOWN) {
//                romType = chkRomType;
            }
        }
    }

    romType = 0 == strcmp(CARTNAME_SNATCHER,    filename) ? ROM_SNATCHER :
              0 == strcmp(CARTNAME_SDSNATCHER,  filename) ? ROM_SDSNATCHER :
              0 == strcmp(CARTNAME_SCCMIRRORED, filename) ? ROM_SCCMIRRORED :
              0 == strcmp(CARTNAME_SCCEXPANDED, filename) ? ROM_SCCEXTENDED :
              0 == strcmp(CARTNAME_SCC,         filename) ? ROM_SCC :
              0 == strcmp(CARTNAME_SCCPLUS,     filename) ? ROM_SCCPLUS :
              0 == strcmp(CARTNAME_JOYREXPSG,   filename) ? ROM_JOYREXPSG :
              0 == strcmp(CARTNAME_FMPAC,       filename) ? ROM_FMPAC :
              0 == strcmp(CARTNAME_PAC,         filename) ? ROM_PAC :
              0 == strcmp(CARTNAME_GAMEREADER,  filename) ? ROM_GAMEREADER :
              0 == strcmp(CARTNAME_SUNRISEIDE,  filename) ? ROM_SUNRISEIDE :
              0 == strcmp(CARTNAME_GIDE,        filename) ? ROM_GIDE :
              0 == strcmp(CARTNAME_BEERIDE,     filename) ? ROM_BEERIDE :
              0 == strcmp(CARTNAME_NMS1210,     filename) ? ROM_NMS1210 :
              0 == strcmp(CARTNAME_GOUDASCSI,   filename) ? ROM_GOUDASCSI :
              0 == strcmp(CARTNAME_SONYHBI55,   filename) ? ROM_SONYHBI55 :
              0 == strcmp(CARTNAME_EXTRAM16KB,  filename) ? ROM_EXTRAM16KB :
              0 == strcmp(CARTNAME_EXTRAM32KB,  filename) ? ROM_EXTRAM32KB :
              0 == strcmp(CARTNAME_EXTRAM48KB,  filename) ? ROM_EXTRAM48KB :
              0 == strcmp(CARTNAME_EXTRAM64KB,  filename) ? ROM_EXTRAM64KB :
              0 == strcmp(CARTNAME_EXTRAM16KB,  filename) ? ROM_EXTRAM16KB :
              0 == strcmp(CARTNAME_EXTRAM32KB,  filename) ? ROM_EXTRAM32KB :
              0 == strcmp(CARTNAME_EXTRAM48KB,  filename) ? ROM_EXTRAM48KB :
              0 == strcmp(CARTNAME_EXTRAM64KB,  filename) ? ROM_EXTRAM64KB :
              0 == strcmp(CARTNAME_EXTRAM512KB, filename) ? ROM_EXTRAM512KB :
              0 == strcmp(CARTNAME_EXTRAM1MB,   filename) ? ROM_EXTRAM1MB :
              0 == strcmp(CARTNAME_EXTRAM2MB,   filename) ? ROM_EXTRAM2MB :
              0 == strcmp(CARTNAME_EXTRAM4MB,   filename) ? ROM_EXTRAM4MB :
              0 == strcmp(CARTNAME_MEGARAM128,  filename) ? ROM_MEGARAM128 :
              0 == strcmp(CARTNAME_MEGARAM256,  filename) ? ROM_MEGARAM256 :
              0 == strcmp(CARTNAME_MEGARAM512,  filename) ? ROM_MEGARAM512 :
              0 == strcmp(CARTNAME_MEGARAM768,  filename) ? ROM_MEGARAM768 :
              0 == strcmp(CARTNAME_MEGARAM2M,   filename) ? ROM_MEGARAM2M  :
              0 == strcmp(CARTNAME_MEGASCSI128, filename) ? SRAM_MEGASCSI128 :
              0 == strcmp(CARTNAME_MEGASCSI256, filename) ? SRAM_MEGASCSI256 :
              0 == strcmp(CARTNAME_MEGASCSI512, filename) ? SRAM_MEGASCSI512 :
              0 == strcmp(CARTNAME_MEGASCSI1MB, filename) ? SRAM_MEGASCSI1MB :
              0 == strcmp(CARTNAME_NOWINDDOS1,  filename) ? ROM_NOWIND :
              0 == strcmp(CARTNAME_NOWINDDOS2,  filename) ? ROM_NOWIND :
              0 == strcmp(CARTNAME_ESERAM128,   filename) ? SRAM_ESERAM128 :
              0 == strcmp(CARTNAME_ESERAM256,   filename) ? SRAM_ESERAM256 :
              0 == strcmp(CARTNAME_ESERAM512,   filename) ? SRAM_ESERAM512 :
              0 == strcmp(CARTNAME_ESERAM1MB,   filename) ? SRAM_ESERAM1MB :
              0 == strcmp(CARTNAME_MEGAFLSHSCC, filename) ? ROM_MEGAFLSHSCC :
              0 == strcmp(CARTNAME_WAVESCSI128, filename) ? SRAM_WAVESCSI128 :
              0 == strcmp(CARTNAME_WAVESCSI256, filename) ? SRAM_WAVESCSI256 :
              0 == strcmp(CARTNAME_WAVESCSI512, filename) ? SRAM_WAVESCSI512 :
              0 == strcmp(CARTNAME_WAVESCSI1MB, filename) ? SRAM_WAVESCSI1MB :
              0 == strcmp(CARTNAME_ESESCC128,   filename) ? SRAM_ESESCC128 :
              0 == strcmp(CARTNAME_ESESCC256,   filename) ? SRAM_ESESCC256 :
              0 == strcmp(CARTNAME_ESESCC512,   filename) ? SRAM_ESESCC512 :
              romType;

    if (drive == 0) {
        strcpy(properties->media.carts[0].fileName, filename);
        strcpy(properties->media.carts[0].fileNameInZip, romName);
        properties->media.carts[0].type = romType;
#ifndef NO_FILE_HISTORY
        updateFileHistory(*properties->filehistory.cartridge[0], 
                          properties->filehistory.cartridgeType[0], 
                          filename, romType);
#endif
    }
    else {
        strcpy(properties->media.carts[1].fileName, filename);
        strcpy(properties->media.carts[1].fileNameInZip, romName);
        properties->media.carts[1].type = romType;
#ifndef NO_FILE_HISTORY
        updateFileHistory(*properties->filehistory.cartridge[1], 
                          properties->filehistory.cartridgeType[1], 
                          filename, romType);
#endif
    }

    if (autostart && !noautostart) {
        emulatorStop();
        emulatorStart(NULL);
    }
    else if (emulatorGetState() != EMU_STOPPED) {
        emulatorSuspend();
        boardChangeCartridge(drive, romType, filename, isZip ? romName : NULL);
        emulatorResume();
    }
    else {
        boardChangeCartridge(drive, romType, filename, isZip ? romName : NULL);
    }

    return 1;
}

int insertDiskette(Properties* properties, int drive, const char* fname, const char* inZipFile, int forceAutostart) {
    char diskName[512] = "";
    char filename[512] = "";
    int autostart = forceAutostart == 1 || (drive == 0 ? properties->diskdrive.autostartA : 0);
    int noautostart = forceAutostart == -1;
    int isZip = isFileExtension(fname, ".zip");

    if (fname) strcpy(filename, fname);

    emulatorResetMixer();

    if (isZip) {
        if (inZipFile != NULL) {
            strcpy(diskName, inZipFile);
        }
        else {
            // This is really ugly and should be done nicer...
            // The idea is to get files of three types and merge them into
            // one list. Maybe it should be done in zipGetFileList?
            int i;
            char* fileList;
            int countDsk;
            int countDi1;
            int countDi2;
            int count360;
            int count720;
            int countSf7;
            char* fileListDsk = zipGetFileList(filename, ".dsk", &countDsk);
            char* fileListDi1 = zipGetFileList(filename, ".di1", &countDi1);
            char* fileListDi2 = zipGetFileList(filename, ".di2", &countDi2);
            char* fileList360 = zipGetFileList(filename, ".360", &count360);
            char* fileList720 = zipGetFileList(filename, ".720", &count720);
            char* fileListSf7 = zipGetFileList(filename, ".Sf7", &countSf7);
            int count = countDsk + countDi1 + countDi2 + count360 + count720 + countSf7;
            int sizeDsk = 0;
            int sizeDi1 = 0;
            int sizeDi2 = 0;
            int size360 = 0;
            int size720 = 0;
            int sizeSf7 = 0;

            for (i = 0; i < countDsk; i++) {
                sizeDsk += strlen(fileListDsk + sizeDsk) + 1;
            }
            for (i = 0; i < countDi1; i++) {
                sizeDi1 += strlen(fileListDi1 + sizeDi1) + 1;
            }
            for (i = 0; i < countDi2; i++) {
                sizeDi2 += strlen(fileListDi2 + sizeDi2) + 1;
            }
            for (i = 0; i < count360; i++) {
                sizeDi2 += strlen(fileList360 + size360) + 1;
            }
            for (i = 0; i < count720; i++) {
                size720 += strlen(fileList720 + size720) + 1;
            }
            for (i = 0; i < countSf7; i++) {
                sizeSf7 += strlen(fileListSf7 + sizeSf7) + 1;
            }

            fileList = malloc(sizeDsk + sizeDi1 + sizeDi2 + size360 + size720 + sizeSf7);
            memcpy(fileList, fileListDsk, sizeDsk);
            memcpy(fileList + sizeDsk, fileListDi1, sizeDi1);
            memcpy(fileList + sizeDsk + sizeDi1, fileListDi2, sizeDi2);
            memcpy(fileList + sizeDsk + sizeDi1 + sizeDi2, fileList360, size360);
            memcpy(fileList + sizeDsk + sizeDi1 + sizeDi2 + size360, fileList720, size720);
            memcpy(fileList + sizeDsk + sizeDi1 + sizeDi2 + size360 + size720, fileListSf7, sizeSf7);

            if (count == 0) {
                archShowNoDiskInZipDialog();
                return 0;
            }

            if (count == 1) {
                strcpy(diskName, fileList);
            }
            else {
                char* filename = archFilenameGetOpenDiskZip(properties, drive, fname, fileList, count, &autostart);
                if (filename == NULL) {
                    free(fileList);
                    return 0;
                }
                strcpy(diskName, filename);
            }

            if(fileListDsk) free(fileListDsk);
            if(fileListDi1) free(fileListDi1);
            if(fileListDi2) free(fileListDi2);
            if(fileList360) free(fileList360);
            if(fileList720) free(fileList720);
            if(fileListSf7) free(fileListSf7);
            free(fileList);
        }
    }

    strcpy(properties->media.disks[drive].fileName, filename);
    strcpy(properties->media.disks[drive].fileNameInZip, diskName);
    updateExtendedDiskName(drive, properties->media.disks[drive].fileName, properties->media.disks[drive].fileNameInZip);
#ifndef NO_FILE_HISTORY
    if (drive < 2) {
        updateFileHistory(*properties->filehistory.diskdrive[drive], NULL, filename, 0);
    }
#endif

    if (autostart && !noautostart) {
        emulatorStop();
        emulatorStart(NULL);
    }
    else if (emulatorGetState() != EMU_STOPPED) {
        emulatorSuspend();
        boardChangeDiskette(drive, filename, isZip ? diskName : NULL);
        emulatorResume();
    }

    return 1;
}

int insertCassette(Properties* properties, int drive, const char* fname, const char* inZipFile, int forceAutostart) {
    int autostart = forceAutostart == 1;
    int noautostart = forceAutostart == -1;
    char tapeName[512] = "";
    char filename[512] = "";
    int isZip = isFileExtension(fname, ".zip");

    if (fname) strcpy(filename, fname);

    if (isZip) {
        if (inZipFile != NULL) {
            strcpy(tapeName, inZipFile);
        }
        else {
            int count;
            char* fileList = zipGetFileList(filename, ".cas", &count);

            if (fileList == NULL) {
                archShowNoCasInZipDialog();
                return 0;
            }

            if (count == 1) {
                strcpy(tapeName, fileList);
            }
            else {
                char* filename = archFilenameGetOpenCasZip(properties, fname, fileList, count, &autostart);
                if (filename == NULL) {
                    free(fileList);
                    return 0;
                }
                strcpy(tapeName, filename);
            }
            free(fileList);
        }
    }

    strcpy(properties->media.tapes[0].fileName, filename);
    strcpy(properties->media.tapes[0].fileNameInZip, tapeName);
    updateExtendedCasName(0, properties->media.tapes[0].fileName, properties->media.tapes[0].fileNameInZip);
#ifndef NO_FILE_HISTORY
    updateFileHistory(*properties->filehistory.cassette[0], NULL, filename, 0);
#endif

    if (autostart && !noautostart) {
        emulatorStart(NULL);
    }
    else if (emulatorGetState() != EMU_STOPPED) {
        emulatorSuspend();
        boardChangeCassette(0, filename, isZip ? tapeName : NULL);
        emulatorResume();
    }

    return 1;
}

static int insertDisketteOrCartridge(Properties* properties, int drive, const char* fname, int forceAutostart) 
{
    int countDsx;
    int countDi1;
    int countDi2;
    int count360;
    int count720;
    int countSf7;
    int countRox;
    int countRi;
    int countMx1;
    int countMx2;
    int countCol;
    int countSg;
    int countSc;
    int countCas;
    char* fileListDsk = NULL;
    char* fileListRom = NULL;
    char* fileListDsx = zipGetFileList(fname, ".dsk", &countDsx);
    char* fileListDi1 = zipGetFileList(fname, ".di1", &countDi1);
    char* fileListDi2 = zipGetFileList(fname, ".di2", &countDi2);
    char* fileList360 = zipGetFileList(fname, ".360", &count360);
    char* fileList720 = zipGetFileList(fname, ".720", &count720);
    char* fileListSf7 = zipGetFileList(fname, ".sf7", &countSf7);
    char* fileListRox = zipGetFileList(fname, ".rom", &countRox);
    char* fileListRi  = zipGetFileList(fname, ".ri",  &countRi);
    char* fileListMx1 = zipGetFileList(fname, ".mx1", &countMx1);
    char* fileListMx2 = zipGetFileList(fname, ".mx2", &countMx2);
    char* fileListCol = zipGetFileList(fname, ".col", &countCol);
    char* fileListSg  = zipGetFileList(fname, ".sg",  &countSg);
    char* fileListSc  = zipGetFileList(fname, ".sc",  &countSc);
    char* fileListCas = zipGetFileList(fname, ".cas", &countCas);
    int countRom = countRox + countRi + countMx1 + countMx2 + countCol + countSg + countSc;
    int countDsk = countDsx + countDi1 + countDi2 + count360 + count720 + countSf7;
    char* fileList;
    int sizeDsk = 0;
    int sizeDsx = 0;
    int sizeDi1 = 0;
    int sizeDi2 = 0;
    int size360 = 0;
    int size720 = 0;
    int sizeSf7 = 0;
    int sizeRox = 0;
    int sizeRi  = 0;
    int sizeRom = 0;
    int sizeMx1 = 0;
    int sizeMx2 = 0;
    int sizeCol = 0;
    int sizeSg  = 0;
    int sizeSc  = 0;
    int sizeCas = 0;
    int success = 0;
    const char* filename;
    int autostart;
    int romType;
    int i;

    // First merge different dsk formats into one list
    for (i = 0; i < countDsx; i++) {
        sizeDsx += strlen(fileListDsx + sizeDsx) + 1;
    }
    for (i = 0; i < countDi1; i++) {
        sizeDi1 += strlen(fileListDi1 + sizeDi1) + 1;
    }
    for (i = 0; i < countDi2; i++) {
        sizeDi2 += strlen(fileListDi2 + sizeDi2) + 1;
    }
    for (i = 0; i < count360; i++) {
        sizeDi2 += strlen(fileList360 + size360) + 1;
    }
    for (i = 0; i < count720; i++) {
        size720 += strlen(fileList720 + size720) + 1;
    }
    for (i = 0; i < countSf7; i++) {
        sizeSf7 += strlen(fileListSf7 + sizeSf7) + 1;
    }

    if (countDsk > 0) {
        fileListDsk = malloc(sizeDsx + sizeDi1 + sizeDi2);
        memcpy(fileListDsk, fileListDsx, sizeDsx);
        memcpy(fileListDsk + sizeDsx, fileListDi1, sizeDi1);
        memcpy(fileListDsk + sizeDsx + sizeDi1, fileListDi2, sizeDi2);
        memcpy(fileListDsk + sizeDsx + sizeDi1 + sizeDi2, fileList360, size360);
        memcpy(fileListDsk + sizeDsx + sizeDi1 + sizeDi2 + size360, fileList720, size720);
        memcpy(fileListDsk + sizeDsx + sizeDi1 + sizeDi2 + size360 + size720, fileListSf7, sizeSf7);
    }

    // Then merge different dsk formats into one list
    for (i = 0; i < countRox; i++) {
        sizeRox += strlen(fileListRox + sizeRox) + 1;
    }
    for (i = 0; i < countRi; i++) {
        sizeRi += strlen(fileListRi + sizeRi) + 1;
    }
    for (i = 0; i < countMx1; i++) {
        sizeMx1 += strlen(fileListMx1 + sizeMx1) + 1;
    }
    for (i = 0; i < countMx2; i++) {
        sizeMx2 += strlen(fileListMx2 + sizeMx2) + 1;
    }
    for (i = 0; i < countCol; i++) {
        sizeCol += strlen(fileListCol + sizeCol) + 1;
    }
    for (i = 0; i < countSg; i++) {
        sizeSg += strlen(fileListSg + sizeSg) + 1;
    }
    for (i = 0; i < countSc; i++) {
        sizeSc += strlen(fileListSc + sizeSc) + 1;
    }

    if (countRom > 0) {
        fileListRom = malloc(sizeRox + sizeMx1 + sizeMx2);
        memcpy(fileListRom, fileListRox, sizeRox);
        memcpy(fileListRom + sizeRox, fileListMx1, sizeMx1);
        memcpy(fileListRom + sizeRox + sizeMx1, fileListMx2, sizeMx2);
        memcpy(fileListRom + sizeRox + sizeMx1 + sizeMx2, fileListCol, sizeCol);
        memcpy(fileListRom + sizeRox + sizeMx1 + sizeMx2 + sizeCol, fileListRi, sizeRi);
        memcpy(fileListRom + sizeRox + sizeMx1 + sizeMx2 + sizeCol + sizeRi, fileListSg, sizeSg);
        memcpy(fileListRom + sizeRox + sizeMx1 + sizeMx2 + sizeCol + sizeRi + sizeSg, fileListSc, sizeSc);
    }

    // Finally check different types...
    if (fileListDsk == NULL && fileListRom == NULL && fileListCas == NULL) {
        return 0;
    }

    if (fileListDsk == NULL && fileListCas == NULL) {
        free(fileListRom);
        return insertCartridge(properties, drive, fname, NULL, ROM_UNKNOWN, 0);
    }

    if (fileListRom == NULL && fileListCas == NULL) {
        free(fileListDsk);
        return insertDiskette(properties, drive, fname, NULL, 0);
    }

    if (fileListRom == NULL && fileListDsk == NULL) {
        free(fileListCas);
        return insertCassette(properties, 0, fname, NULL, 0);
    }

    for (i = 0; i < countRom; i++) {
        sizeRom += strlen(fileListRom + sizeRom) + 1;
    }

    for (i = 0; i < countDsk; i++) {
        sizeDsk += strlen(fileListDsk + sizeDsk) + 1;
    }

    for (i = 0; i < countCas; i++) {
        sizeCas += strlen(fileListCas + sizeCas) + 1;
    }

    fileList = malloc(sizeDsk + sizeRom + sizeCas);
    memcpy(fileList, fileListRom, sizeRom);
    memcpy(fileList + sizeRom, fileListDsk, sizeDsk);
    memcpy(fileList + sizeRom + sizeDsk, fileListCas, sizeCas);

    autostart = forceAutostart;

    filename = archFilenameGetOpenAnyZip(properties, fname, fileList, countDsk + countRom + countCas, &autostart, &romType);
    if (filename != NULL) {
        if (isFileExtension(filename, ".rom") || isFileExtension(filename, ".ri") || 
            isFileExtension(filename, ".mx1") || isFileExtension(filename, ".mx2") || 
            isFileExtension(filename, ".col") || 
            isFileExtension(filename, ".sg") || isFileExtension(filename, ".sc")) {
            success = insertCartridge(properties, drive, fname, filename, romType, autostart);
        }
        else if (isFileExtension(filename, ".dsk") || 
                isFileExtension(filename, ".di1") || isFileExtension(filename, ".di2") || 
                isFileExtension(filename, ".360") || isFileExtension(filename, ".720") || 
                isFileExtension(filename, ".sf7")) 
        {
            success = insertDiskette(properties, drive, fname, filename, autostart);
        }
        else if (isFileExtension(filename, ".cas")) {
            success = insertCassette(properties, 0, fname, filename, autostart);
        }
    }

    if(fileListDsk) free(fileListDsk);
    if(fileListDi1) free(fileListDi1);
    if(fileListDi2) free(fileListDi2);
    if(fileList360) free(fileList360);
    if(fileList720) free(fileList720);
    if(fileListSf7) free(fileListSf7);
    if(fileListRom) free(fileListRom);
    if(fileListRox) free(fileListRox);
    if(fileListRi)  free(fileListRi);
    if(fileListMx1) free(fileListMx1);
    if(fileListMx2) free(fileListMx2);
    if(fileListCol) free(fileListCol);
    if(fileListSg)  free(fileListSg);
    if(fileListSc)  free(fileListSc);
    if(fileListCas) free(fileListCas);
    if(fileList) free(fileList);

    return success;
}

int tryLaunchUnknownFile(Properties* properties, const char* fileName, int forceAutostart) 
{
    int rv = 0;

    if (isFileExtension(fileName, ".sta")) {
        emulatorStart(fileName);
        return 1;
    }

    if (isFileExtension(fileName, ".rom") || isFileExtension(fileName, ".ri") || 
        isFileExtension(fileName, ".mx1") || isFileExtension(fileName, ".mx2") || 
        isFileExtension(fileName, ".col") || 
        isFileExtension(fileName, ".sg") || isFileExtension(fileName, ".sc")) 
    {
        rv = insertCartridge(properties, properties->cartridge.quickStartDrive, fileName, NULL, ROM_UNKNOWN, forceAutostart);
    }
    else if (isFileExtension(fileName, ".dsk") || 
             isFileExtension(fileName, ".di1") || isFileExtension(fileName, ".di2") || 
             isFileExtension(fileName, ".360") || isFileExtension(fileName, ".720") || 
             isFileExtension(fileName, ".sf7")) 
    {
        int drive = properties->diskdrive.quickStartDrive;
        FILE* f = fopen(fileName, "rb");
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            if (ftell(f) >= 1024 * 1024) {
                drive = diskGetHdDriveId(0, 0);
            }
            fclose(f);
        }

        rv = insertDiskette(properties, drive, fileName, NULL, forceAutostart);
    }
    else if (isFileExtension(fileName, ".cas")) {
        if (properties->cassette.rewindAfterInsert) tapeRewindNextInsert();
        rv = insertCassette(properties, 0, fileName, NULL, forceAutostart);
    }
    else if (isFileExtension(fileName, ".zip")) {
        rv = insertDisketteOrCartridge(properties, properties->cartridge.quickStartDrive, fileName, forceAutostart);
    }
    
    archUpdateMenu(0);

    return rv;
}

