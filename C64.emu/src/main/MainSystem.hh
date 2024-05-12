#pragma once

/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "VicePlugin.hh"
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <vector>
#include <string>
#include <string_view>
#include <atomic>

extern "C"
{
	#include "vicii.h"
	#include "sid/sid.h"
	#include "sid/sid-resources.h"
}

namespace EmuEx
{

enum JoystickMode : uint8_t
{
	Auto, Port1, Port2, Keyboard
};

constexpr auto toString(JoystickMode v)
{
	using enum JoystickMode;
	switch(v)
	{
		case Auto: return "Auto";
		case Port1: return "Port 1";
		case Port2: return "Port 2";
		case Keyboard: return "Keyboard Cursor";
	}
}

}


namespace IG
{

template<>
constexpr bool isValidProperty(const EmuEx::JoystickMode &v) { return v <= EmuEx::JoystickMode::Keyboard; }

}

namespace EmuEx
{

class EmuAudio;

enum
{
	CFGKEY_DRIVE_TRUE_EMULATION = 256, CFGKEY_AUTOSTART_WARP = 257,
	CFGKEY_AUTOSTART_TDE = 258, CFGKEY_C64_MODEL = 259,
	CFGKEY_BORDER_MODE = 260, CFGKEY_JOYSTICK_MODE = 261,
	CFGKEY_SID_ENGINE = 262, CFGKEY_CROP_NORMAL_BORDERS = 263,
	CFGKEY_SYSTEM_FILE_PATH = 264, CFGKEY_DTV_MODEL = 265,
	CFGKEY_C128_MODEL = 266, CFGKEY_SUPER_CPU_MODEL = 267,
	CFGKEY_CBM2_MODEL = 268, CFGKEY_CBM5x0_MODEL = 269,
	CFGKEY_PET_MODEL = 270, CFGKEY_PLUS4_MODEL = 271,
	CFGKEY_VIC20_MODEL = 272, CFGKEY_VICE_SYSTEM = 273,
	CFGKEY_VIRTUAL_DEVICE_TRAPS = 274, CFGKEY_RESID_SAMPLING = 275,
	CFGKEY_MODEL = 276, CFGKEY_AUTOSTART_BASIC_LOAD = 277,
	CFGKEY_VIC20_RAM_EXPANSIONS = 278, CFGKEY_AUTOSTART_ON_LOAD = 279,
	CFGKEY_PALETTE_NAME = 280, CFGKEY_C64_RAM_EXPANSION_MODULE = 281,
	CFGKEY_DEFAULT_MODEL = 282, CFGKEY_DEFAULT_PALETTE_NAME = 283,
	CFGKEY_DRIVE8_TYPE = 284, CFGKEY_DRIVE9_TYPE = 285,
	CFGKEY_DRIVE10_TYPE = 286, CFGKEY_DRIVE11_TYPE = 287,
	CFGKEY_DEFAULT_DRIVE_TRUE_EMULATION = 288, CFGKEY_COLOR_SATURATION = 289,
	CFGKEY_COLOR_CONTRAST = 290, CFGKEY_COLOR_BRIGHTNESS = 291,
	CFGKEY_COLOR_GAMMA = 292, CFGKEY_COLOR_TINT = 293,
	CFGKEY_DEFAULT_JOYSTICK_MODE = 294
};

enum Vic20Ram : uint8_t
{
	BLOCK_0 = 1,
	BLOCK_1 = 1 << 1,
	BLOCK_2 = 1 << 2,
	BLOCK_3 = 1 << 3,
	BLOCK_5 = 1 << 5
};

enum class ColorSetting
{
	Saturation, Contrast, Brightness, Gamma, Tint
};

constexpr uint8_t SYSTEM_FLAG_NO_AUTOSTART = bit(0);

bool hasC64DiskExtension(std::string_view name);
bool hasC64TapeExtension(std::string_view name);
bool hasC64CartExtension(std::string_view name);
int systemCartType(ViceSystem system);

class C64System final: public EmuSystem
{
public:
	double systemFrameRate{60.};
	std::binary_semaphore execSem{0}, execDoneSem{0};
	EmuAudio *audioPtr{};
	struct video_canvas_s *activeCanvas{};
	const char *sysFileDir{};
	ThreadId emuThreadId{};
	VicePlugin plugin{};
	mutable ArchiveIO firmwareArch;
	std::string defaultPaletteName{};
	std::string lastMissingSysFile;
	IG::PixmapView canvasSrcPix{};
	PixelFormat pixFmt{PixelFmtRGBA8888};
	ViceSystem currSystem{};
	bool viceThreadSignaled{};
	bool inCPUTrap{};
	Property<JoystickMode, CFGKEY_DEFAULT_JOYSTICK_MODE,
		PropertyDesc<JoystickMode>{.defaultValue = JoystickMode::Port2}> defaultJoystickMode;
	Property<JoystickMode, CFGKEY_JOYSTICK_MODE,
		PropertyDesc<JoystickMode>{.defaultValue = JoystickMode::Auto}> joystickMode;
	JoystickMode effectiveJoystickMode{};
	bool ctrlLock{};
	bool c64IsInit{}, c64FailedInit{};
	std::array <FS::PathString, Config::envIsLinux ? 3 : 1> sysFilePath{};
	std::array<char, 21> externalPaletteResStr{};
	std::array<char, 17> paletteFileResStr{};
	std::string colorSettingResStr[5];
	bool defaultDriveTrueEmulation = true;
	bool optionCropNormalBorders = true;
	ViceSystem optionViceSystem{ViceSystem::C64};
	int8_t defaultModel{};
	bool optionAutostartOnLaunch{true};

	// higher quality ReSID sampling modes take orders of magnitude more CPU power,
	// set some reasonable defaults based on CPU type
	#if defined __aarch64__ || defined __x86_64__
	static constexpr uint8_t defaultReSidSampling = SID_RESID_SAMPLING_INTERPOLATION;
	#else
	static constexpr uint8_t defaultReSidSampling = SID_RESID_SAMPLING_FAST;
	#endif

	C64System(ApplicationContext ctx);
	int intResource(const char *name) const;
	void setIntResource(const char *name, int val);
	bool updateIntResourceInCPUTrap(const char *name, int val);
	void resetIntResource(const char *name);
	int defaultIntResource(const char *name) const;
	const char *stringResource(const char *name) const;
	void setStringResource(const char *name, const char *val);
	void setBorderMode(int mode);
	int borderMode() const;
	void setSidEngine(int engine);
	int sidEngine() const;
	void setReSidSampling(int sampling);
	int reSidSampling() const;
	void setDriveTrueEmulation(bool on);
	bool driveTrueEmulation() const;
	void setAutostartWarp(bool on);
	bool autostartWarp() const;
	void setAutostartTDE(bool on);
	bool autostartTDE() const;
	void setModel(int model);
	int model() const;
	void setDriveType(int idx, int type);
	int driveType(int idx);
	int colorSetting(ColorSetting) const;
	std::string colorSettingAsString(ColorSetting) const;
	void setColorSetting(ColorSetting, int);

	std::vector<std::string> systemFilesWithExtension(const char *ext) const;
	const char *videoChipStr() const;
	void setPaletteResources(const char *palName);
	bool usingExternalPalette() const;
	const char *externalPaletteName() const;
	const char *paletteName() const;
	void setJoystickMode(JoystickMode);
	void updateJoystickDevices();
	bool currSystemIsC64() const;
	bool currSystemIsC64Or128() const;
	void setReuSize(int size);
	void resetCanvasSourcePixmap(struct video_canvas_s *c);
	ArchiveIO &firmwareArchive(CStringView path) const;
	void setSystemFilesPath(CStringView path, FS::file_type);
	void enterCPUTrap();

	void signalViceThreadAndWait()
	{
		assert(!viceThreadSignaled);
		viceThreadSignaled = true;
		execSem.release();
		execDoneSem.acquire();
	}

	bool signalEmuTaskThreadAndWait()
	{
		if(!viceThreadSignaled)
			return false;
		viceThreadSignaled = false;
		execDoneSem.release();
		execSem.acquire();
		return true;
	}


	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".vsf"; }
	size_t stateSize();
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &io, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return fromHz<FrameTime>(systemFrameRate); }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	FS::FileString configName() const;
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	VController::KbMap vControllerKeyboardMap(VControllerKbMode mode);
	void onVKeyboardShown(VControllerKeyboard &, bool shown);
	VideoSystem videoSystem() const;
	void closeSystem();
	void renderFramebuffer(EmuVideo &);
	bool shouldFastForward() const;
	bool onVideoRenderFormatChange(EmuVideo &, PixelFormat);
	void addThreadGroupIds(std::vector<ThreadId> &ids) const { ids.emplace_back(emuThreadId); }

protected:
	void initC64(EmuApp &app);
	void setVirtualDeviceTraps(bool on);
	bool virtualDeviceTraps() const;
	void handleKeyboardInput(InputAction, bool positionalShift = {});
	void setCanvasSkipFrame(bool on);
	bool updateCanvasPixelFormat(struct video_canvas_s *, PixelFormat);
	void tryLoadingSplitVic20Cart();
};

using MainSystem = C64System;

inline C64System &gC64System() { return static_cast<C64System&>(gSystem()); }

}
