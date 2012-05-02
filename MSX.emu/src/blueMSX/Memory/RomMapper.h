/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/RomMapper.h,v $
**
** $Revision: 1.18 $
**
** $Date: 2008-12-22 21:33:59 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2004 Daniel Vik
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
#ifndef ROM_MAPPER_H
#define ROM_MAPPER_H


#include "MediaDb.h"


#ifndef WII
const char* romTypeToString(RomType romType) {
    switch (romType) {    
    case ROM_STANDARD:    return "Standard";
    case ROM_MSXDOS2:     return "MSXDOS 2";
    case ROM_KONAMI5:     return "Konami 5";
    case ROM_KONAMI4:     return "Konami 4";
    case ROM_ASCII8:      return "ASCII 8";
    case ROM_ASCII16:     return "ASCII 16";
    case ROM_GAMEMASTER2: return "Game Master 2 (SRAM)";
    case ROM_ASCII8SRAM:  return "ASCII 8 (SRAM)";
    case ROM_ASCII16SRAM: return "ASCII 16 (SRAM)";
    case ROM_RTYPE:       return "R-Type";
    case ROM_CROSSBLAIM:  return "Cross Blaim";
    case ROM_HARRYFOX:    return "Harry Fox";
    case ROM_MAJUTSUSHI:  return "Konami Majutsushi";
    case ROM_KOREAN80:    return "Korean 80 in 1";
    case ROM_KOREAN126:   return "Korean 126 in 1";
    case ROM_SCC:         return "SCC";
    case ROM_SCCPLUS:     return "SCC+";
    case ROM_SNATCHER:    return "The Snatcher";
    case ROM_SDSNATCHER:  return "SD Snatcher";
    case ROM_SCCMIRRORED: return "SCC mirrored";
    case ROM_SCCEXTENDED: return "SCC extended";
    case ROM_FMPAC:       return "FMPAC (SRAM)";
    case ROM_KONAMI4NF:   return "Konami4 alt.";
    case ROM_ASCII16NF:   return "ASCII 16 alt.";
    case ROM_PLAIN:       return "Mirrored ROM";
    case ROM_NORMAL:      return "Normal ROM";
    case ROM_DISKPATCH:   return "Normal + Disk Patch";
    case ROM_CASPATCH:    return "Normal + Cassette Patch";
    case ROM_TC8566AF:    return "TC8566AF Disk Controller";
    case ROM_MICROSOL:    return "Microsol Disk Controller";
    case ROM_NATIONALFDC: return "National Disk Controller";
    case ROM_PHILIPSFDC:  return "Philips Disk Controller";
    case ROM_SVI738FDC:   return "SVI-738 Disk Controller";
    case RAM_MAPPER:      return "Mapped RAM";
    case RAM_NORMAL:      return "Normal RAM";
    case ROM_KANJI:       return "Kanji";
    case ROM_HOLYQURAN:   return "Holy Quran";
    case SRAM_MATSUCHITA: return "Matsushita SRAM";
    case ROM_PANASONIC16: return "Panasonic 16kB SRAM";
    case ROM_PANASONIC32: return "Panasonic 32kB SRAM";
    case ROM_BUNSETU:     return "Bunsetsu";
    case ROM_JISYO:       return "Jisyo";
    case ROM_KANJI12:     return "Kanji12";
    case ROM_NATIONAL:    return "National (SRAM)";
    case SRAM_S1985:      return "S1985";
    case ROM_S1990:       return "S1990";
    case ROM_F4DEVICE:    return "F4 Device Normal";
    case ROM_F4INVERTED:  return "F4 Device Inverted";
    case ROM_TURBORTIMER: return "Turbo-R Timer";
	/*
    case AUDIO_MOONSOUND: return "Moonsound Audio";
    case AUDIO_Y8950:     return "Y8950 Audio";
    case AUDIO_YM2413:    return "YM2413 Audio";
	*/
    case ROM_KOEI:        return "Koei (SRAM)";
    case ROM_BASIC:       return "Basic ROM";
    case ROM_HALNOTE:     return "Halnote";
    case ROM_LODERUNNER:  return "Lode Runner";
    case ROM_0x4000:      return "Normal 0x4000";
    case ROM_KONAMISYNTH: return "Konami Synthesizer";
    case ROM_PAC:         return "PAC (SRAM)";
    case ROM_MEGARAM:     return "MegaRAM";
    case ROM_MEGARAM128:  return "128kB MegaRAM";
    case ROM_MEGARAM256:  return "256kB MegaRAM";
    case ROM_MEGARAM512:  return "512kB MegaRAM";
    case ROM_MEGARAM768:  return "768kB MegaRAM";
    case ROM_MEGARAM2M:   return "2MB MegaRAM";
    case ROM_EXTRAM:      return "External RAM";
    case ROM_EXTRAM512KB: return "512kB External RAM";
    case ROM_EXTRAM1MB:   return "1MB External RAM";
    case ROM_EXTRAM2MB:   return "2MB External RAM";
    case ROM_EXTRAM4MB:   return "4MB External RAM";
    case ROM_MSXMUSIC:    return "MSX Music";
    case ROM_MSXAUDIO:    return "MSX Audio";
    case ROM_MSXRS232:    return "MSX RS-232";
    case ROM_MOONSOUND:   return "Moonsound";
    case ROM_KOREAN90:    return "Korean 90 in 1";
    case ROM_SVI328:      return "SVI-328 Cartridge";
    case ROM_SVI328FDC:   return "SVI-328 Disk Controller";
    case ROM_SVI328PRN:   return "SVI-328 Printer";
    case ROM_SVI328RS232: return "SVI-328 Serial Port";
    case ROM_SVI80COL:    return "SVI 80 Column Card";
    case ROM_SVI328RSIDE: return "SVI-328 RS IDE";
    case ROM_COLECO:      return "Coleco Cartridge";
    case ROM_SONYHBI55:   return "Sony HBI-55";
    case ROM_MSXAUDIODEV: return "MSX Audio Chip";
    case ROM_MSXPRN:      return "MSX Printer";
    case ROM_TURBORPCM:   return "Turbo-R PCM Chip";

    case ROM_UNKNOWN:     return "Unknown";
    }

    return "unknown";
}

const char* romTypeToShortString(RomType romType) {
    switch (romType) {    
    case ROM_STANDARD:    return "STANDARD";
    case ROM_MSXDOS2:     return "MSXDOS2";
    case ROM_KONAMI5:     return "KONAMI5";
    case ROM_KONAMI4:     return "KONAMI4";
    case ROM_ASCII8:      return "ASCII8";
    case ROM_ASCII16:     return "ASCII16";
    case ROM_GAMEMASTER2: return "GMASTER2";
    case ROM_ASCII8SRAM:  return "ASCII8SRAM";
    case ROM_ASCII16SRAM: return "ASCII16SRAM";
    case ROM_RTYPE:       return "R-TYPE";
    case ROM_CROSSBLAIM:  return "CROSSBLAIM";
    case ROM_HARRYFOX:    return "HARRYFOX";
    case ROM_KOREAN80:    return "80IN1";
    case ROM_KOREAN126:   return "126IN1";
    case ROM_SCC:         return "SCC";
    case ROM_SCCPLUS:     return "SCC+";
    case ROM_SNATCHER:    return "SNATCHER";
    case ROM_SDSNATCHER:  return "SDSNATCHER";
    case ROM_SCCMIRRORED: return "SCCMIRRORED";
    case ROM_SCCEXTENDED: return "SCCEXTENDED";
    case ROM_FMPAC:       return "FMPAC";
    case ROM_KONAMI4NF:   return "KONAMI4ALT";
    case ROM_ASCII16NF:   return "ASCII16ALT";
    case ROM_PLAIN:       return "MIRRORED";
    case ROM_NORMAL:      return "NORMAL";
    case ROM_DISKPATCH:   return "DISKPATCH";
    case ROM_CASPATCH:    return "CASPATCH";
    case ROM_TC8566AF:    return "TC8566AF";
    case ROM_MICROSOL:    return "MICROSOL";
    case ROM_NATIONALFDC: return "NATNL FDC";
    case ROM_PHILIPSFDC:  return "PHILIPSFDC";
    case ROM_SVI738FDC:   return "SVI738 FDC";
    case RAM_MAPPER:      return "MAPPED RAM";
    case RAM_NORMAL:      return "NORMAL RAM";
    case ROM_KANJI:       return "KANJI";
    case ROM_HOLYQURAN:   return "HOLYQURAN";
    case SRAM_MATSUCHITA: return "MATSUSHITA";
    case ROM_PANASONIC16: return "PANASON 16";
    case ROM_PANASONIC32: return "PANASON 32";
    case ROM_BUNSETU:     return "BUNSETSU";
    case ROM_JISYO:       return "JISYO";
    case ROM_KANJI12:     return "KANJI12";
    case ROM_NATIONAL:    return "NATIONAL";
    case SRAM_S1985:      return "S1985";
    case ROM_S1990:       return "S1990";
    case ROM_F4DEVICE:    return "F4NORMAL";
    case ROM_F4INVERTED:  return "F4INV";
    case ROM_TURBORTIMER: return "TURBORTMR";
	/*
    case AUDIO_MOONSOUND: return "MOONSOUND";
    case AUDIO_Y8950:     return "Y8950";
    case AUDIO_YM2413:    return "YM2413";
	*/
    case ROM_KOEI:        return "KOEI";
    case ROM_BASIC:       return "BASIC";
    case ROM_HALNOTE:     return "HALNOTE";
    case ROM_LODERUNNER:  return "LODERUNNER";
    case ROM_0x4000:      return "0x4000";
    case ROM_KONAMISYNTH: return "KONSYNTH";
    case ROM_MAJUTSUSHI:  return "MAJUTSUSHI";
    case ROM_PAC:         return "PAC";
    case ROM_MEGARAM:     return "MEGARAM";
    case ROM_MEGARAM128:  return "MEGARAM128";
    case ROM_MEGARAM256:  return "MEGARAM256";
    case ROM_MEGARAM512:  return "MEGARAM512";
    case ROM_MEGARAM768:  return "MEGARAM768";
    case ROM_MEGARAM2M:   return "MEGARAM2MB";
    case ROM_EXTRAM:      return "EXTERN RAM";
    case ROM_EXTRAM512KB: return "EXTRAM 512";
    case ROM_EXTRAM1MB:   return "EXTRAM 2MB";
    case ROM_EXTRAM2MB:   return "EXTRAM 1MB";
    case ROM_EXTRAM4MB:   return "EXTRAM 4MB";
    case ROM_MSXMUSIC:    return "MSXMUSIC";
    case ROM_MSXAUDIO:    return "MSXAUDIO";
    case ROM_MSXRS232:    return "MSXRS232";
    case ROM_MOONSOUND:   return "MOONSOUND";
    case ROM_KOREAN90:    return "90IN1";
    case ROM_SVI328:      return "SVI328";
    case ROM_SVI328FDC:   return "SVI328FDC";
    case ROM_SVI328PRN:   return "SVI328PRN";
    case ROM_SVI328RS232: return "SVI328RS232";
    case ROM_SVI80COL:    return "SVI80COL";
    case ROM_SVI328RSIDE: return "SVI328RSIDE";
    case ROM_COLECO:      return "COLECO";
    case ROM_SONYHBI55:   return "HBI-55";
    case ROM_MSXAUDIODEV: return "MSXAUDIO";
    case ROM_MSXPRN:      return "MSXPRN";
    case ROM_TURBORPCM:   return "TURBOR PCM";

    case ROM_UNKNOWN:     return "UNKNOWN";
    }

    return "UNKNOWN";
}
#endif

extern RomType romMapperGuessRom(const void *buffer, int size, int guess, char* extendedName);
extern RomType romMapperTypeFromString(const char* name);
extern void    romMapperSetRomdbFilename(const char* filename);
extern void    romMapperSetDiskdbFilename(const char* filename);
extern void    romMapperSetCasdbFilename(const char* filename);
extern void    romMapperSetDefaultType(RomType romType);
extern void    romMapperGetDiskInfo(const void *data, int size, char* extendedName);
extern void    romMapperGetCasInfo(const void *data, int size, char* extendedName);

#endif /*ROM_MAPPER_H*/
