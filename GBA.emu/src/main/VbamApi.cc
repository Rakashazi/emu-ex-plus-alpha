/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <vbam/gba/GBA.h>
#include <vbam/gba/Sound.h>
#include <vbam/gba/RTC.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/Util.h>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "internal.hh"

struct GameSettings
{
	const char *gameName;
	char gameID[5];
	int saveSize;
	int saveType;
	bool rtcEnabled;
	bool mirroringEnabled;
	bool useBios;
};

constexpr GameSettings setting[]
{
#include "gba-over.inc"
};

int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
SystemColorMap systemColorMap;
uint32_t throttle{};
uint32_t speedup_throttle{};
uint32_t speedup_frame_skip{};
int emulating{};

static void debuggerOutput(const char *s, uint32_t addr)
{
	logMsg("called dbgOutput");
}
void (*dbgOutput)(const char *, uint32_t) = debuggerOutput;

#ifndef NDEBUG
void systemMessage(int num, const char *msg, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, msg );
	logger_vprintf(LOG_M, msg, args);
	va_end( args );
	logger_printf(LOG_M, "\n");
}

void log(const char *msg, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, msg );
	logger_vprintf(LOG_M, msg, args);
	va_end( args );
	logger_printf(LOG_M, "\n");
}
#else
void log(const char *msg, ...) {}
#endif

void systemUpdateMotionSensor()
{
}

int systemGetSensorX()
{
	return 0;
}

int systemGetSensorY()
{
	return 0;
}

int systemGetSensorZ()
{
	return 0;
}

uint8_t systemGetSensorDarkness()
{
	return 0;
}

void systemCartridgeRumble(bool e) {}

bool systemCanChangeSoundQuality()
{
	return false;
}

uint32_t systemGetClock() { return 0; }

void StartLink(uint16_t siocnt) {}

void StartGPLink(uint16_t value) {}

bool agbPrintWrite(uint32_t address, uint16_t value) { return false; };

void agbPrintEnable(bool enable) {}

void agbPrintFlush() {}

int CheckEReaderRegion(void) { return 0; }

void EReaderWriteMemory(uint32_t address, uint32_t value) {}

void BIOS_EReader_ScanCard(int swi_num) {}

bool soundInit() { return true; }
void soundSetThrottle(unsigned short throttle) {}
void soundPause() {}
void soundResume() {}
void soundShutdown() {}

static const char *saveTypeStr(int s)
{
	switch(s)
	{
		case GBA_SAVE_AUTO: return "Auto";
		case GBA_SAVE_EEPROM: return "EEPROM";
		case GBA_SAVE_SRAM: return "SRAM";
		case GBA_SAVE_FLASH: return "Flash";
		case GBA_SAVE_EEPROM_SENSOR: return "EEPROM+Sensor";
		case GBA_SAVE_NONE: return "None";
	}
	return "Unknown";
}

static void resetGameSettings()
{
	//agbPrintEnable(0);
	rtcEnable(0);
	cpuSaveType = GBA_SAVE_AUTO;
	flashSize = SIZE_FLASH512;
	eepromSize = SIZE_EEPROM_512;
}

void setGameSpecificSettings(GBASys &gba, int romSize)
{
	using namespace EmuEx;
	bool mirroringEnable{};
	resetGameSettings();
	logMsg("game id:%c%c%c%c", gba.mem.rom[0xac], gba.mem.rom[0xad], gba.mem.rom[0xae], gba.mem.rom[0xaf]);
	for(auto e : setting)
	{
		if(IG::equal_n(e.gameID, 4, &gba.mem.rom[0xac]))
		{
			logMsg("loading settings for:%s", e.gameName);
			if(e.rtcEnabled)
			{
				logMsg("uses RTC");
				detectedRtcGame = 1;
			}
			if(e.saveType > 0)
			{
				logMsg("uses save type:%s", saveTypeStr(e.saveType));
				cpuSaveType = e.saveType;
				if(e.saveType == GBA_SAVE_SRAM)
					flashSize = SIZE_SRAM;
			}
			if(e.saveSize > 0)
			{
				logMsg("uses save size:%d", e.saveSize);
				if(e.saveType == GBA_SAVE_FLASH && e.saveSize == SIZE_FLASH1M)
					flashSize = SIZE_FLASH1M;
				else if((e.saveType == GBA_SAVE_EEPROM || e.saveType == GBA_SAVE_EEPROM_SENSOR) && e.saveSize == SIZE_EEPROM_8K)
					eepromSize = SIZE_EEPROM_8K;
			}
			if(e.mirroringEnabled)
			{
				logMsg("uses mirroring");
				mirroringEnable = e.mirroringEnabled;
			}
			break;
		}
	}
	doMirroring(gba, mirroringEnable);
	if(cpuSaveType == GBA_SAVE_AUTO)
	{
		utilGBAFindSave(gba.mem.rom, romSize);
		logMsg("save type found from rom scan:%s", saveTypeStr(saveType));
	}
	else
	{
		saveType = cpuSaveType;
		if (flashSize == SIZE_FLASH512 || flashSize == SIZE_FLASH1M)
			flashSetSize(flashSize);
	}
	if(detectedRtcGame && (unsigned)optionRtcEmulation == RTC_EMU_AUTO)
	{
		rtcEnable(true);
	}
	else
	{
		bool rtcOn = (unsigned)optionRtcEmulation == RTC_EMU_ON;
		logMsg("forcing RTC:%s", rtcOn ? "on" : "off");
		rtcEnable(rtcOn);
	}
}
