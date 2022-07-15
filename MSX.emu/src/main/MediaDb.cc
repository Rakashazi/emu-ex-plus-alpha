/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Media/MediaDb.cpp,v $
**
** $Revision: 1.91 $
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

extern "C" {
#include "MsxTypes.h"
#include "MediaDb.h"
#include "Crc32Calc.h"
#include "TokenExtract.h"
#include "StrcmpNoCase.h"
#include "ArchGlob.h"
#include "Board.h"
#include "Language.h"
}

#include <string.h>
#include <imagine/util/ranges.hh>
#include <imagine/logger/logger.h>
// throw_exception.hpp, Boost 1.50
#define UUID_AA15E74A856F11E08B8D93F24824019B
#define BOOST_THROW_EXCEPTION(x)
#define BOOST_STATIC_ASSERT( ... ) static_assert(__VA_ARGS__, #__VA_ARGS__)
#include "../uuid/sha1.hpp"

struct RomDBInfo
{
	unsigned digest[5];
	unsigned romType;
};

static const RomDBInfo romDB[] =
{
#include "EmbeddedRomDBData.h"
};

struct MediaType {
    constexpr MediaType(RomType rt) : romType(rt) {}

    constexpr MediaType(const MediaType& mt) : romType(mt.romType) {}

    RomType romType;
};

static RomType  romdbDefaultType = ROM_UNKNOWN;

extern "C" const char* romTypeToString(RomType romType)
{
    switch (romType) {    
    case ROM_STANDARD:    return langRomTypeStandard();
    case ROM_MSXDOS2:     return langRomTypeMsxdos2();
    case ROM_KONAMI5:     return langRomTypeKonamiScc();
    case ROM_MANBOW2:     return langRomTypeManbow2();
    case ROM_MEGAFLSHSCC: return langRomTypeMegaFlashRomScc();
    case ROM_OBSONET:     return langRomTypeObsonet();
    case ROM_DUMAS:       return langRomTypeDumas();
    case ROM_NOWIND:      return langRomTypeNoWind();
    case ROM_KONAMI4:     return langRomTypeKonami();
    case ROM_ASCII8:      return langRomTypeAscii8();
    case ROM_ASCII16:     return langRomTypeAscii16();
    case ROM_GAMEMASTER2: return langRomTypeGameMaster2();
    case ROM_ASCII8SRAM:  return langRomTypeAscii8Sram();
    case ROM_ASCII16SRAM: return langRomTypeAscii16Sram();
    case ROM_RTYPE:       return langRomTypeRtype();
    case ROM_CROSSBLAIM:  return langRomTypeCrossblaim();
    case ROM_HARRYFOX:    return langRomTypeHarryFox();
    case ROM_MAJUTSUSHI:  return langRomTypeMajutsushi();
    case ROM_KOREAN80:    return langRomTypeZenima80();
    case ROM_KOREAN90:    return langRomTypeZenima90();
    case ROM_KOREAN126:   return langRomTypeZenima126();
    case ROM_SCC:         return langRomTypeScc();
    case ROM_SCCPLUS:     return langRomTypeSccPlus();
    case ROM_SNATCHER:    return langRomTypeSnatcher();
    case ROM_SDSNATCHER:  return langRomTypeSdSnatcher();
    case ROM_SCCMIRRORED: return langRomTypeSccMirrored();
    case ROM_SCCEXTENDED: return langRomTypeSccExtended();
    case ROM_FMPAC:       return langRomTypeFmpac();
    case ROM_FMPAK:       return langRomTypeFmpak();
    case ROM_KONAMI4NF:   return langRomTypeKonamiGeneric();
    case ROM_ASCII16NF:   return langRomTypeSuperPierrot();
    case ROM_PLAIN:       return langRomTypeMirrored();
    case ROM_NETTOUYAKYUU:return "Jaleco Moero!! Nettou Yakyuu '88";
    case ROM_MATRAINK:    return "Matra INK";
    case ROM_FORTEII:     return "Forte II";
    case ROM_NORMAL:      return langRomTypeNormal();
    case ROM_DISKPATCH:   return langRomTypeDiskPatch();
    case ROM_CASPATCH:    return langRomTypeCasPatch();
    case ROM_TC8566AF:    return langRomTypeTc8566afFdc();
    case ROM_TC8566AF_TR: return langRomTypeTc8566afTrFdc();
    case ROM_MICROSOL:    return langRomTypeMicrosolFdc();
    case ROM_ARC:         return "Parallax ARC";
    case ROM_NATIONALFDC: return langRomTypeNationalFdc();
    case ROM_PHILIPSFDC:  return langRomTypePhilipsFdc();
    case ROM_SVI738FDC:   return langRomTypeSvi738Fdc();
    case RAM_MAPPER:      return langRomTypeMappedRam();
    case RAM_1KB_MIRRORED:return langRomTypeMirroredRam1k();
    case RAM_2KB_MIRRORED:return langRomTypeMirroredRam2k();
    case RAM_NORMAL:      return langRomTypeNormalRam();
    case ROM_KANJI:       return langRomTypeKanji();
    case ROM_HOLYQURAN:   return langRomTypeHolyQuran();
    case SRAM_MATSUCHITA: return langRomTypeMatsushitaSram();
    case SRAM_MATSUCHITA_INV: return langRomTypeMasushitaSramInv();
    case ROM_PANASONIC8:  return langRomTypePanasonic8();
    case ROM_PANASONICWX16:return langRomTypePanasonicWx16();
    case ROM_PANASONIC16: return langRomTypePanasonic16();
    case ROM_PANASONIC32: return langRomTypePanasonic32();
    case ROM_FSA1FMMODEM: return langRomTypePanasonicModem();
    case ROM_DRAM:        return langRomTypeDram();
    case ROM_BUNSETU:     return langRomTypeBunsetsu();
    case ROM_JISYO:       return langRomTypeJisyo();
    case ROM_KANJI12:     return langRomTypeKanji12();
    case ROM_NATIONAL:    return langRomTypeNationalSram();
    case SRAM_S1985:      return langRomTypeS1985();
    case ROM_S1990:       return langRomTypeS1990();
    case ROM_TURBORIO:    return langRomTypeTurborPause();
    case ROM_F4DEVICE:    return langRomTypeF4deviceNormal();
    case ROM_F4INVERTED:  return langRomTypeF4deviceInvert();
    case ROM_MSXMIDI:     return langRomTypeMsxMidi();
    case ROM_TURBORTIMER: return langRomTypeTurborTimer();
    case ROM_KOEI:        return langRomTypeKoei();
    case ROM_BASIC:       return langRomTypeBasic();
    case ROM_HALNOTE:     return langRomTypeHalnote();
    case ROM_LODERUNNER:  return langRomTypeLodeRunner();
    case ROM_0x4000:      return langRomTypeNormal4000();
    case ROM_0xC000:      return langRomTypeNormalC000();
    case ROM_KONAMISYNTH: return langRomTypeKonamiSynth();
    case ROM_KONAMKBDMAS: return langRomTypeKonamiKbdMast();
    case ROM_KONWORDPRO:  return langRomTypeKonamiWordPro();
    case ROM_PAC:         return langRomTypePac();
    case ROM_MEGARAM:     return langRomTypeMegaRam();
    case ROM_MEGARAM128:  return langRomTypeMegaRam128();
    case ROM_MEGARAM256:  return langRomTypeMegaRam256();
    case ROM_MEGARAM512:  return langRomTypeMegaRam512();
    case ROM_MEGARAM768:  return langRomTypeMegaRam768();
    case ROM_MEGARAM2M:   return langRomTypeMegaRam2mb();
    case ROM_EXTRAM:      return langRomTypeExtRam();
    case ROM_EXTRAM16KB:  return langRomTypeExtRam16();
    case ROM_EXTRAM32KB:  return langRomTypeExtRam32();
    case ROM_EXTRAM48KB:  return langRomTypeExtRam48();
    case ROM_EXTRAM64KB:  return langRomTypeExtRam64();
    case ROM_EXTRAM512KB: return langRomTypeExtRam512();
    case ROM_EXTRAM1MB:   return langRomTypeExtRam1mb();
    case ROM_EXTRAM2MB:   return langRomTypeExtRam2mb();
    case ROM_EXTRAM4MB:   return langRomTypeExtRam4mb();
    case ROM_MSXMUSIC:    return langRomTypeMsxMusic();
    case ROM_MSXAUDIO:    return langRomTypeMsxAudio();
    case ROM_MOONSOUND:   return langRomTypeMoonsound();
    case ROM_SVI328:      return langRomTypeSvi328Cart();
    case ROM_SVI328FDC:   return langRomTypeSvi328Fdc();
    case ROM_SVI328PRN:   return langRomTypeSvi328Prn();
    case ROM_SVI328RS232: return langRomTypeSvi328Uart();
    case ROM_SVI80COL:    return langRomTypeSvi328col80();
    case ROM_SVI328RSIDE: return langRomTypeSvi328RsIde();
    case ROM_SVI727:      return langRomTypeSvi727col80();
    case ROM_COLECO:      return langRomTypeColecoCart();
    case ROM_SG1000:      return langRomTypeSg1000Cart();
    case ROM_SC3000:      return langRomTypeSc3000Cart();
    case ROM_SG1000CASTLE:return langRomTypeTheCastle();
    case ROM_SEGABASIC:   return langRomTypeSegaBasic();
    case ROM_SONYHBI55:   return langRomTypeSonyHbi55();
    case ROM_MSXAUDIODEV: return langRomTypeY8950();
    case ROM_MSXPRN:      return langRomTypeMsxPrinter();
    case ROM_TURBORPCM:   return langRomTypeTurborPcm();
    case ROM_JOYREXPSG:   return "Joyrex PSG";
    case ROM_OPCODEPSG:   return "Opcode PSG";
    case ROM_GAMEREADER:  return langRomTypeGameReader();
    case ROM_SUNRISEIDE:  return langRomTypeSunriseIde();
    case ROM_BEERIDE:     return langRomTypeBeerIde();
    case ROM_NMS1210:     return "Philips NMS1210 Serial Interface";
    case ROM_GIDE:        return langRomTypeGide();
    case ROM_MICROSOL80:  return langRomTypeVmx80();
    case ROM_NMS8280DIGI: return langRomTypeNms8280Digitiz();
    case ROM_SONYHBIV1:   return langRomTypeHbiV1Digitiz();
    case ROM_PLAYBALL:    return langRomTypePlayBall();
    case ROM_FMDAS:       return langRomTypeFmdas();
    case ROM_YAMAHASFG01: return langRomTypeSfg01();
    case ROM_YAMAHASFG05: return langRomTypeSfg05();
    case ROM_YAMAHANET:   return "Yamaha Net";
    case ROM_SF7000IPL:   return "SF-7000 IPL";
    case ROM_OPCODEBIOS:  return "ColecoVision Opcode Bios";
    case ROM_OPCODEMEGA:  return "ColecoVision Opcode MegaRam";
    case ROM_OPCODESAVE:  return "ColecoVision Opcode SaveRam";
    case ROM_OPCODESLOT:  return "ColecoVision Opcode Slot Manager";
    case ROM_CVMEGACART:  return "ColecoVision MegaCart(R)";
    case SRAM_MEGASCSI:   return langRomTypeMegaSCSI();
    case SRAM_MEGASCSI128:return langRomTypeMegaSCSI128();
    case SRAM_MEGASCSI256:return langRomTypeMegaSCSI256();
    case SRAM_MEGASCSI512:return langRomTypeMegaSCSI512();
    case SRAM_MEGASCSI1MB:return langRomTypeMegaSCSI1mb();
    case SRAM_ESERAM:     return langRomTypeEseRam();
    case SRAM_ESERAM128:  return langRomTypeEseRam128();
    case SRAM_ESERAM256:  return langRomTypeEseRam256();
    case SRAM_ESERAM512:  return langRomTypeEseRam512();
    case SRAM_ESERAM1MB:  return langRomTypeEseRam1mb();
    case SRAM_WAVESCSI:   return langRomTypeWaveSCSI();
    case SRAM_WAVESCSI128:return langRomTypeWaveSCSI128();
    case SRAM_WAVESCSI256:return langRomTypeWaveSCSI256();
    case SRAM_WAVESCSI512:return langRomTypeWaveSCSI512();
    case SRAM_WAVESCSI1MB:return langRomTypeWaveSCSI1mb();
    case SRAM_ESESCC:     return langRomTypeEseSCC();
    case SRAM_ESESCC128:  return langRomTypeEseSCC128();
    case SRAM_ESESCC256:  return langRomTypeEseSCC256();
    case SRAM_ESESCC512:  return langRomTypeEseSCC512();
    case ROM_GOUDASCSI:   return langRomTypeGoudaSCSI();

    case ROM_UNKNOWN:     return langTextUnknown();
    }

    return langTextUnknown();
}

extern "C" const char* romTypeToShortString(RomType romType) 
{
    switch (romType) {
    case ROM_STANDARD:    return "STANDARD";
    case ROM_MSXDOS2:     return "MSXDOS2";
    case ROM_KONAMI5:     return "KONAMI SCC";
    case ROM_MANBOW2:     return "MANBOW 2";
    case ROM_MEGAFLSHSCC: return "MEGAFLSHSCC";
    case ROM_OBSONET:     return "OBSONET";
    case ROM_DUMAS:       return "DUMAS";
    case ROM_NOWIND:      return "NOWIND";
    case ROM_KONAMI4:     return "KONAMI";
    case ROM_ASCII8:      return "ASCII8";
    case ROM_ASCII16:     return "ASCII16";
    case ROM_GAMEMASTER2: return "GMASTER2";
    case ROM_ASCII8SRAM:  return "ASCII8SRAM";
    case ROM_ASCII16SRAM: return "ASCII16SRAM";
    case ROM_RTYPE:       return "R-TYPE";
    case ROM_CROSSBLAIM:  return "CROSSBLAIM";
    case ROM_HARRYFOX:    return "HARRYFOX";
    case ROM_KOREAN80:    return "ZEM 80IN1";
    case ROM_KOREAN126:   return "ZEM 126IN1";
    case ROM_KOREAN90:    return "ZEM 90IN1";
    case ROM_SCC:         return "SCC";
    case ROM_SCCPLUS:     return "SCC-I";
    case ROM_SNATCHER:    return "SNATCHER";
    case ROM_SDSNATCHER:  return "SDSNATCHER";
    case ROM_SCCMIRRORED: return "SCCMIRRORED";
    case ROM_SCCEXTENDED: return "SCCEXTENDED";
    case ROM_FMPAC:       return "FMPAC";
    case ROM_FMPAK:       return "FMPAK";
    case ROM_KONAMI4NF:   return "KONAMI GEN";
    case ROM_ASCII16NF:   return "SUPERPIERR";
    case ROM_PLAIN:       return "MIRRORED";
    case ROM_NETTOUYAKYUU:return "NETTOU YAKYUU";
    case ROM_MATRAINK:    return "MATRA INK";
    case ROM_FORTEII:     return "FORTE II";
    case ROM_NORMAL:      return "NORMAL";
    case ROM_DRAM:        return "DRAM";
    case ROM_DISKPATCH:   return "DISKPATCH";
    case ROM_CASPATCH:    return "CASPATCH";
    case ROM_TC8566AF:    return "TC8566AF";
    case ROM_TC8566AF_TR: return "TC8566AF";
    case ROM_MICROSOL:    return "MICROSOL";
    case ROM_ARC:         return "ARC";
    case ROM_NATIONALFDC: return "NATNL FDC";
    case ROM_PHILIPSFDC:  return "PHILIPSFDC";
    case ROM_SVI738FDC:   return "SVI738 FDC";
    case RAM_MAPPER:      return "MAPPED RAM";
    case RAM_1KB_MIRRORED:return "1K MIR RAM";
    case RAM_2KB_MIRRORED:return "2K MIR RAM";
    case RAM_NORMAL:      return "NORMAL RAM";
    case ROM_KANJI:       return "KANJI";
    case ROM_HOLYQURAN:   return "HOLYQURAN";
    case SRAM_MATSUCHITA:     return "MATSUSHITA";
    case SRAM_MATSUCHITA_INV: return "MATSUS INV";
    case ROM_PANASONICWX16:   return "PANASON 16";
    case ROM_PANASONIC16: return "PANASON 16";
    case ROM_PANASONIC32: return "PANASON 32";
    case ROM_BUNSETU:     return "BUNSETSU";
    case ROM_JISYO:       return "JISYO";
    case ROM_KANJI12:     return "KANJI12";
    case ROM_NATIONAL:    return "NATIONAL";
    case SRAM_S1985:      return "S1985";
    case ROM_S1990:       return "S1990";
    case ROM_TURBORIO:    return "TR PAUSE";
    case ROM_F4DEVICE:    return "F4NORMAL";
    case ROM_F4INVERTED:  return "F4INV";
    case ROM_MSXMIDI:     return "MSX-MIDI";
    case ROM_TURBORTIMER: return "TURBORTMR";
    case ROM_KOEI:        return "KOEI";
    case ROM_BASIC:       return "BASIC";
    case ROM_HALNOTE:     return "HALNOTE";
    case ROM_LODERUNNER:  return "LODERUNNER";
    case ROM_0x4000:      return "4000h";
    case ROM_0xC000:      return "C000h";
    case ROM_KONAMISYNTH: return "KONSYNTH";
    case ROM_KONAMKBDMAS: return "KBDMASTER";
    case ROM_KONWORDPRO:  return "KONWORDPRO";
    case ROM_MAJUTSUSHI:  return "MAJUTSUSHI";
    case ROM_PAC:         return "PAC";
    case ROM_MEGARAM:     return "MEGARAM";
    case ROM_MEGARAM128:  return "MEGARAM128";
    case ROM_MEGARAM256:  return "MEGARAM256";
    case ROM_MEGARAM512:  return "MEGARAM512";
    case ROM_MEGARAM768:  return "MEGARAM768";
    case ROM_MEGARAM2M:   return "MEGARAM2MB";
    case ROM_EXTRAM:      return "EXTERN RAM";
    case ROM_EXTRAM16KB:  return "EXTRAM 16";
    case ROM_EXTRAM32KB:  return "EXTRAM 32";
    case ROM_EXTRAM48KB:  return "EXTRAM 48";
    case ROM_EXTRAM64KB:  return "EXTRAM 64";
    case ROM_EXTRAM512KB: return "EXTRAM 512";
    case ROM_EXTRAM1MB:   return "EXTRAM 2MB";
    case ROM_EXTRAM2MB:   return "EXTRAM 1MB";
    case ROM_EXTRAM4MB:   return "EXTRAM 4MB";
    case ROM_MSXMUSIC:    return "MSXMUSIC";
    case ROM_MSXAUDIO:    return "MSXAUDIO";
    case ROM_MOONSOUND:   return "MOONSOUND";
    case ROM_SVI328:      return "SVI328";
    case ROM_SVI328FDC:   return "SVI328FDC";
    case ROM_SVI328PRN:   return "SVI328PRN";
    case ROM_SVI328RS232: return "SVI328RS232";
    case ROM_SVI80COL:    return "SVI80COL";
    case ROM_SVI328RSIDE: return "SVI328RSIDE";
    case ROM_SVI727:      return "SVI727";
    case ROM_COLECO:      return "COLECO";
    case ROM_SG1000:      return "SG-1000";
    case ROM_SC3000:      return "SC-3000";
    case ROM_SG1000CASTLE:return "THECASTLE";
    case ROM_SEGABASIC:   return "SEGABASIC";
    case ROM_SONYHBI55:   return "HBI-55";
    case ROM_MSXAUDIODEV: return "MSXAUDIO";
    case ROM_MSXPRN:      return "MSXPRN";
    case ROM_TURBORPCM:   return "TURBOR PCM";
    case ROM_JOYREXPSG:   return "JOYREX PSG";
    case ROM_OPCODEPSG:   return "OPCODE PSG";
    case ROM_OPCODEBIOS:  return "OPCODE BIOS";
    case ROM_OPCODEMEGA:  return "OPCODE MEGA";
    case ROM_OPCODESAVE:  return "OPCODE SAVE";
    case ROM_OPCODESLOT:  return "OPCODE SLOT";
    case ROM_GAMEREADER:  return "GAMEREADER";
    case ROM_SUNRISEIDE:  return "SUNRISEIDE";
    case ROM_BEERIDE:     return "BEER IDE";
    case ROM_NMS1210:     return "NMS1210";
    case ROM_GIDE:        return "GIDE";
    case ROM_MICROSOL80:  return "MICROSOL80";
    case ROM_NMS8280DIGI: return "8280 DIGI";
    case ROM_SONYHBIV1:   return "SONY HBI-V1";
    case ROM_PLAYBALL:    return "PLAYBALL";
    case ROM_FMDAS:       return "FM-DAS";
    case ROM_YAMAHASFG01: return "SFG-01";
    case ROM_YAMAHASFG05: return "SFG-05";
    case ROM_YAMAHANET:   return "YAMAHA NET";
    case ROM_SF7000IPL:   return "SF-7000 IPL";
    case ROM_CVMEGACART:  return "MEGACART";
    case SRAM_MEGASCSI:   return "MEGASCSI";
    case SRAM_MEGASCSI128:return "MEGASCSI128";
    case SRAM_MEGASCSI256:return "MEGASCSI256";
    case SRAM_MEGASCSI512:return "MEGASCSI512";
    case SRAM_MEGASCSI1MB:return "MEGASCSI1MB";
    case SRAM_ESERAM:     return "ESE-RAM";
    case SRAM_ESERAM128:  return "ESE-RAM128";
    case SRAM_ESERAM256:  return "ESE-RAM256";
    case SRAM_ESERAM512:  return "ESE-RAM512";
    case SRAM_ESERAM1MB:  return "ESE-RAM1MB";
    case SRAM_WAVESCSI:   return "WAVESCSI";
    case SRAM_WAVESCSI128:return "WAVESCSI128";
    case SRAM_WAVESCSI256:return "WAVESCSI256";
    case SRAM_WAVESCSI512:return "WAVESCSI512";
    case SRAM_WAVESCSI1MB:return "WAVESCSI1MB";
    case SRAM_ESESCC:     return "ESE-SCC";
    case SRAM_ESESCC128:  return "ESE-SCC128";
    case SRAM_ESESCC256:  return "ESE-SCC256";
    case SRAM_ESESCC512:  return "ESE-SCC512";
    case ROM_GOUDASCSI:   return "GOUDA SCSI";

    case ROM_UNKNOWN:     return "UNKNOWN";
    }

    return "UNKNOWN";
}

int romTypeIsRom(RomType romType) {
    switch (romType) {
    case ROM_SCC:         return 1;
    case ROM_SCCPLUS:     return 1;
    case ROM_MOONSOUND:   return 1;
    case ROM_SNATCHER:    return 1;
    case ROM_SDSNATCHER:  return 1;
    case ROM_SCCMIRRORED: return 1;
    case ROM_SCCEXTENDED: return 1;
    case ROM_PLAIN:       return 1;
    case ROM_NETTOUYAKYUU:return 1;
    case ROM_MATRAINK:    return 1;
    case ROM_FORTEII:     return 1;
    case ROM_FMPAK:       return 1;
    case ROM_NORMAL:      return 1;
    case ROM_DRAM:        return 1;
    case ROM_DISKPATCH:   return 1;
    case ROM_CASPATCH:    return 1;
    case ROM_MICROSOL:    return 1;
    case ROM_ARC:         return 1;
    case ROM_NATIONALFDC: return 1;
    case ROM_PHILIPSFDC:  return 1;
    case ROM_SVI738FDC:   return 1;
    case ROM_HOLYQURAN:   return 1;
    case SRAM_MATSUCHITA: return 1;
    case SRAM_MATSUCHITA_INV: return 1;
    case ROM_BASIC:       return 1;
    case ROM_0x4000:      return 1;
    case ROM_0xC000:      return 1;
    case ROM_KONAMISYNTH: return 1;
    case ROM_KONAMKBDMAS: return 1;
    case ROM_KONWORDPRO:  return 1;
    case ROM_MICROSOL80:  return 1;
    case ROM_SONYHBIV1:   return 1;
    case ROM_PLAYBALL:    return 1;
    case ROM_FMDAS:       return 1;
    case ROM_YAMAHASFG01: return 1;
    case ROM_YAMAHASFG05: return 1;
    case ROM_SF7000IPL:   return 1;
    case ROM_YAMAHANET:   return 1;
    case ROM_EXTRAM16KB:  return 1;
    case ROM_EXTRAM32KB:  return 1;
    case ROM_EXTRAM48KB:  return 1;
    case ROM_EXTRAM64KB:  return 1;
    }
    return 0;
}

int romTypeIsMegaRom(RomType romType) {
    switch (romType) {
    case ROM_STANDARD:    return 1;
    case ROM_MSXDOS2:     return 1;
    case ROM_KONAMI5:     return 1;
    case ROM_MANBOW2:     return 1;
    case ROM_MEGAFLSHSCC: return 1;
    case ROM_OBSONET:     return 1;
    case ROM_DUMAS:       return 1;
    case ROM_NOWIND:      return 1;
    case ROM_KONAMI4:     return 1;
    case ROM_ASCII8:      return 1;
    case ROM_ASCII16:     return 1;
    case ROM_GAMEMASTER2: return 1;
    case ROM_ASCII8SRAM:  return 1;
    case ROM_TC8566AF:    return 1;
    case ROM_TC8566AF_TR: return 1;
    case ROM_ASCII16SRAM: return 1;
    case ROM_RTYPE:       return 1;
    case ROM_CROSSBLAIM:  return 1;
    case ROM_HARRYFOX:    return 1;
    case ROM_KOREAN80:    return 1;
    case ROM_KOREAN126:   return 1;
    case ROM_KONAMI4NF:   return 1;
    case ROM_ASCII16NF:   return 1;
    case ROM_HOLYQURAN:   return 1;
    case ROM_MAJUTSUSHI:  return 1;
    case ROM_KOEI:        return 1;
    case ROM_HALNOTE:     return 1;
    case ROM_LODERUNNER:  return 1;
    case ROM_MSXAUDIO:    return 1;
    case ROM_KOREAN90:    return 1;
    case ROM_SONYHBI55:   return 1;
    case ROM_EXTRAM512KB: return 1;
    case ROM_EXTRAM1MB:   return 1;
    case ROM_EXTRAM2MB:   return 1;
    case ROM_EXTRAM4MB:   return 1;
    case ROM_GAMEREADER:  return 1;
    case ROM_SUNRISEIDE:  return 1;
    case ROM_BEERIDE:     return 1;
    case SRAM_MEGASCSI:   return 1;
    case SRAM_MEGASCSI128:return 1;
    case SRAM_MEGASCSI256:return 1;
    case SRAM_MEGASCSI512:return 1;
    case SRAM_MEGASCSI1MB:return 1;
    case SRAM_ESERAM:     return 1;
    case SRAM_ESERAM128:  return 1;
    case SRAM_ESERAM256:  return 1;
    case SRAM_ESERAM512:  return 1;
    case SRAM_ESERAM1MB:  return 1;
    case SRAM_WAVESCSI:   return 1;
    case SRAM_WAVESCSI128:return 1;
    case SRAM_WAVESCSI256:return 1;
    case SRAM_WAVESCSI512:return 1;
    case SRAM_WAVESCSI1MB:return 1;
    case SRAM_ESESCC:     return 1;
    case SRAM_ESESCC128:  return 1;
    case SRAM_ESESCC256:  return 1;
    case SRAM_ESESCC512:  return 1;
    }
    return 0;
}

int romTypeIsMegaRam(RomType romType) {
    switch (romType) {
    case ROM_MEGARAM:     return 1;
    case ROM_MEGARAM128:  return 1;
    case ROM_MEGARAM256:  return 1;
    case ROM_MEGARAM512:  return 1;
    case ROM_MEGARAM768:  return 1;
    case ROM_MEGARAM2M:   return 1;
    }
    return 0;
}

int romTypeIsFmPac(RomType romType) {
    switch (romType) {
    case ROM_FMPAC:       return 1;
    }
    return 0;
}

RomType mediaDbGetRomType(MediaType* mediaType)
{
    return mediaType->romType;
}

extern "C" void mediaDbSetDefaultRomType(RomType romType)
{
    romdbDefaultType = romType;
}

extern "C" MediaType* mediaDbLookupRom(const void *buffer, int size)
{
    const char* romData = (const char*)buffer;
    static MediaType defaultColeco(ROM_COLECO);
    static MediaType defaultSvi(ROM_SVI328);
    static MediaType defaultSg1000(ROM_SG1000);
    static MediaType defaultSc3000(ROM_SC3000);

    /*if (romdb == NULL)
    {
        return NULL;
    }*/
    static MediaType staticMediaType(ROM_UNKNOWN);

    boost::uuids::detail::sha1 sha1;
		sha1.process_bytes(buffer, size);
		unsigned int digest[5];
		sha1.get_digest(digest);
		logMsg("rom sha1 0x%X 0x%X 0x%X 0x%X 0x%X", digest[0], digest[1], digest[2], digest[3], digest[4]);

		for(auto e : romDB)
		{
			int match = 0;
			for(auto i : IG::iotaCount(5))
			{
					if(digest[i] != e.digest[i])
						break;
					else match++;
			}
			if(match == 5)
			{
				logMsg("found match with type %s", romTypeToString(e.romType));
				staticMediaType = e.romType;
				return &staticMediaType;
			}
		}

		logMsg("rom not in DB");
		return nullptr;

    /*MediaType* mediaType = mediaDbLookup(romdb, buffer, size);

    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0xF3 && romData[1] == 0x31)
    {
        mediaType = &defaultSvi;
    }

    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0x55 && (unsigned char)romData[1] == 0xAA)
    {
        mediaType = &defaultColeco;
    }
#if 0
    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0x55 && (unsigned char)romData[1] == 0xAA)
    {
        mediaType = &defaultSg1000;
    }
#endif
    return mediaType;*/
}

extern "C" MediaType* mediaDbGuessRom(const void *buffer, int size)
{
    static MediaType staticMediaType(ROM_UNKNOWN);

    const UInt8* romData = (const UInt8*)buffer;
    int i;
    int mapper;
    UInt32 counters[6] = { 0, 0, 0, 0, 0, 0 };

    staticMediaType.romType = romdbDefaultType;

    if (size < 128) {
        return &staticMediaType;
    }

    MediaType* mediaType = mediaDbLookupRom(buffer, size);
    if (mediaType == NULL) {
        mediaType = &staticMediaType;
//        printf("xx %d\n", romdbDefaultType);
    }

    if (mediaType->romType != ROM_UNKNOWN) {
        return mediaType;
    }

    logMsg("detecting ROM type");

    BoardType boardType = boardGetType();

    switch (boardType) {
    case BOARD_SVI:
        staticMediaType.romType = ROM_SVI328;
        return &staticMediaType;
    case BOARD_COLECO:
    case BOARD_COLECOADAM:
        staticMediaType.romType = ROM_COLECO;
        return &staticMediaType;
    case BOARD_SG1000:
        staticMediaType.romType = ROM_SG1000;
        return &staticMediaType;
    case BOARD_SC3000:
    case BOARD_SF7000:
        staticMediaType.romType = ROM_SC3000;
        return &staticMediaType;
    case BOARD_MSX_FORTE_II:
        break;
    case BOARD_MSX:
        break;
    }


	if (size <= 0x10000) {
		if (size == 0x10000) {
            if (romData[0x4000] == 'A' && romData[0x4001] == 'B') mediaType->romType = ROM_PLAIN;
            else mediaType->romType = ROM_ASCII16;
			return mediaType;
		} 
        
        if (size <= 0x4000 && romData[0] == 'A' && romData[1] == 'B') {
			UInt16 init = romData[2] + 256 * romData[3];
			UInt16 text = romData[8] + 256 * romData[9];
//			if (init == 0 && (text & 0xc000) == 0x8000) {
			if ((text & 0xc000) == 0x8000) {
                mediaType->romType = ROM_BASIC;
			    return mediaType;
			}
		}
        mediaType->romType = ROM_PLAIN;
		return mediaType;
	}
    
    const char ManbowTag[] = "Mapper: Manbow 2";
    UInt32 tagLength = strlen(ManbowTag);
    for (i = 0; i < (int)(size - tagLength); i++) {
        if (romData[i] == ManbowTag[0]) {
            if (memcmp(romData + i, ManbowTag, tagLength) == 0) {
                mediaType->romType = ROM_MANBOW2;
			    return mediaType;
            }
        }
    }

    /* Count occurences of characteristic addresses */
    for (i = 0; i < size - 3; i++) {
        if (romData[i] == 0x32) {
            UInt32 value = romData[i + 1] + ((UInt32)romData[i + 2] << 8);

            switch(value) {
            case 0x4000: 
            case 0x8000: 
            case 0xa000: 
                counters[3]++;
                break;

            case 0x5000: 
            case 0x9000: 
            case 0xb000: 
                counters[2]++;
                break;

            case 0x6000: 
                counters[3]++;
                counters[4]++;
                counters[5]++;
                break;

            case 0x6800: 
            case 0x7800: 
                counters[4]++;
                break;

            case 0x7000: 
                counters[2]++;
                counters[4]++;
                counters[5]++;
                break;

            case 0x77ff: 
                counters[5]++;
                break;
            }
        }
    }

    /* Find which mapper type got more hits */
    mapper = 0;

    counters[4] -= counters[4] ? 1 : 0;

	for (i = 0; i <= 5; i++) {
		if (counters[i] > 0 && counters[i] >= counters[mapper]) {
			mapper = i;
		}
	}

    if (mapper == 5 && counters[0] == counters[5]) {
		mapper = 0;
	}

    switch (mapper) {
    default:
    case 0: mediaType->romType = ROM_STANDARD; break;
    case 1: mediaType->romType = ROM_MSXDOS2; break;
    case 2: mediaType->romType = ROM_KONAMI5; break;
    case 3: mediaType->romType = ROM_KONAMI4; break;
    case 4: mediaType->romType = ROM_ASCII8; break;
    case 5: mediaType->romType = ROM_ASCII16; break;
    }
    
    return mediaType;
}
