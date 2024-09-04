#pragma once

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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/base/Sensor.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/enum.hh>
#include <core/gba/gba.h>
#include <core/gba/gbaCheats.h>

namespace IG
{
class ApplicationContext;
}

struct GBASys;

namespace EmuEx
{

enum class RtcMode : uint8_t {AUTO, OFF, ON};

enum
{
	CFGKEY_RTC_EMULATION = 256, CFGKEY_SAVE_TYPE_OVERRIDE = 257,
	CFGKEY_PCM_VOLUME = 258, CFGKEY_GB_APU_VOLUME = 259,
	CFGKEY_SOUND_FILTERING = 260, CFGKEY_SOUND_INTERPOLATION = 261,
	CFGKEY_SENSOR_TYPE = 262, CFGKEY_LIGHT_SENSOR_SCALE = 263,
	CFGKEY_CHEATS_PATH = 264, CFGKEY_PATCHES_PATH = 265,
	CFGKEY_USE_BIOS = 266, CFGKEY_DEFAULT_USE_BIOS = 267,
	CFGKEY_BIOS_PATH = 268
};

void setSaveType(int type, int size);
const char *saveTypeStr(int type, int size);
bool saveMemoryHasContent();
int soundVolumeAsInt(GBASys &, bool gbVol);
int soundFilteringAsInt(GBASys &);

constexpr uint32_t packSaveTypeOverride(int type, int size = 0) { return (type << 24) | (size & 0xFFFFFF); }
constexpr std::pair<int, int> unpackSaveTypeOverride(uint32_t val) { return {val >> 24, val & 0xFFFFFF}; }

constexpr bool optionSaveTypeOverrideIsValid(const auto &val)
{
	auto [type, size] = unpackSaveTypeOverride(val);
	return type >= GBA_SAVE_AUTO && type <= GBA_SAVE_NONE;
}

WISE_ENUM_CLASS((GbaSensorType, uint8_t),
	Auto, None, Accelerometer, Gyroscope, Light);

constexpr float lightSensorScaleLuxDefault = 10000.f;
constexpr uint8_t darknessLevelDefault = 0xee;

class Cheat: public CheatsData {};
class CheatCode: public CheatsData {};

class GbaSystem final: public EmuSystem
{
public:
	std::string cheatsDir;
	std::string patchesDir;
	std::string biosPath;
	[[no_unique_address]] IG::SensorListener sensorListener;
	Property<RtcMode, CFGKEY_RTC_EMULATION,
		PropertyDesc<RtcMode>{.defaultValue = RtcMode::AUTO, .isValid = isValidWithMax<RtcMode::ON>}> optionRtcEmulation;
	Property<uint32_t, CFGKEY_SAVE_TYPE_OVERRIDE,
		PropertyDesc<uint32_t>{.defaultValue = GBA_SAVE_AUTO, .isValid = optionSaveTypeOverrideIsValid}> optionSaveTypeOverride;
	FileIO saveFileIO;
	static constexpr size_t maxStateSize{0x1FFFFF};
	size_t saveStateSize{};
	int detectedSaveSize{};
	int sensorX{}, sensorY{}, sensorZ{};
	float lightSensorScaleLux{lightSensorScaleLuxDefault};
	uint8_t darknessLevel{darknessLevelDefault};
	uint8_t detectedSaveType{};
	bool detectedRtcGame{};
	bool saveMemoryIsMappedFile{};
	Property<AutoTristate, CFGKEY_USE_BIOS> useBios;
	Property<bool, CFGKEY_DEFAULT_USE_BIOS> defaultUseBios;
	ConditionalMember<Config::SENSORS, GbaSensorType> sensorType{};
	ConditionalMember<Config::SENSORS, GbaSensorType> detectedSensorType{};
	static constexpr auto gbaFrameTime{fromSeconds<FrameTime>(280896. / 16777216.)}; // ~59.7275Hz

	GbaSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setGameSpecificSettings(GBASys &gba, int romSize);
	void setRTC(RtcMode mode);
	std::pair<int, int> saveTypeOverride() { return unpackSaveTypeOverride(optionSaveTypeOverride); }
	void setSaveTypeOverride(int type, int size) { optionSaveTypeOverride = packSaveTypeOverride(type, size); };
	void setSensorActive(bool);
	void setSensorType(GbaSensorType);
	void clearSensorValues();

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".gqs"; }
	size_t stateSize() { return saveStateSize; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return gbaFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void onStart();
	void onStop();
	bool resetSessionOptions(EmuApp &);
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	void closeSystem();
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	void renderFramebuffer(EmuVideo &);
	Cheat* newCheat(EmuApp&, const char* name, CheatCodeDesc);
	bool setCheatName(Cheat&, const char* name);
	std::string_view cheatName(const Cheat&) const;
	void setCheatEnabled(Cheat&, bool on);
	bool isCheatEnabled(const Cheat&) const;
	bool addCheatCode(EmuApp&, Cheat*&, CheatCodeDesc);
	Cheat* removeCheatCode(Cheat&, CheatCode&);
	bool removeCheat(Cheat&);
	void forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)>);
	void forEachCheatCode(Cheat&, DelegateFunc<bool(CheatCode&, std::string_view)>);

private:
	void readCheatFile();
	void writeCheatFile();
	void applyGamePatches(uint8_t *rom, int &romSize);
	bool shouldUseBios() const
	{
		switch(useBios)
		{
			case AutoTristate::Auto: return defaultUseBios && biosPath.size();
			case AutoTristate::On: return biosPath.size();
			case AutoTristate::Off: return false;
		}
		std::unreachable();
	}
};

using MainSystem = GbaSystem;

}

void CPULoop(GBASys &, EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *, EmuEx::EmuAudio *);
void CPUCleanUp();
bool CPUReadState(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteState(IG::ApplicationContext, GBASys &gba, const char *);
