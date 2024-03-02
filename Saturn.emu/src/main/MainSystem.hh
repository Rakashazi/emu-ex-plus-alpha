#pragma once

/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/EmuSystem.hh>
#include <mednafen/mednafen.h>
#include <ss/ss.h>
#include <ss/smpc.h>
#include <ss/cart.h>

extern const Mednafen::MDFNGI EmulatedSS;

namespace Mednafen
{
class CDInterface;
}

namespace MDFN_IEN_SS
{
extern Mednafen::CDInterface* Cur_CDIF;
extern IG::ThreadId RThreadId;
extern const int ActiveCartType;
extern uint8 AreaCode;
}

namespace MDFN_IEN_SS::VDP2
{
extern const uint8 InterlaceMode;
extern const bool PAL;
}

namespace EmuEx
{

class EmuApp;

enum class InputDeviceType : int8_t
{
	none = -1,
	gamepad,
	multipad,
	mouse,
	wheel,
	mission,
	dmission,
	gun,
	keyboard,
	jpkeyboard
};

enum class WidescreenMode : uint8_t
{
	Auto, On, Off
};

struct InputConfig
{
	std::array<bool, 2> multitaps{};
	std::array<InputDeviceType, 12> devs{};
	constexpr bool operator ==(InputConfig const&) const = default;
};

enum
{
	CFGKEY_NA_BIOS_PATH = 275, CFGKEY_JP_BIOS_PATH = 276,
	CFGKEY_KOF_ROM_PATH = 277, CFGKEY_ULTRAMAN_ROM_PATH = 278,
	CFGKEY_SYSTEM_CONTENT_ROTATION = 279, CFGKEY_INPUT_PORT_CONFIG = 280,
	CFGKEY_CART_TYPE = 281, CFGKEY_REGION = 282,
	CFGKEY_BIOS_LANG = 283, CFGKEY_AUTO_RTC_TIME = 284,
	CFGKEY_VIDEO_LINES = 285, CFGKEY_CORRECT_LINE_ASPECT = 286,
	CFGKEY_DEFAULT_NTSC_VIDEO_LINES = 287, CFGKEY_DEFAULT_PAL_VIDEO_LINES = 288,
	CFGKEY_DEFAULT_SHOW_H_OVERSCAN = 289, CFGKEY_SHOW_H_OVERSCAN = 290,
	CFGKEY_DEINTERLACE_MODE = 291, CFGKEY_WIDESCREEN_MODE = 292,
	CFGKEY_NO_MD5_FILENAMES = 293
};

struct VideoLineRange
{
	int16_t first, last;

	constexpr bool operator ==(VideoLineRange const&) const = default;
	constexpr operator bool() const { return first || last; }
	constexpr auto size() const { return (last - first) + 1; }
};

class SaturnSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedSS};
	FrameTime frameTime_{};
	double audioRate{44100};
	size_t currStateSize{};
	EmulateSpecStruct espec{};
	FileIO backupRamFileIO;
	FileIO cartRamFileIO;
	FileIO rtcFileIO;
	FileIO stvEepromFileIO;
	std::array<uint64_t, 13> inputBuff; // 12 gamepad buffers + 1 for misc keys
	static constexpr int maxFrameBuffWidth = 704, maxFrameBuffHeight = 576;
	alignas(8) uint32_t pixBuff[maxFrameBuffWidth * maxFrameBuffHeight];
	MutablePixmapView mSurfacePix;
	std::vector<CDInterface *> CDInterfaces;
	std::string naBiosPath;
	std::string jpBiosPath;
	std::string kof95ROMPath;
	std::string ultramanROMPath;
	VideoLineRange videoLines{};
	static constexpr VideoLineRange safeNtscLines{8, 231}, safePalLines{16, 271};
	VideoLineRange defaultNtscLines{safeNtscLines}, defaultPalLines{safePalLines};
	int16_t cartType{MDFN_IEN_SS::CART__RESERVED};
	uint8_t lastInterlaceMode{};
	int8_t region{};
	int8_t biosLanguage{MDFN_IEN_SS::SMPC_RTC_LANG_ENGLISH};
	InputConfig inputConfig{};
	DeinterlaceMode deinterlaceMode{DeinterlaceMode::Bob};
	bool defaultShowHOverscan{};
	bool showHOverscan{};
	bool correctLineAspect{};
	bool autoRTCTime{true};
	bool noMD5InFilenames{};
	Rotation sysContentRotation{Rotation::ANY};
	WidescreenMode widescreenMode{WidescreenMode::Auto};

	SaturnSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
		espec.sys = this;
	}
	void updatePixmap(PixelFormat);
	void loadCartNV(EmuApp &, FileIO &);
	void saveCartNV(FileIO &);
	void applyInputConfig(InputConfig, EmuApp &);
	void applyInputConfig(EmuApp &app) { applyInputConfig(inputConfig, app); }
	void setInputConfig(InputConfig, EmuApp &);
	int currentDiscId() const;
	void setDisc(int id);
	void updateVideoSettings();
	void setVideoLines(VideoLineRange lines)
	{
		videoLines = lines;
		updateVideoSettings();
	}
	void setShowHOverscan(bool on)
	{
		showHOverscan = on;
		updateVideoSettings();
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".mca"; }
	size_t stateSize();
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags);
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const;
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	void renderFramebuffer(EmuVideo &);
	bool onVideoRenderFormatChange(EmuVideo &, PixelFormat);
	WSize multiresVideoBaseSize() const;
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	VideoSystem videoSystem() const { return MDFN_IEN_SS::VDP2::PAL ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC; }
	double videoAspectRatioScale() const;
	bool onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, WRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, WRect);
	Rotation contentRotation() const;
	void addThreadGroupIds(std::vector<ThreadId> &ids) const { ids.emplace_back(MDFN_IEN_SS::RThreadId); }
};

using MainSystem = SaturnSystem;

}
