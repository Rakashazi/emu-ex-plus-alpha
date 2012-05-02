/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageMinimal.c,v $
**
** $Revision: 1.21 $
**
** $Date: 2009-04-04 20:57:19 $
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
#include "Language.h"
#include <string.h>

EmuLanguageType langFromName(char* name, int translate)
{
    return 0 == strcmp(name, "English") ? EMU_LANG_ENGLISH : EMU_LANG_UNKNOWN;
}


const char* langToName(EmuLanguageType languageType, int translate)
{
    return languageType == EMU_LANG_ENGLISH ? "English" : "";
}

EmuLanguageType langGetType(int i)
{
    return i == 0 ? EMU_LANG_ENGLISH : EMU_LANG_UNKNOWN;
}

void langInit() {
}

int langSetLanguage(EmuLanguageType languageType)
{
    return languageType == EMU_LANG_ENGLISH ? 1 : 0;
}


//----------------------
// Generic lines
//----------------------

char* langTextUnknown()             { return "Unknown"; }


//----------------------
// Rom type lines
//----------------------

char* langRomTypeStandard()         { return "Standard"; }
char* langRomTypeMsxdos2()          { return "MSXDOS 2"; }
char* langRomTypeKonamiScc()        { return "Konami SCC"; }
char* langRomTypeManbow2()          { return "Manbbow 2"; }
char* langRomTypeMegaFlashRomScc()  { return "Mega Flash Rom SCC"; }
char* langRomTypeKonami()           { return "Konami"; }
char* langRomTypeAscii8()           { return "ASCII 8"; }
char* langRomTypeAscii16()          { return "ASCII 16"; }
char* langRomTypeGameMaster2()      { return "Game Master 2 (SRAM)"; }
char* langRomTypeAscii8Sram()       { return "ASCII 8 (SRAM)"; }
char* langRomTypeAscii16Sram()      { return "ASCII 16 (SRAM)"; }
char* langRomTypeRtype()            { return "R-Type"; }
char* langRomTypeCrossblaim()       { return "Crossblaim"; }
char* langRomTypeHarryFox()         { return "Harry Fox"; }
char* langRomTypeMajutsushi()       { return "Konami Majutsushi"; }
char* langRomTypeZenima80()         { return "Zenima 80 in 1"; }
char* langRomTypeZenima90()         { return "Zenima 90 in 1"; }
char* langRomTypeZenima126()        { return "Zenima 126 in 1"; }
char* langRomTypeScc()              { return "SCC"; }
char* langRomTypeSccPlus()          { return "SCC+"; }
char* langRomTypeSnatcher()         { return "Snatcher"; }
char* langRomTypeSdSnatcher()       { return "SD Snatcher"; }
char* langRomTypeSccMirrored()      { return "SCC Mirrored"; }
char* langRomTypeSccExtended()      { return "SCC Extended"; }
char* langRomTypeFmpac()            { return "FMPAC"; }
char* langRomTypeFmpak()            { return "FMPAK"; }
char* langRomTypeKonamiGeneric()    { return "Konami Generic"; }
char* langRomTypeSuperPierrot()     { return "Super Pierrot"; }
char* langRomTypeMirrored()         { return "Mirrored ROM"; }
char* langRomTypeNormal()           { return "Normal ROM"; }
char* langRomTypeDiskPatch()        { return "Normal + Disk Patch"; }
char* langRomTypeCasPatch()         { return "Normal + Cassette Patch"; }
char* langRomTypeTc8566afFdc()      { return "TC8566AF Disk Controller"; }
char* langRomTypeTc8566afTrFdc()    { return "TC8566AF Turbo-R Disk Controller"; }
char* langRomTypeMicrosolFdc()      { return "Microsol Disk Controller"; }
char* langRomTypeNationalFdc()      { return "National Disk Controller"; }
char* langRomTypePhilipsFdc()       { return "Philips Disk Controller"; }
char* langRomTypeSvi738Fdc()        { return "SVI-738 Disk Controller"; }
char* langRomTypeMappedRam()        { return "Mapped RAM"; }
char* langRomTypeMirroredRam1k()    { return "1kB Mirrored RAM"; }
char* langRomTypeMirroredRam2k()    { return "2kB Mirrored RAM"; }
char* langRomTypeNormalRam()        { return "Normal RAM"; }
char* langRomTypeKanji()            { return "Kanji"; }
char* langRomTypeHolyQuran()        { return "Holy Quran"; }
char* langRomTypeMatsushitaSram()   { return "Matsushita SRAM"; }
char* langRomTypePanasonic8()       { return "Panasonic FM 8kB SRAM"; }
char* langRomTypePanasonicWx16()    { return "Panasonic WX 16kB SRAM"; }
char* langRomTypePanasonic16()      { return "Panasonic 16kB SRAM"; }
char* langRomTypePanasonic32()      { return "Panasonic 32kB SRAM"; }
char* langRomTypePanasonicModem()   { return "Panasonic Modem"; }
char* langRomTypeDram()             { return "Panasonic DRAM"; }
char* langRomTypeBunsetsu()         { return "Bunsetsu"; }
char* langRomTypeJisyo()            { return "Jisyo"; }
char* langRomTypeKanji12()          { return "Kanji 12"; }
char* langRomTypeNationalSram()     { return "National (SRAM)"; }
char* langRomTypeS1985()            { return "S1985"; }
char* langRomTypeS1990()            { return "S1990"; }
char* langRomTypeTurborPause()      { return "Turbo-R Pause"; }
char* langRomTypeF4deviceNormal()   { return "F4 Device Normal"; }
char* langRomTypeF4deviceInvert()   { return "F4 Device Inverted"; }
char* langRomTypeMsxMidi()          { return "MSX-MIDI"; }
char* langRomTypeTurborTimer()      { return "Turbo-R Timer"; }
char* langRomTypeKoei()             { return "Koei (SRAM)"; }
char* langRomTypeBasic()            { return "Basic ROM"; }
char* langRomTypeHalnote()          { return "Halnote"; }
char* langRomTypeLodeRunner()       { return "Lode Runner"; }
char* langRomTypeNormal4000()       { return "Normal 4000h"; }
char* langRomTypeNormalC000()       { return "Normal C000h"; }
char* langRomTypeKonamiSynth()      { return "Konami Synthesizer"; }
char* langRomTypeKonamiKbdMast()    { return "Konami Keyboard Master"; }
char* langRomTypeKonamiWordPro()    { return "Konami Word Pro"; }
char* langRomTypePac()              { return "PAC (SRAM)"; }
char* langRomTypeMegaRam()          { return "MegaRAM"; }
char* langRomTypeMegaRam128()       { return "128kB MegaRAM"; }
char* langRomTypeMegaRam256()       { return "256kB MegaRAM"; }
char* langRomTypeMegaRam512()       { return "512kB MegaRAM"; }
char* langRomTypeMegaRam768()       { return "768kB MegaRAM"; }
char* langRomTypeMegaRam2mb()       { return "2MB MegaRAM"; }
char* langRomTypeExtRam()           { return "External RAM"; }
char* langRomTypeExtRam16()         { return "16kB External RAM"; }
char* langRomTypeExtRam32()         { return "32kB External RAM"; }
char* langRomTypeExtRam48()         { return "48kB External RAM"; }
char* langRomTypeExtRam64()         { return "64kB External RAM"; }
char* langRomTypeExtRam512()        { return "512kB External RAM"; }
char* langRomTypeExtRam1mb()        { return "1MB External RAM"; }
char* langRomTypeExtRam2mb()        { return "2MB External RAM"; }
char* langRomTypeExtRam4mb()        { return "4MB External RAM"; }
char* langRomTypeMsxMusic()         { return "MSX Music"; }
char* langRomTypeMsxAudio()         { return "MSX Audio"; }
char* langRomTypeY8950()            { return "Y8950"; }
char* langRomTypeMoonsound()        { return "Moonsound"; }
char* langRomTypeSvi328Cart()       { return "SVI-328 Cartridge"; }
char* langRomTypeSvi328Fdc()        { return "SVI-328 Disk Controller"; }
char* langRomTypeSvi328Prn()        { return "SVI-328 Printer"; }
char* langRomTypeSvi328Uart()       { return "SVI-328 Serial Port"; }
char* langRomTypeSvi328col80()      { return "SVI-328 80 Column Card"; }
char* langRomTypeSvi328RsIde()      { return "SVI-328 RS IDE"; }
char* langRomTypeSvi727col80()      { return "SVI-727 80 Column Card"; }
char* langRomTypeColecoCart()       { return "Coleco Cartridge"; }
char* langRomTypeSg1000Cart()       { return "SG-1000 Cartridge"; }
char* langRomTypeSc3000Cart()       { return "SC-3000 Cartridge"; }
char* langRomTypeTheCastle()        { return "The Castle"; }
char* langRomTypeSonyHbi55()        { return "Sony HBI-55"; }
char* langRomTypeMsxPrinter()       { return "MSX Printer"; }
char* langRomTypeTurborPcm()        { return "Turbo-R PCM Chip"; }
char* langRomTypeGameReader()       { return "Sunrise GameReader"; }
char* langRomTypeSunriseIde()       { return "Sunrise IDE"; }
char* langRomTypeBeerIde()          { return "Beer IDE"; }
char* langRomTypeGide()             { return "GIDE"; }
char* langRomTypeVmx80()            { return "Microsol VMX-80"; }
char* langRomTypeNms8280Digitiz()   { return "Philips NMS-8280 Digitizer"; }
char* langRomTypeHbiV1Digitiz()     { return "Sony HBI-V1 Digitizer"; }
char* langRomTypeFmdas()            { return "F&M Direct Assembler System"; }
char* langRomTypeSfg01()            { return "Yamaha SFG-01"; }
char* langRomTypeSfg05()            { return "Yamaha SFG-05"; }
char* langRomTypePlayBall()         { return "Sony Playball"; }
char* langRomTypeObsonet()          { return "Obsonet"; }
char* langRomTypeDumas()            { return "Dumas"; }
char* langRomTypeSegaBasic()        { return "Sega Basic"; }
char* langRomTypeMegaSCSI()         { return "MEGA-SCSI"; }
char* langRomTypeMegaSCSI128()      { return "128kB MEGA-SCSI"; }
char* langRomTypeMegaSCSI256()      { return "256kB MEGA-SCSI"; }
char* langRomTypeMegaSCSI512()      { return "512kB MEGA-SCSI"; }
char* langRomTypeMegaSCSI1mb()      { return "1MB MEGA-SCSI"; }
char* langRomTypeEseRam()           { return "Ese-RAM"; }
char* langRomTypeEseRam128()        { return "128kB Ese-RAM"; }
char* langRomTypeEseRam256()        { return "256kB Ese-RAM"; }
char* langRomTypeEseRam512()        { return "512kB Ese-RAM"; }
char* langRomTypeEseRam1mb()        { return "1MB Ese-RAM"; }
char* langRomTypeWaveSCSI()         { return "WAVE-SCSI"; }
char* langRomTypeWaveSCSI128()      { return "128kB WAVE-SCSI"; }
char* langRomTypeWaveSCSI256()      { return "256kB WAVE-SCSI"; }
char* langRomTypeWaveSCSI512()      { return "512kB WAVE-SCSI"; }
char* langRomTypeWaveSCSI1mb()      { return "1MB WAVE-SCSI"; }
char* langRomTypeEseSCC()           { return "Ese-SCC"; }
char* langRomTypeEseSCC128()        { return "128kB Ese-SCC"; }
char* langRomTypeEseSCC256()        { return "256kB Ese-SCC"; }
char* langRomTypeEseSCC512()        { return "512kB Ese-SCC"; }
char* langRomTypeNoWind()           { return "NoWind USB"; }
char* langRomTypeGoudaSCSI()        { return "Gouda SCSI"; }
char* langRomTypeMasushitaSramInv() { return "Matsushita SRAM - Turbo 5.37MHz"; }

//----------------------
// Debug type lines
//----------------------

char* langDbgMemVisible()           { return "Visible Memory"; }
char* langDbgMemRamNormal()         { return "Normal"; }
char* langDbgMemRamMapped()         { return "Mapped"; }
char* langDbgMemVram()              { return "VRAM"; }
char* langDbgMemYmf278()            { return "YMF278 Sample RAM"; }
char* langDbgMemAy8950()            { return "AY8950 Sample RAM"; }
char* langDbgMemScc()               { return "Memory"; }

char* langDbgCallstack()            { return "Callstack"; }

char* langDbgRegs()                 { return "Registers"; }
char* langDbgRegsCpu()              { return "CPU Registers"; }
char* langDbgRegsYmf262()           { return "YMF262 Registers"; }
char* langDbgRegsYmf278()           { return "YMF278 Registers"; }
char* langDbgRegsAy8950()           { return "AY8950 Registers"; }
char* langDbgRegsYm2413()           { return "YM2413 Registers"; }

char* langDbgDevRamMapper()         { return "RAM Mapper"; }
char* langDbgDevRam()               { return "RAM"; }
char* langDbgDevIdeBeer()           { return "Beer IDE"; }
char* langDbgDevIdeGide()           { return "GIDE"; }
char* langDbgDevIdeSviRs()           { return "SVI-328 RS IDE"; }
char* langDbgDevScsiGouda()         { return "Gouda SCSI"; }
char* langDbgDevF4Device()          { return "F4 Device"; }
char* langDbgDevFmpac()             { return "FMPAC"; }
char* langDbgDevFmpak()             { return "FMPAK"; }
char* langDbgDevKanji()             { return "Kanji"; }
char* langDbgDevKanji12()           { return "Kanji 12"; }
char* langDbgDevKonamiKbd()         { return "Konami Keyboard Master"; }
char* langDbgDevKorean80()          { return "Korean 80"; }
char* langDbgDevKorean90()          { return "Korean 90"; }
char* langDbgDevKorean128()         { return "Korean 128"; }
char* langDbgDevMegaRam()           { return "Mega RAM"; }
char* langDbgDevFdcMicrosol()       { return "Microsol FDC"; }
char* langDbgDevMoonsound()         { return "Moonsound"; }
char* langDbgDevMsxAudio()          { return "MSX Audio"; }
char* langDbgDevMsxAudioMidi()      { return "MSX Audio MIDI"; }
char* langDbgDevMsxMusic()          { return "MSX Music"; }
char* langDbgDevPrinter()           { return "Printer"; }
char* langDbgDevRs232()             { return "RS232"; }
char* langDbgDevS1990()             { return "S1990"; }
char* langDbgDevSfg05()             { return "Yamaha SFG-05"; }
char* langDbgDevHbi55()             { return "Sony HBI-55"; }
char* langDbgDevSviFdc()            { return "SVI FDC"; }
char* langDbgDevSviPrn()            { return "SVI Printer"; }
char* langDbgDevSvi80Col()          { return "SVI 80 Column"; }
char* langDbgDevPcm()               { return "PCM"; }
char* langDbgDevMatsushita()        { return "Matsushita"; }
char* langDbgDevS1985()             { return "S1985"; }
char* langDbgDevCrtc6845()          { return "CRTC6845"; }
char* langDbgDevTms9929A()          { return "TMS9929A"; }
char* langDbgDevTms99x8A()          { return "TMS99x8A"; }
char* langDbgDevV9938()             { return "V9938"; }
char* langDbgDevV9958()             { return "V9958"; }
char* langDbgDevZ80()               { return "Z80"; }
char* langDbgDevMsxMidi()           { return "MSX MIDI"; }
char* langDbgDevPpi()               { return "PPI"; }
char* langDbgDevRtc()               { return "RTC"; }
char* langDbgDevTrPause()           { return "TR Pause"; }
char* langDbgDevAy8910()            { return "AY8910 PSG"; }
char* langDbgDevScc()               { return "SCC"; }

char* langEnumControlsJoyNone()             { return "None"; }
char* langEnumControlsJoyTetrisDongle()     { return "Tetris 2 Dongle"; }
char* langEnumControlsJoyMagicKeyDongle()   { return "MagicKey Dongle"; }
char* langEnumControlsJoyMouse()            { return "Mouse"; }
char* langEnumControlsJoy2Button()          { return "2-button Joystick"; }
char* langEnumControlsJoyGunStick()         { return "Gun Stick"; }
char* langEnumControlsJoyAsciiLaser()       { return "ASCII Plus-X Terminator Laser"; }
char* langEnumControlsJoyArkanoidPad()      { return "Arkanoid Pad"; }
char* langEnumControlsJoyColeco()           { return "ColecoVision Joystick"; }
