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
#include <imagine/util/math/math.hh>
#include "MainSystem.hh"

struct GameSettings
{
	std::string_view gameName;
	std::string_view gameId;
	int saveSize;
	int saveType;
	bool rtcEnabled;
	bool mirroringEnabled;
	bool useBios;
};

constexpr GameSettings settings[]
{
#include "gba-over.inc"
};

int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
SystemColorMap systemColorMap;
uint32_t throttle{};
uint32_t speedup_throttle{};
uint32_t speedup_frame_skip{};
int emulating{};

using namespace EmuEx;

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

void systemUpdateMotionSensor() {}
int systemGetSensorX() { return static_cast<EmuEx::GbaSystem&>(gSystem()).sensorX; }
int systemGetSensorY() { return static_cast<EmuEx::GbaSystem&>(gSystem()).sensorY; }
int systemGetSensorZ() { return static_cast<EmuEx::GbaSystem&>(gSystem()).sensorZ; }
uint8_t systemGetSensorDarkness() { return static_cast<EmuEx::GbaSystem&>(gSystem()).darknessLevel; }

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

namespace EmuEx
{

const char *saveTypeStr(int type, int size)
{
	switch(type)
	{
		case GBA_SAVE_AUTO: return "Auto";
		case GBA_SAVE_EEPROM: return "EEPROM";
		case GBA_SAVE_SRAM: return "SRAM";
		case GBA_SAVE_FLASH: return size == SIZE_FLASH1M ? "Flash (128K)" : "Flash (64K)";
		case GBA_SAVE_EEPROM_SENSOR: return "EEPROM + Sensor";
		case GBA_SAVE_NONE: return "None";
	}
	return "Unknown";
}

bool saveMemoryHasContent()
{
	auto hasContent = [](std::span<uint8_t> mem)
	{
		for(auto v : mem)
		{
			if(v != 0xFF)
				return true;
		}
		return false;
	};
	switch(saveType)
	{
		case GBA_SAVE_EEPROM:
		case GBA_SAVE_EEPROM_SENSOR:
			return hasContent(eepromData);
		case GBA_SAVE_SRAM:
		case GBA_SAVE_FLASH:
			return hasContent(flashSaveMemory);
	}
	return false;
}

static void resetGameSettings()
{
	//agbPrintEnable(0);
	rtcEnable(0);
	flashSize = SIZE_FLASH512;
	eepromSize = SIZE_EEPROM_512;
}

void setSaveType(int type, int size)
{
	assert(type != GBA_SAVE_AUTO);
	saveType = type;
	switch(type)
	{
		case GBA_SAVE_EEPROM:
		case GBA_SAVE_EEPROM_SENSOR:
			eepromSize = size == SIZE_EEPROM_8K ? SIZE_EEPROM_8K : SIZE_EEPROM_512;
			break;
		case GBA_SAVE_SRAM:
			flashSize = SIZE_SRAM;
			break;
		case GBA_SAVE_FLASH:
			flashSetSize(size == SIZE_FLASH1M ? SIZE_FLASH1M : SIZE_FLASH512);
			break;
	}
}

static GbaSensorType detectSensorType(std::string_view gameId)
{
	static constexpr std::string_view tiltIds[]{"KHPJ", "KYGJ", "KYGE", "KYGP"};
	if(IG::contains(tiltIds, gameId))
	{
		logMsg("detected accelerometer sensor");
		return GbaSensorType::Accelerometer;
	}
	static constexpr std::string_view gyroIds[]{"RZWJ", "RZWE", "RZWP"};
	if(IG::contains(gyroIds, gameId))
	{
		logMsg("detected gyroscope sensor");
		return GbaSensorType::Gyroscope;
	}
	static constexpr std::string_view lightIds[]{"U3IJ", "U3IE", "U3IP",
		"U32J", "U32E", "U32P", "U33J"};
	if(IG::contains(lightIds, gameId))
	{
		logMsg("detected light sensor");
		return GbaSensorType::Light;
	}
	return GbaSensorType::None;
}

void GbaSystem::setGameSpecificSettings(GBASys &gba, int romSize)
{
	using namespace EmuEx;
	resetGameSettings();
	logMsg("game id:%c%c%c%c", gba.mem.rom[0xac], gba.mem.rom[0xad], gba.mem.rom[0xae], gba.mem.rom[0xaf]);
	GameSettings foundSettings{};
	std::string_view gameId{(char*)&gba.mem.rom[0xac], 4};
	if(auto it = std::ranges::find_if(settings, [&](const auto &s){return s.gameId == gameId;});
		it != std::end(settings))
	{
		foundSettings = *it;
		logMsg("found settings for:%s save type:%s save size:%d rtc:%d mirroring:%d",
			it->gameName.data(), saveTypeStr(it->saveType, it->saveSize), it->saveSize, it->rtcEnabled, it->mirroringEnabled);
	}
	detectedRtcGame = foundSettings.rtcEnabled;
	detectedSaveType = foundSettings.saveType;
	detectedSaveSize = foundSettings.saveSize;
	detectedSensorType = detectSensorType(gameId);
	doMirroring(gba, foundSettings.mirroringEnabled);
	if(detectedSaveType == GBA_SAVE_AUTO)
	{
		utilGBAFindSave(gba.mem.rom, romSize);
		detectedSaveType = saveType;
		detectedSaveSize = saveType == GBA_SAVE_FLASH ? flashSize : 0;
		logMsg("save type found from rom scan:%s", saveTypeStr(detectedSaveType, detectedSaveSize));
	}
	if(auto [type, size] = saveTypeOverride();
		type != GBA_SAVE_AUTO)
	{
		setSaveType(type, size);
		logMsg("save type override:%s", saveTypeStr(type, size));
	}
	else
	{
		setSaveType(detectedSaveType, detectedSaveSize);
	}
	setRTC((RtcMode)optionRtcEmulation.val);
}

void GbaSystem::setSensorActive(bool on)
{
	auto ctx = appContext();
	auto typeToSet = sensorType;
	if(sensorType == GbaSensorType::Auto)
		typeToSet = detectedSensorType;
	if(!on)
	{
		sensorListener = {};
	}
	else if(typeToSet == GbaSensorType::Accelerometer)
	{
		sensorListener = IG::SensorListener{ctx, IG::SensorType::Accelerometer, [this, ctx](SensorValues vals)
		{
			vals = ctx.remapSensorValuesForDeviceRotation(vals);
			sensorX = IG::remap(vals[0], -9.807f, 9.807f, 1897, 2197);
			sensorY = IG::remap(vals[1], -9.807f, 9.807f, 2197, 1897);
			//logDMsg("updated accel: %d,%d", sensorX, sensorY);
		}};
	}
	else if(typeToSet == GbaSensorType::Gyroscope)
	{
		sensorListener = IG::SensorListener{ctx, IG::SensorType::Gyroscope, [this, ctx](SensorValues vals)
		{
			vals = ctx.remapSensorValuesForDeviceRotation(vals);
			sensorZ = IG::remap(vals[2], -20.f, 20.f, 1800, -1800);
			//logDMsg("updated gyro: %d", sensorZ);
		}};
	}
	else if(typeToSet == GbaSensorType::Light)
	{
		sensorListener = IG::SensorListener{ctx, IG::SensorType::Light, [this](SensorValues vals)
		{
			if(!lightSensorScaleLux)
				darknessLevel = 0;
			else
				darknessLevel = IG::remapClamp(vals[0], lightSensorScaleLux, 0.f, std::numeric_limits<decltype(darknessLevel)>{});
			//logDMsg("updated light: %u", darknessLevel);
		}};
	}
}

void GbaSystem::clearSensorValues()
{
	sensorX = sensorY = sensorZ = 0;
}

}

size_t saveMemorySize()
{
	if(!saveType || saveType == GBA_SAVE_NONE)
		return 0;
  if (saveType == GBA_SAVE_FLASH) {
  	return flashSize;
  } else if (saveType == GBA_SAVE_SRAM) {
  	return 0x8000;
  }
  // eeprom case
  return eepromSize;
}

void setSaveMemory(IG::ByteBuffer buff)
{
  if(!saveType || saveType == GBA_SAVE_NONE)
    return;
	assert(buff.size() == saveMemorySize());
  if (saveType == GBA_SAVE_FLASH || saveType == GBA_SAVE_SRAM) {
  	flashSaveMemory = std::move(buff);
  } else { // eeprom case
  	eepromData = std::move(buff);
  }
}
