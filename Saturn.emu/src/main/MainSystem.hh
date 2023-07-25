#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

extern "C"
{
	#include <yabause/yabause.h>
	#include <yabause/sh2core.h>
	#include <yabause/peripheral.h>
}

namespace EmuEx::Controls
{
static const unsigned gamepadKeys = 23;
}

extern const int defaultSH2CoreID;
extern SH2Interface_struct *SH2CoreList[];

namespace EmuEx
{

extern Byte1Option optionSH2Core;
extern FS::PathString biosPath;
extern unsigned SH2Cores;
extern yabauseinit_struct yinit;
extern PerPad_struct *pad[2];

class SaturnSystem final: public EmuSystem
{
public:
	SaturnSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		yinit.cdpath = contentLocationPtr();
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".yss"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return fromHz<FrameTime>(60.); }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	void onOptionsLoaded();
};

using MainSystem = SaturnSystem;

bool hasBIOSExtension(std::string_view name);

}
