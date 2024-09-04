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

#include <core/gba/gba.h>
#include <core/gba/gbaSound.h>
#include <core/gba/gbaRtc.h>
#include <core/gba/gbaEeprom.h>
#include <core/gba/gbaFlash.h>
#include <core/gba/gbaCheats.h>
#include <core/gba/gbaGfx.h>
#include <core/base/sound_driver.h>
#include <core/base/file_util.h>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/math.hh>
#include <imagine/io/IO.hh>
#include "MainSystem.hh"
#include "GBASys.hh"

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

#ifdef VBAM_ENABLE_DEBUGGER
int  oldreg[18];
char oldbuffer[10];
#endif

// this is an optional hack to change the backdrop/background color:
// -1: disabled
// 0x0000 to 0x7FFF: set custom 15 bit color
//int customBackdropColor = -1;

extern int romSize;
int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
SystemColorMap systemColorMap;
int emulating{};
CoreOptions coreOptions
{
	.saveType = GBA_SAVE_NONE
};

void CPUUpdateRenderBuffers(GBASys &gba, bool force);

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
	switch(coreOptions.saveType)
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
	g_flashSize = SIZE_FLASH512;
	eepromSize = SIZE_EEPROM_512;
}

void setSaveType(int type, int size)
{
	assert(type != GBA_SAVE_AUTO);
	coreOptions.saveType = type;
	switch(type)
	{
		case GBA_SAVE_EEPROM:
		case GBA_SAVE_EEPROM_SENSOR:
			eepromSize = size == SIZE_EEPROM_8K ? SIZE_EEPROM_8K : SIZE_EEPROM_512;
			break;
		case GBA_SAVE_SRAM:
			g_flashSize = SIZE_SRAM;
			break;
		case GBA_SAVE_FLASH:
			flashSetSize(size == SIZE_FLASH1M ? SIZE_FLASH1M : SIZE_FLASH512);
			break;
	}
}

static GbaSensorType detectSensorType(std::string_view gameId)
{
	static constexpr std::string_view tiltIds[]{"KHPJ", "KYGJ", "KYGE", "KYGP"};
	if(std::ranges::contains(tiltIds, gameId))
	{
		logMsg("detected accelerometer sensor");
		return GbaSensorType::Accelerometer;
	}
	static constexpr std::string_view gyroIds[]{"RZWJ", "RZWE", "RZWP"};
	if(std::ranges::contains(gyroIds, gameId))
	{
		logMsg("detected gyroscope sensor");
		return GbaSensorType::Gyroscope;
	}
	static constexpr std::string_view lightIds[]{"U3IJ", "U3IE", "U3IP",
		"U32J", "U32E", "U32P", "U33J"};
	if(std::ranges::contains(lightIds, gameId))
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
		flashDetectSaveType(gba.mem.rom, romSize);
		detectedSaveType = coreOptions.saveType;
		detectedSaveSize = coreOptions.saveType == GBA_SAVE_FLASH ? g_flashSize : 0;
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
	setRTC(optionRtcEmulation);
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

void preLoadRomSetup(GBASys &gba)
{
  romSize = SIZE_ROM;
  /*if (rom != NULL) {
    CPUCleanUp();
  }*/

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

  memset(gba.mem.workRAM, 0, sizeof(gba.mem.workRAM));
}

void postLoadRomSetup(GBASys &gba)
{
  uint16_t *temp = (uint16_t *)(gba.mem.rom+((romSize+1)&~1));
  int i;
  for (i = (romSize+1)&~1; i < 0x2000000; i+=2) {
    WRITE16LE(temp, (i >> 1) & 0xFFFF);
    temp++;
  }

  memset(gba.mem.bios, 0, sizeof(gba.mem.bios));

  memset(gba.mem.internalRAM, 0, sizeof(gba.mem.internalRAM));

  memset(gba.mem.ioMem.b, 0, sizeof(gba.mem.ioMem));

  gba.lcd.reset();

  flashInit();
  eepromInit();

  CPUUpdateRenderBuffers(gba, true);
}

int CPULoadRomWithIO(GBASys &gba, IG::IO &io)
{
	preLoadRomSetup(gba);
	romSize = io.read(gba.mem.rom, romSize);
  postLoadRomSetup(gba);
  return romSize;
}

size_t saveMemorySize()
{
	if(!coreOptions.saveType || coreOptions.saveType == GBA_SAVE_NONE)
		return 0;
  if (coreOptions.saveType == GBA_SAVE_FLASH) {
  	return g_flashSize;
  } else if (coreOptions.saveType == GBA_SAVE_SRAM) {
  	return 0x8000;
  }
  // eeprom case
  return eepromSize;
}

void setSaveMemory(IG::ByteBuffer buff)
{
  if(!coreOptions.saveType || coreOptions.saveType == GBA_SAVE_NONE)
    return;
	assert(buff.size() == saveMemorySize());
  if (coreOptions.saveType == GBA_SAVE_FLASH || coreOptions.saveType == GBA_SAVE_SRAM) {
  	flashSaveMemory = std::move(buff);
  } else { // eeprom case
  	eepromData = std::move(buff);
  }
}

void utilWriteIntMem(uint8_t*& data, int val)
{
	memcpy(data, &val, sizeof(int));
	data += sizeof(int);
}

void utilWriteMem(uint8_t*& data, const void* in_data, unsigned size)
{
	memcpy(data, in_data, size);
	data += size;
}

void utilWriteDataMem(uint8_t*& data, const variable_desc* desc)
{
	while (desc->address)
	{
		utilWriteMem(data, desc->address, desc->size);
		desc++;
	}
}

int utilReadIntMem(const uint8_t*& data)
{
	int res;
	memcpy(&res, data, sizeof(int));
	data += sizeof(int);
	return res;
}

void utilReadMem(void* buf, const uint8_t*& data, unsigned size)
{
	memcpy(buf, data, size);
	data += size;
}

void utilReadDataMem(const uint8_t*& data, const variable_desc* desc)
{
	while (desc->address)
	{
		utilReadMem(desc->address, data, desc->size);
		desc++;
	}
}

void cheatsSaveGame(uint8_t*& data)
{
	utilWriteIntMem(data, 0);
	CheatsData cheat{};
	for([[maybe_unused]] auto i: iotaCount(100))
	{
		utilWriteMem(data, &cheat, sizeof(cheat));
	}
}

void cheatsReadGame(const uint8_t*& data)
{
  utilReadIntMem(data);
  CheatsData cheat{};
	for([[maybe_unused]] auto i: iotaCount(100))
	{
		utilReadMem(&cheat, data, sizeof(cheat));
	}
}

const char *dispModeName(GBALCD::RenderLineFunc renderLine)
{
	if (renderLine == mode0RenderLine) return "0";
	else if (renderLine == mode0RenderLineNoWindow) return "0NW";
	else if (renderLine == mode0RenderLineAll) return "0A";
	else if (renderLine == mode1RenderLine) return "1";
	else if (renderLine == mode1RenderLineNoWindow) return "1NW";
	else if (renderLine == mode1RenderLineAll) return "1A";
	else if (renderLine == mode2RenderLine) return "2";
	else if (renderLine == mode2RenderLineNoWindow) return "2NW";
	else if (renderLine == mode2RenderLineAll) return "2A";
	else if (renderLine == mode3RenderLine) return "3";
	else if (renderLine == mode3RenderLineNoWindow) return "3NW";
	else if (renderLine == mode3RenderLineAll) return "3A";
	else if (renderLine == mode4RenderLine) return "4";
	else if (renderLine == mode4RenderLineNoWindow) return "4NW";
	else if (renderLine == mode4RenderLineAll) return "4A";
	else if (renderLine == mode5RenderLine) return "5";
	else if (renderLine == mode5RenderLineNoWindow) return "5NW";
	else if (renderLine == mode5RenderLineAll) return "5A";
	else return "Invalid";
}
