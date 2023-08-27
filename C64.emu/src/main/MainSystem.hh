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
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
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

class EmuAudio;

enum
{
	CFGKEY_DRIVE_TRUE_EMULATION = 256, CFGKEY_AUTOSTART_WARP = 257,
	CFGKEY_AUTOSTART_TDE = 258, CFGKEY_C64_MODEL = 259,
	CFGKEY_BORDER_MODE = 260, CFGKEY_SWAP_JOYSTICK_PORTS = 261,
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
};

enum Vic20Ram : uint8_t
{
	BLOCK_0 = 1,
	BLOCK_1 = 1 << 1,
	BLOCK_2 = 1 << 2,
	BLOCK_3 = 1 << 3,
	BLOCK_5 = 1 << 5
};

enum JoystickMode : uint8_t
{
	NORMAL = 0,
	SWAPPED = 1,
	KEYBOARD = 2,
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
	VicePlugin plugin{};
	std::string defaultPaletteName{};
	std::string lastMissingSysFile;
	IG::PixmapView canvasSrcPix{};
	PixelFormat pixFmt{};
	ViceSystem currSystem{};
	std::atomic_bool runningFrame{};
	bool ctrlLock{};
	bool c64IsInit{}, c64FailedInit{};
	std::array <FS::PathString, Config::envIsLinux ? 3 : 1> sysFilePath{};
	std::array<char, 21> externalPaletteResStr{};
	std::array<char, 17> paletteFileResStr{};
	Byte1Option optionDriveTrueEmulation{CFGKEY_DRIVE_TRUE_EMULATION, 0};
	Byte1Option optionCropNormalBorders{CFGKEY_CROP_NORMAL_BORDERS, 1};
	Byte1Option optionAutostartWarp{CFGKEY_AUTOSTART_WARP, 1};
	Byte1Option optionAutostartTDE{CFGKEY_AUTOSTART_TDE, 0};
	Byte1Option optionAutostartBasicLoad{CFGKEY_AUTOSTART_BASIC_LOAD, 0};
	Byte1Option optionViceSystem{CFGKEY_VICE_SYSTEM, std::to_underlying(ViceSystem::C64), false,
		optionIsValidWithMax<VicePlugin::SYSTEMS-1, uint8_t>};
	SByte1Option optionModel{};
	SByte1Option optionDefaultModel{};
	Byte1Option optionBorderMode{CFGKEY_BORDER_MODE, VICII_NORMAL_BORDERS};
	Byte1Option optionSidEngine{CFGKEY_SID_ENGINE, SID_ENGINE_RESID, false,
		optionIsValidWithMax<1, uint8_t>};
	Byte1Option optionReSidSampling{CFGKEY_RESID_SAMPLING, SID_RESID_SAMPLING_INTERPOLATION, false,
		optionIsValidWithMax<3, uint8_t>};
	Byte1Option optionSwapJoystickPorts{CFGKEY_SWAP_JOYSTICK_PORTS, JoystickMode::NORMAL, false,
		optionIsValidWithMax<JoystickMode::KEYBOARD>};
	Byte1Option optionAutostartOnLaunch{CFGKEY_AUTOSTART_ON_LOAD, 1};
	// VIC-20 specific
	Byte1Option optionVic20RamExpansions{CFGKEY_VIC20_RAM_EXPANSIONS, 0};
	// C64 specific
	Byte2Option optionC64RamExpansionModule{CFGKEY_C64_RAM_EXPANSION_MODULE, 0};

	C64System(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		makeDetachedThread(
			[this]()
			{
				execSem.acquire();
				logMsg("starting maincpu_mainloop()");
				plugin.maincpu_mainloop();
			});

		if(sysFilePath.size() == 3)
		{
			sysFilePath[1] = "~/.local/share/C64.emu";
			sysFilePath[2] = "/usr/share/games/vice";
		}

		// higher quality ReSID sampling modes take orders of magnitude more CPU power,
		// set some reasonable defaults based on CPU type
		#if defined __x86_64__
		optionReSidSampling.initDefault(SID_RESID_SAMPLING_RESAMPLING);
		#elif defined __aarch64__
		optionReSidSampling.initDefault(SID_RESID_SAMPLING_INTERPOLATION);
		#else
		optionReSidSampling.initDefault(SID_RESID_SAMPLING_FAST);
		#endif
	}

	int intResource(const char *name) const;
	void setIntResource(const char *name, int val);
	void resetIntResource(const char *name);
	int defaultIntResource(const char *name) const;
	const char *stringResource(const char *name) const;
	void setStringResource(const char *name, const char *val);
	void setBorderMode(int mode);
	void setSidEngine(int engine);
	void setReSidSampling(int sampling);
	void setDriveTrueEmulation(bool on);
	bool driveTrueEmulation() const;
	void setAutostartWarp(bool on);
	void setAutostartTDE(bool on);
	void setAutostartBasicLoad(bool on);
	bool autostartBasicLoad() const;
	void setSysModel(int model);
	int sysModel() const;
	void setDefaultModel(int model);
	void applySessionOptions();
	std::vector<std::string> systemFilesWithExtension(const char *ext) const;
	const char *videoChipStr() const;
	void setPaletteResources(const char *palName);
	bool usingExternalPalette() const;
	const char *externalPaletteName() const;
	const char *paletteName() const;
	void setJoystickMode(JoystickMode);
	bool currSystemIsC64() const;
	bool currSystemIsC64Or128() const;
	void setRuntimeReuSize(int size);
	void resetCanvasSourcePixmap(struct video_canvas_s *c);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".vsf"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &io, unsigned key, size_t readSize);
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

protected:
	void initC64(EmuApp &app);
	int reSidSampling() const;
	void setVirtualDeviceTraps(bool on);
	bool virtualDeviceTraps() const;
	int sidEngine() const;
	bool autostartWarp() const;
	bool autostartTDE() const;
	int borderMode() const;
	void handleKeyboardInput(InputAction, bool positionalShift = {});
	void setModel(int model);
	void applyInitialOptionResources();
	void execC64Frame();
	void startCanvasRunningFrame();
	void setCanvasSkipFrame(bool on);
	bool updateCanvasPixelFormat(struct video_canvas_s *, PixelFormat);
	void tryLoadingSplitVic20Cart();
};

using MainSystem = C64System;

inline C64System &gC64System() { return static_cast<C64System&>(gSystem()); }

}
