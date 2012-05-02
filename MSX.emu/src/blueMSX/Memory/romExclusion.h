/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/ramNormal.h,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:42 $
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
#ifndef ROM_EXCLUSION_H
#define ROM_EXCLUSION_H

#include "MsxTypes.h"

#ifdef EXCLUDE_SPECIAL_GAME_CARTS
#define romMapperSonyHBI55Create() 0
#define romMapperMatraINKCreate(a, b, c, d, e, f) 0
#define romMapperNettouYakyuuCreate(a, b, c, d, e, f) 0
#define romMapperFmDasCreate(a, b, c, d, e, f) 0
#define romMapperKonamiSynthCreate(a, b, c, d, e, f) 0
#define romMapperMajutsushiCreate(a, b, c, d, e, f) 0
#define romMapperHolyQuranCreate(a, b, c, d, e, f) 0
#define romMapperKonamiWordProCreate(a, b, c, d, e, f) 0
#define romMapperKonamiKeyboardMasterCreate(a, b, c, d, e, f, g, h) 0
#define romMapperGameMaster2Create(a, b, c, d, e, f) 0
#define romMapperHarryFoxCreate(a, b, c, d, e, f) 0
#define romMapperHalnoteCreate(a, b, c, d, e, f) 0
#define romMapperRTypeCreate(a, b, c, d, e, f) 0
#define romMapperCrossBlaimCreate(a, b, c, d, e, f) 0
#define romMapperLodeRunnerCreate(a, b, c, d, e, f) 0
#define romMapperKorean80Create(a, b, c, d, e, f) 0
#define romMapperKorean90Create(a, b, c, d, e, f) 0
#define romMapperKorean126Create(a, b, c, d, e, f) 0
#define romMapperArcCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_MSXMIDI
#define MSXMidiCreate() 0
#endif
#ifdef EXCLUDE_NMS8280DIGI
#define romMapperNms8280VideoDaCreate() 0
#endif
#ifdef EXCLUDE_JOYREXPSG
#define romMapperJoyrexPsgCreate() 0
#endif
#ifdef EXCLUDE_OPCODE_DEVICES
#define romMapperOpcodePsgCreate() 0
#define romMapperOpcodeMegaRamCreate(a, b, c) 0
#define romMapperOpcodeSaveRamCreate(a, b, c) 0
#define romMapperOpcodeSlotManagerCreate() 0
#define romMapperOpcodeBiosCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_SVI_DEVICES
#define svi328FdcCreate() 0
#define romMapperSvi328PrnCreate() 0
#define romMapperSvi328Rs232Create(a) 0
#define romMapperSvi328RsIdeCreate(a) 0
#define romMapperSvi738FdcCreate(a, b, c, d, e, f) 0
#define romMapperSvi727Create(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_FORTEII
#define romMapperForteIICreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_OBSONET
#define romMapperObsonetCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_NOWIND
#define romMapperNoWindCreate(a, b, c, d, e, f, g) 0
#endif
#ifdef EXCLUDE_DUMAS
#define romMapperDumasCreate(a, b, c, d, e, f, g, h) 0
#endif
#ifdef EXCLUDE_MOONSOUND
#define romMapperMoonsoundCreate(a, b, c, d) 0
#endif
#ifdef EXCLUDE_PANASONIC_DEVICES
#define romMapperA1FMCreate(a, b, c, d, e, f, g) 0
#define romMapperPanasonicCreate(a, b, c, d, e, f, g, h) 0
#define romMapperA1FMModemCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_YAMAHA_SFG
#define romMapperSfg05Create(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_ROM_YAMAHANET
#define romMapperNetCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_SEGA_DEVICES
#define romMapperSf7000IplCreate(a, b, c, d, e, f) 0
#define romMapperSg1000Create(a, b, c, d, e, f) 0
#define romMapperSg1000CastleCreate(a, b, c, d, e, f) 0
#define romMapperSegaBasicCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_DISK_DEVICES
#define romMapperDiskCreate(a, b, c, d, e, f) 0
#define romMapperTC8566AFCreate(a, b, c, d, e, f, g) 0
#define romMapperMicrosolCreate(a, b, c, d, e, f) 0
#define romMapperNationalFdcCreate(a, b, c, d, e, f) 0
#define romMapperPhilipsFdcCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_IDE_DEVICES_SUNRISE
#define romMapperSunriseIdeCreate(a, b, c, d, e, f, g) 0
#endif
#ifdef EXCLUDE_IDE_DEVICES
#define romMapperBeerIdeCreate(a, b, c, d, e, f, g) 0
#define romMapperGoudaSCSICreate(a, b, c, d, e, f, g) 0
#define sramMapperEseSCCCreate(a, b, c, d, e, f, g, h) 0
#define sramMapperMegaSCSICreate(a, b, c, d, e, f, g, h) 0
#endif
#ifdef EXCLUDE_MICROSOL80
#define romMapperMicrosolVmx80Create(a, b, c, d, e, f, g, h) 0
#endif
#ifdef EXCLUDE_SRAM_MATSUCHITA
#define sramMapperMatsushitaCreate(a) 0
#endif
#ifdef EXCLUDE_SRAM_S1985
#define sramMapperS1985Create() 0
#endif
#ifdef EXCLUDE_ROM_S1990
#define romMapperS1990Create() 0
#endif
#ifdef EXCLUDE_ROM_TURBOR
#define romMapperTurboRTimerCreate() 0
#define romMapperTurboRIOCreate() 0
#define romMapperTurboRPcmCreate() 0
#define romMapperDramCreate(a, b, c, d, e, f) 0
#endif
#ifdef EXCLUDE_ROM_F4DEVICE
#define romMapperF4deviceCreate(a) 0
#endif

#endif
