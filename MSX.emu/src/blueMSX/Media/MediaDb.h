/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Media/MediaDb.h,v $
**
** $Revision: 1.51 $
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
#ifndef MEDIA_DB_H
#define MEDIA_DB_H

typedef int RomType;
enum  {
    ROM_UNKNOWN     = 0,
    ROM_STANDARD    = 1,
    ROM_MSXDOS2     = 2,
    ROM_KONAMI5     = 3,
    ROM_KONAMI4     = 4,
    ROM_ASCII8      = 5,
    ROM_ASCII16     = 6,
    ROM_GAMEMASTER2 = 7,
    ROM_ASCII8SRAM  = 8,
    ROM_ASCII16SRAM = 9,
    ROM_RTYPE       = 10,
    ROM_CROSSBLAIM  = 11,
    ROM_HARRYFOX    = 12,
    ROM_KOREAN80    = 13,
    ROM_KOREAN126   = 14,
    ROM_SCCEXTENDED = 15,
    ROM_FMPAC       = 16,
    ROM_KONAMI4NF   = 17,
    ROM_ASCII16NF   = 18,
    ROM_PLAIN       = 19,
    ROM_NORMAL      = 20,
    ROM_DISKPATCH   = 21,
    RAM_MAPPER      = 22,
    RAM_NORMAL      = 23,
    ROM_KANJI       = 24,
    ROM_HOLYQURAN   = 25,
    SRAM_MATSUCHITA = 26,
    ROM_PANASONIC16 = 27,
    ROM_BUNSETU     = 28,
    ROM_JISYO       = 29,
    ROM_KANJI12     = 30,
    ROM_NATIONAL    = 31,
    SRAM_S1985      = 32,
    ROM_F4DEVICE    = 33,
    ROM_F4INVERTED  = 34,
    ROM_KOEI        = 38,
    ROM_BASIC       = 39,
    ROM_HALNOTE     = 40,
    ROM_LODERUNNER  = 41,
    ROM_0x4000      = 42,
    ROM_PAC         = 43,
    ROM_MEGARAM     = 44,
    ROM_MEGARAM128  = 45,
    ROM_MEGARAM256  = 46,
    ROM_MEGARAM512  = 47,
    ROM_MEGARAM768  = 48,
    ROM_MEGARAM2M   = 49,
    ROM_MSXAUDIO    = 50,
    ROM_KOREAN90    = 51,
    ROM_SNATCHER    = 52,
    ROM_SDSNATCHER  = 53,
    ROM_SCCMIRRORED = 54,
    ROM_SCC         = 55,
    ROM_SCCPLUS     = 56,
    ROM_TC8566AF    = 57,
    ROM_S1990       = 58,
    ROM_TURBORTIMER = 59,
    ROM_TURBORPCM   = 60,
    ROM_KONAMISYNTH = 61,
    ROM_MAJUTSUSHI  = 62,
    ROM_MICROSOL    = 63,
    ROM_NATIONALFDC = 64,
    ROM_PHILIPSFDC  = 65,
    ROM_CASPATCH    = 66,
    ROM_SVI738FDC   = 67,
    ROM_PANASONIC32 = 68,
    ROM_EXTRAM      = 69,
    ROM_EXTRAM512KB = 70,
    ROM_EXTRAM1MB   = 71,
    ROM_EXTRAM2MB   = 72,
    ROM_EXTRAM4MB   = 73,
    ROM_SVI328      = 74,
    ROM_SVI328FDC   = 75,
    ROM_COLECO      = 76,
    ROM_SONYHBI55   = 77,
    ROM_MSXMUSIC    = 78,
    ROM_MOONSOUND   = 79,
    ROM_MSXAUDIODEV = 80,
    ROM_V9958       = 81,
    ROM_SVI80COL    = 82,
    ROM_SVI328PRN   = 83,
    ROM_MSXPRN      = 84,
    ROM_SVI328RS232 = 85,
    ROM_0xC000      = 86,
    ROM_FMPAK       = 87,
    ROM_MSXMIDI     = 88,
    ROM_MSXRS232    = 89,
    ROM_TURBORIO    = 90,
    ROM_KONAMKBDMAS = 91,
    ROM_GAMEREADER  = 92,
    RAM_1KB_MIRRORED= 93,
    ROM_SG1000      = 94,
    ROM_SG1000CASTLE= 95,
    ROM_SUNRISEIDE  = 96,
    ROM_GIDE        = 97,
    ROM_BEERIDE     = 98,
    ROM_KONWORDPRO  = 99,
    ROM_MICROSOL80  = 100,
    ROM_NMS8280DIGI = 101,
    ROM_SONYHBIV1   = 102,
    ROM_SVI727      = 103,
    ROM_FMDAS       = 104,
    ROM_YAMAHASFG05 = 105,
    ROM_YAMAHASFG01 = 106,
    ROM_SF7000IPL   = 107,
    ROM_SC3000      = 108,
    ROM_PLAYBALL    = 109,
    ROM_OBSONET     = 110,
    RAM_2KB_MIRRORED= 111,
    ROM_SEGABASIC   = 112,
    ROM_CVMEGACART  = 113,
    ROM_DUMAS       = 114,
    SRAM_MEGASCSI   = 115,
    SRAM_MEGASCSI128= 116,
    SRAM_MEGASCSI256= 117,
    SRAM_MEGASCSI512= 118,
    SRAM_MEGASCSI1MB= 119,
    SRAM_ESERAM     = 120,
    SRAM_ESERAM128  = 121,
    SRAM_ESERAM256  = 122,
    SRAM_ESERAM512  = 123,
    SRAM_ESERAM1MB  = 124,
    SRAM_ESESCC     = 125,
    SRAM_ESESCC128  = 126,
    SRAM_ESESCC256  = 127,
    SRAM_ESESCC512  = 128,
    SRAM_WAVESCSI   = 129,
    SRAM_WAVESCSI128= 130,
    SRAM_WAVESCSI256= 131,
    SRAM_WAVESCSI512= 132,
    SRAM_WAVESCSI1MB= 133,
    ROM_NOWIND      = 134,
    ROM_GOUDASCSI   = 135,
    ROM_MANBOW2     = 136,
    ROM_MEGAFLSHSCC = 137,
    ROM_FORTEII     = 138,
    ROM_PANASONIC8  = 139,
    ROM_FSA1FMMODEM = 140,
    ROM_DRAM        = 141,
    ROM_PANASONICWX16=142,
    ROM_TC8566AF_TR = 143,
    ROM_MATRAINK    = 144,
    ROM_NETTOUYAKYUU= 145,
    ROM_YAMAHANET   = 146,
    ROM_JOYREXPSG   = 147,
    ROM_OPCODEPSG   = 148,
    ROM_EXTRAM16KB  = 149,
    ROM_EXTRAM32KB  = 150,
    ROM_EXTRAM48KB  = 151,
    ROM_EXTRAM64KB  = 152,
    ROM_NMS1210     = 153,
    ROM_ARC         = 154,
    ROM_OPCODEBIOS  = 155,
    ROM_OPCODESLOT  = 156,
    ROM_OPCODESAVE  = 157,
    ROM_OPCODEMEGA  = 158,
    SRAM_MATSUCHITA_INV = 159,
    ROM_SVI328RSIDE = 160,
    ROM_MAXROMID    = 160
};

typedef struct MediaType MediaType;
typedef struct MediaDb MediaDb;

void mediaDbAddFromXmlFile(const char* fileName);

MediaType* mediaDbLookup(MediaDb* mediaDb, const void *buffer, int size);


void mediaDbLoad(const char* directory);

void mediaDbCreateRomdb();
void mediaDbCreateDiskdb();
void mediaDbCreateCasdb();


MediaType* mediaDbLookupRom(const void *buffer, int size);
MediaType* mediaDbGuessRom(const void *buffer, int size);
MediaType* mediaDbLookupDisk(const void *buffer, int size);
MediaType* mediaDbLookupCas(const void *buffer, int size);

RomType     mediaDbGetRomType(MediaType* mediaType);
const char* mediaDbGetTitle(MediaType* mediaType);
const char* mediaDbGetYear(MediaType* mediaType);
const char* mediaDbGetCompany(MediaType* mediaType);
const char* mediaDbGetRemark(MediaType* mediaType);
const char* mediaDbGetPrettyString(MediaType* mediaType);

void mediaDbSetDefaultRomType(RomType romType);
RomType mediaDbStringToType(const char* romName);
const char* romTypeToString(RomType romType);
const char* romTypeToShortString(RomType romType);

int romTypeIsRom(RomType romType);
int romTypeIsMegaRom(RomType romType);
int romTypeIsMegaRam(RomType romType);
int romTypeIsFmPac(RomType romType);

#endif /*MEDIA_DB_H*/
