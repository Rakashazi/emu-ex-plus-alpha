#pragma once

/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <fceu/driver.h>
#include <fceu/palette.h>
#include <fceu/state.h>

namespace EmuEx::Controls
{
extern const int gamepadKeys;
}

namespace EmuEx
{

class EmuAudio;

enum
{
	CFGKEY_FDS_BIOS_PATH = 270, CFGKEY_FOUR_SCORE = 271,
	CFGKEY_VIDEO_SYSTEM = 272, CFGKEY_SPRITE_LIMIT = 273,
	CFGKEY_SOUND_QUALITY = 274, CFGKEY_INPUT_PORT_1 = 275,
	CFGKEY_INPUT_PORT_2 = 276, CFGKEY_DEFAULT_PALETTE_PATH = 277,
	CFGKEY_DEFAULT_VIDEO_SYSTEM = 278, CFGKEY_COMPATIBLE_FRAMESKIP = 279,
	CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER = 280, CFGKEY_SWAP_DUTY_CYCLES = 281,
	CFGKEY_START_VIDEO_LINE = 282, CFGKEY_VISIBLE_VIDEO_LINES = 283,
	CFGKEY_HORIZONTAL_VIDEO_CROP = 284, CFGKEY_CORRECT_LINE_ASPECT = 285,
	CFGKEY_FF_DURING_FDS_ACCESS = 286, CFGKEY_CHEATS_PATH = 287,
	CFGKEY_PATCHES_PATH = 288, CFGKEY_PALETTE_PATH = 289,
	CFGKEY_OVERCLOCKING = 290, CFGKEY_OVERCLOCK_EXTRA_LINES = 291,
	CFGKEY_OVERCLOCK_VBLANK_MULTIPLIER = 292,
};

constexpr int maxExtraLinesPerFrame = 30000;
constexpr int maxVBlankMultiplier  = 16;

constexpr bool isSupportedStartingLine(uint8_t line)
{
	switch(line)
	{
		case 0:
		case 8:
			return true;
	}
	return false;
}

constexpr bool isSupportedLineCount(uint8_t lines)
{
	switch(lines)
	{
		case 224:
		case 232:
		case 240:
			return true;
	}
	return false;
}

class NesSystem final: public EmuSystem
{
public:
	using PalArray = std::array<pal, 512>;

	ESI nesInputPortDev[2]{SI_UNSET, SI_UNSET};
	uint32 padData{};
	uint32 zapperData[3]{};
	uint8_t fcExtData{};
	bool usingZapper{};
	uint8_t autoDetectedRegion{};
	PixelFormat pixFmt{};
	PalArray defaultPal{};
	union
	{
		uint16_t col16[256];
		uint32_t col32[256];
	} nativeCol;
	alignas(16) uint8 XBufData[256 * 256 + 16]{};
	std::string cheatsDir;
	std::string patchesDir;
	std::string palettesDir;
	std::string defaultPalettePath;
	std::string fdsBiosPath;
	bool fastForwardDuringFdsAccess = true;
	bool fdsIsAccessing{};
	Byte1Option optionFourScore{CFGKEY_FOUR_SCORE, 0};
	SByte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, -1, false, optionIsValidWithMinMax<-1, 2>};
	SByte1Option optionInputPort2{CFGKEY_INPUT_PORT_2, -1, false, optionIsValidWithMinMax<-1, 2>};
	Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
	Byte1Option optionDefaultVideoSystem{CFGKEY_DEFAULT_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
	Byte1Option optionSpriteLimit{CFGKEY_SPRITE_LIMIT, 1};
	Byte1Option optionSoundQuality{CFGKEY_SOUND_QUALITY, 0, false, optionIsValidWithMax<2>};
	Byte1Option optionCompatibleFrameskip{CFGKEY_COMPATIBLE_FRAMESKIP, 0};
	Byte1Option optionDefaultStartVideoLine{CFGKEY_START_VIDEO_LINE, 8, false, isSupportedStartingLine};
	Byte1Option optionDefaultVisibleVideoLines{CFGKEY_VISIBLE_VIDEO_LINES, 224, false, isSupportedLineCount};
	Byte1Option optionStartVideoLine{CFGKEY_START_VIDEO_LINE, 8, false, isSupportedStartingLine};
	Byte1Option optionVisibleVideoLines{CFGKEY_VISIBLE_VIDEO_LINES, 224, false, isSupportedLineCount};
	Byte1Option optionHorizontalVideoCrop{CFGKEY_HORIZONTAL_VIDEO_CROP, 0};
	Byte1Option optionCorrectLineAspect{CFGKEY_CORRECT_LINE_ASPECT, 0};
	static constexpr auto ntscFrameTime{fromSeconds<FrameTime>(16777215./ 1008307711.)}; // ~60.099Hz
	static constexpr auto palFrameTime{fromSeconds<FrameTime>(16777215. / 838977920.)}; // ~50.00Hz

	NesSystem(ApplicationContext);
	void connectNESInput(int port, ESI type);
	void setupNESInputPorts();
	void setupNESFourScore();
	void updateVideoPixmap(EmuVideo &, bool horizontalCrop, int lines);
	void setDefaultPalette(IG::ApplicationContext, IG::CStringView palPath);
	void renderVideo(EmuSystemTaskContext, EmuVideo &, uint8 *buf);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".fcs"; }
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
	void closeSystem();
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	VideoSystem videoSystem() const;
	double videoAspectRatioScale() const;
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	bool shouldFastForward() const;

private:
	void cacheUsingZapper();
	void setDefaultPalette(IO &io);
};

using MainSystem = NesSystem;

bool hasFDSBIOSExtension(std::string_view name);
const char *regionToStr(int region);
void emulateSound(EmuAudio *audio);
void setRegion(int region, int defaultRegion, int detectedRegion);

}

struct FCEUGI;
struct FCEUFILE;

FCEUGI *FCEUI_LoadGameWithFileVirtual(FCEUFILE *fp, const char *name, int OverwriteVidMode, bool silent);
