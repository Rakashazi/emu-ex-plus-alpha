#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/base/Sensor.hh>
#include <imagine/util/enum.hh>
#include <vbam/gba/GBA.h>

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
};

void readCheatFile(class EmuSystem &);
void setSaveType(int type, int size);
const char *saveTypeStr(int type, int size);
bool saveMemoryHasContent();
int soundVolumeAsInt(GBASys &, bool gbVol);
int soundFilteringAsInt(GBASys &);

constexpr uint32_t packSaveTypeOverride(int type, int size = 0) { return (type << 24) | (size & 0xFFFFFF); }
constexpr std::pair<int, int> unpackSaveTypeOverride(uint32_t val) { return {val >> 24, val & 0xFFFFFF}; }

constexpr bool optionSaveTypeOverrideIsValid(uint32_t val)
{
	auto [type, size] = unpackSaveTypeOverride(val);
	return type >= GBA_SAVE_AUTO && type <= GBA_SAVE_NONE;
}

WISE_ENUM_CLASS((GbaSensorType, uint8_t),
	Auto, None, Accelerometer, Gyroscope, Light);

constexpr float lightSensorScaleLuxDefault = 10000.f;
constexpr uint8_t darknessLevelDefault = 0xee;

class GbaSystem final: public EmuSystem
{
public:
	std::string cheatsDir;
	std::string patchesDir;
	[[no_unique_address]] IG::SensorListener sensorListener;
	Byte1Option optionRtcEmulation{CFGKEY_RTC_EMULATION, std::to_underlying(RtcMode::AUTO), 0, optionIsValidWithMax<2>};
	Byte4Option optionSaveTypeOverride{CFGKEY_SAVE_TYPE_OVERRIDE, GBA_SAVE_AUTO, 0, optionSaveTypeOverrideIsValid};
	FileIO saveFileIO;
	int detectedSaveSize{};
	int sensorX{}, sensorY{}, sensorZ{};
	float lightSensorScaleLux{lightSensorScaleLuxDefault};
	uint8_t darknessLevel{darknessLevelDefault};
	uint8_t detectedSaveType{};
	bool detectedRtcGame{};
	IG_UseMemberIf(Config::SENSORS, GbaSensorType, sensorType){};
	IG_UseMemberIf(Config::SENSORS, GbaSensorType, detectedSensorType){};
	static constexpr auto gbaFrameTime{fromSeconds<FrameTime>(280896. / 16777216.)}; // ~59.7275Hz

	GbaSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setGameSpecificSettings(GBASys &gba, int romSize);
	void setRTC(RtcMode mode);
	std::pair<int, int> saveTypeOverride() { return unpackSaveTypeOverride(optionSaveTypeOverride.val); }
	void setSaveTypeOverride(int type, int size) { optionSaveTypeOverride = packSaveTypeOverride(type, size); };
	void setSensorActive(bool);
	void setSensorType(GbaSensorType);
	void clearSensorValues();

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".gqs"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
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

private:
	void applyGamePatches(uint8_t *rom, int &romSize);
};

using MainSystem = GbaSystem;

}

void CPULoop(GBASys &, EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *, EmuEx::EmuAudio *);
void CPUCleanUp();
bool CPUReadState(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteState(IG::ApplicationContext, GBASys &gba, const char *);
