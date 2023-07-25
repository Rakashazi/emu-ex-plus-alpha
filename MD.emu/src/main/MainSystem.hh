#pragma once

#include <emuframework/EmuSystem.hh>
#include <emuframework/Option.hh>
#include "genplus-config.h"
#include "system.h"

extern t_config config;

namespace EmuEx::Controls
{
extern const int gamepadKeys;
}

namespace EmuEx
{

class EmuApp;

enum
{
	CFGKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_SMS_FM = 279,
	CFGKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283,
	CFGKEY_MD_REGION = 284, CFGKEY_VIDEO_SYSTEM = 285,
	CFGKEY_INPUT_PORT_1 = 286, CFGKEY_INPUT_PORT_2 = 287,
	CFGKEY_MULTITAP = 288, CFGKEY_CHEATS_PATH = 289,
};

bool hasMDExtension(std::string_view name);

class MdSystem final: public EmuSystem
{
public:
	std::string cheatsDir;
	int playerIdxMap[4]{};
	int gunDevIdx{};
	int8_t mdInputPortDev[2]{-1, -1};
	int8_t autoDetectedVidSysPAL{};
	int8_t savedVControllerPlayer = -1;
	Byte1Option optionBigEndianSram{CFGKEY_BIG_ENDIAN_SRAM, 0};
	Byte1Option optionSmsFM{CFGKEY_SMS_FM, 1};
	Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
	Byte1Option optionMultiTap{CFGKEY_MULTITAP, 0};
	SByte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, -1, false, optionIsValidWithMinMax<-1, 4>};
	SByte1Option optionInputPort2{CFGKEY_INPUT_PORT_2, -1, false, optionIsValidWithMinMax<-1, 4>};
	Byte1Option optionRegion{CFGKEY_MD_REGION, 0, false, optionIsValidWithMax<4>};
	Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<2>};
	#ifndef NO_SCD
	FS::PathString cdBiosUSAPath{}, cdBiosJpnPath{}, cdBiosEurPath{};
	#endif
	static constexpr auto ntscFrameTime{fromSeconds<FrameTime>(262. * MCYCLES_PER_LINE / 53693175.)}; // ~59.92Hz
	static constexpr auto palFrameTime{fromSeconds<FrameTime>(313. * MCYCLES_PER_LINE / 53203424.)}; // ~49.70Hz

	MdSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setupInput(EmuApp &);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".gp"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return videoSystem() == VideoSystem::PAL ? palFrameTime : ntscFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	void closeSystem();
	bool resetSessionOptions(EmuApp &);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState dragState,
		Input::DragTrackerState prevDragState, IG::WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	VideoSystem videoSystem() const;

private:
	void setupSmsInput(EmuApp &);
	void setupMdInput(EmuApp &);
};

using MainSystem = MdSystem;

}
