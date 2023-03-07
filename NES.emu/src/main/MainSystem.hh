#pragma once

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
};

extern FS::PathString fdsBiosPath;

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
	IG::PixelFormat pixFmt{};
	PalArray defaultPal{};
	union
	{
		uint16_t col16[256];
		uint32_t col32[256];
	} nativeCol;
	std::string cheatsDir;
	std::string patchesDir;
	std::string palettesDir;
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
	FS::PathString defaultPalettePath{};

	NesSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		backupSavestates = 0;
		if(!FCEUI_Initialize())
		{
			throw std::runtime_error{"Error in FCEUI_Initialize"};
		}
	}
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
	InputAction translateInputAction(InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	void configAudioRate(FloatSeconds frameTime, int rate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	IG::Time backupMemoryLastWriteTime(const EmuApp &) const;
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
