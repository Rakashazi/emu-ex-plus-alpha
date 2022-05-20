#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

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
	CFGKEY_RTC_EMULATION = 256
};

void readCheatFile(EmuSystem &);

class GbaSystem final: public EmuSystem
{
public:
	Byte1Option optionRtcEmulation{CFGKEY_RTC_EMULATION, to_underlying(RtcMode::AUTO), 0, optionIsValidWithMax<2>};
	bool detectedRtcGame{};

	GbaSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setGameSpecificSettings(GBASys &gba, int romSize);
	void setRTC(RtcMode mode);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, IO &io, unsigned key, size_t readSize);
	void writeConfig(ConfigType, IO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	unsigned translateInputAction(unsigned input, bool &turbo);
	VController::Map vControllerMap(int player);
	void configAudioRate(FloatSeconds frameTime, int rate);

	// optional API functions
	bool resetSessionOptions(EmuApp &);
	void onFlushBackupMemory(BackupMemoryDirtyFlags);
	void closeSystem();
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	void renderFramebuffer(EmuVideo &);
};

using MainSystem = GbaSystem;

}

void CPULoop(GBASys &, EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *, EmuEx::EmuAudio *);
void CPUCleanUp();
bool CPUReadBatteryFile(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteBatteryFile(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUReadState(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteState(IG::ApplicationContext, GBASys &gba, const char *);
