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

#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuSystem.hh>
#include <fceu/driver.h>
#include <fceu/palette.h>
#include <fceu/state.h>
#include <fceu/cheat.h>

namespace EmuEx
{

class EmuAudio;

class Cheat: public CHEATF
{
public:
	Cheat(std::string_view name): CHEATF{.name = std::string{name}} {}
};

class CheatCode: public CHEATCODE
{
public:
	CheatCode(uint16 addr, uint8 val, int compare, int type):
		CHEATCODE{.addr = addr, .val = val, .compare = compare, .type = type} {}
};

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
	CFGKEY_OVERCLOCK_VBLANK_MULTIPLIER = 292, CFGKEY_P2_START_AS_FC_MIC = 293,
};

constexpr int maxExtraLinesPerFrame = 30000;
constexpr int maxVBlankMultiplier  = 16;

constexpr bool isSupportedStartingLine(const auto &line)
{
	switch(line)
	{
		case 0:
		case 8:
			return true;
	}
	return false;
}

constexpr bool isSupportedLineCount(const auto &lines)
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

	size_t saveStateSize{};
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
	std::string loaderErrorString;
	Property<bool, CFGKEY_FF_DURING_FDS_ACCESS, PropertyDesc<bool>{.defaultValue = true}> fastForwardDuringFdsAccess;
	bool fdsIsAccessing{};
	Property<bool, CFGKEY_FOUR_SCORE> optionFourScore;
	Property<ESI, CFGKEY_INPUT_PORT_1,
		PropertyDesc<ESI, int8_t>{.defaultValue = SI_UNSET, .isValid = isValidWithMinMax<SI_UNSET, SI_COUNT>}> inputPort1;
	Property<ESI, CFGKEY_INPUT_PORT_2,
		PropertyDesc<ESI, int8_t>{.defaultValue = SI_UNSET, .isValid = isValidWithMinMax<SI_UNSET, SI_COUNT>}> inputPort2;
	Property<uint8_t, CFGKEY_VIDEO_SYSTEM,
		PropertyDesc<uint8_t>{.defaultValue = 0, .isValid = isValidWithMax<3>}> optionVideoSystem;
	Property<uint8_t, CFGKEY_DEFAULT_VIDEO_SYSTEM,
		PropertyDesc<uint8_t>{.defaultValue = 0, .isValid = isValidWithMax<3>}> optionDefaultVideoSystem;
	Property<bool, CFGKEY_SPRITE_LIMIT, PropertyDesc<bool>{.defaultValue = true}> optionSpriteLimit;
	Property<uint8_t, CFGKEY_SOUND_QUALITY,
		PropertyDesc<uint8_t>{.defaultValue = 0, .isValid = isValidWithMax<2>}> optionSoundQuality;
	Property<bool, CFGKEY_COMPATIBLE_FRAMESKIP> optionCompatibleFrameskip;
	Property<uint8_t, CFGKEY_START_VIDEO_LINE,
		PropertyDesc<uint8_t>{.defaultValue = 8, .isValid = isSupportedStartingLine}> optionDefaultStartVideoLine;
	Property<uint8_t, CFGKEY_VISIBLE_VIDEO_LINES,
		PropertyDesc<uint8_t>{.defaultValue = 224, .isValid = isSupportedLineCount}> optionDefaultVisibleVideoLines;
	Property<uint8_t, CFGKEY_START_VIDEO_LINE,
		PropertyDesc<uint8_t>{.defaultValue = 8, .isValid = isSupportedStartingLine}> optionStartVideoLine;
	Property<uint8_t, CFGKEY_VISIBLE_VIDEO_LINES,
		PropertyDesc<uint8_t>{.defaultValue = 224, .isValid = isSupportedLineCount}> optionVisibleVideoLines;
	Property<bool, CFGKEY_HORIZONTAL_VIDEO_CROP> optionHorizontalVideoCrop;
	Property<bool, CFGKEY_CORRECT_LINE_ASPECT> optionCorrectLineAspect;
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
	size_t stateSize() { return saveStateSize; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags);
	bool readConfig(ConfigType, MapIO &, unsigned key);
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
	Cheat* newCheat(EmuApp&, const char* name, CheatCodeDesc);
	bool setCheatName(Cheat&, const char* name);
	std::string_view cheatName(const Cheat&) const;
	void setCheatEnabled(Cheat&, bool on);
	bool isCheatEnabled(const Cheat&) const;
	bool addCheatCode(EmuApp&, Cheat*&, CheatCodeDesc);
	bool modifyCheatCode(EmuApp&, Cheat&, CheatCode&, CheatCodeDesc);
	Cheat* removeCheatCode(Cheat&, CheatCode&);
	bool removeCheat(Cheat&);
	void forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)>);
	void forEachCheatCode(Cheat&, DelegateFunc<bool(CheatCode&, std::string_view)>);

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
extern bool replaceP2StartWithMicrophone;
