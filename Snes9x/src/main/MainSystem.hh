#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <snes9x.h>
#include <port.h>
#include <memmap.h>
#include <cheats.h>
#ifndef SNES9X_VERSION_1_4
#include <controls.h>
#include <apu/apu.h>
#else
#include <apu.h>
#endif

namespace EmuEx::Controls
{
extern const int gamepadKeys;
}

namespace EmuEx
{

class VController;

enum
{
	CFGKEY_MULTITAP = 276, CFGKEY_BLOCK_INVALID_VRAM_ACCESS = 277,
	CFGKEY_VIDEO_SYSTEM = 278, CFGKEY_INPUT_PORT = 279,
	CFGKEY_AUDIO_DSP_INTERPOLATON = 280, CFGKEY_SEPARATE_ECHO_BUFFER = 281,
	CFGKEY_SUPERFX_CLOCK_MULTIPLIER = 282, CFGKEY_ALLOW_EXTENDED_VIDEO_LINES = 283,
	CFGKEY_CHEATS_PATH = 284, CFGKEY_PATCHES_PATH = 285,
	CFGKEY_SATELLAVIEW_PATH = 286, CFGKEY_SUFAMI_BIOS_PATH = 287,
	CFGKEY_BSX_BIOS_PATH = 288,
};

#ifdef SNES9X_VERSION_1_4
constexpr bool IS_SNES9X_VERSION_1_4 = true;
#else
constexpr bool IS_SNES9X_VERSION_1_4 = false;
#endif

constexpr int inputPortMinVal = IS_SNES9X_VERSION_1_4 ? 0 : -1;

#ifndef SNES9X_VERSION_1_4
constexpr int SNES_AUTO_INPUT = -1;
constexpr int SNES_JOYPAD = CTL_JOYPAD;
constexpr int SNES_MOUSE_SWAPPED = CTL_MOUSE;
constexpr int SNES_SUPERSCOPE = CTL_SUPERSCOPE;
constexpr int SNES_JUSTIFIER = CTL_JUSTIFIER;
#endif

class Snes9xSystem final: public EmuSystem
{
public:
	std::string cheatsDir;
	std::string patchesDir;
	std::string satDir{optionUserPathContentToken};
	std::string sufamiBiosPath;
	std::string bsxBiosPath;
	#ifndef SNES9X_VERSION_1_4
	int snesInputPort = SNES_AUTO_INPUT;
	int snesActiveInputPort = SNES_JOYPAD;
	#else
	union
	{
		int snesInputPort = SNES_JOYPAD;
		int snesActiveInputPort;
	};
	uint16 joypadData[5]{};
	#endif
	int snesPointerX{}, snesPointerY{}, snesPointerBtns{}, snesMouseClick{};
	int snesMouseX{}, snesMouseY{};
	int doubleClickFrames{}, rightClickFrames{};
	Input::PointerId mousePointerId{Input::NULL_POINTER_ID};
	bool dragWithButton{}; // true to start next mouse drag with a button held
	Byte1Option optionMultitap{CFGKEY_MULTITAP, 0};
	SByte1Option optionInputPort{CFGKEY_INPUT_PORT, inputPortMinVal, false, optionIsValidWithMinMax<inputPortMinVal, SNES_JUSTIFIER>};
	Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
	Byte1Option optionAllowExtendedVideoLines{CFGKEY_ALLOW_EXTENDED_VIDEO_LINES, 0};
	#ifndef SNES9X_VERSION_1_4
	Byte1Option optionBlockInvalidVRAMAccess{CFGKEY_BLOCK_INVALID_VRAM_ACCESS, 1};
	Byte1Option optionSeparateEchoBuffer{CFGKEY_SEPARATE_ECHO_BUFFER, 0};
	Byte1Option optionSuperFXClockMultiplier{CFGKEY_SUPERFX_CLOCK_MULTIPLIER, 100, false, optionIsValidWithMinMax<5, 250>};
	Byte1Option optionAudioDSPInterpolation{CFGKEY_AUDIO_DSP_INTERPOLATON, DSP_INTERPOLATION_GAUSSIAN, false, optionIsValidWithMax<4>};
	#endif
	static constexpr FloatSeconds ntscFrameTimeSecs{357366. / 21477272.}; // ~60.098Hz
	static constexpr FloatSeconds palFrameTimeSecs{425568. / 21281370.}; // ~50.00Hz
	static constexpr auto ntscFrameTime{round<FrameTime>(ntscFrameTimeSecs)};
	static constexpr auto palFrameTime{round<FrameTime>(palFrameTimeSecs)};

	Snes9xSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		#ifdef SNES9X_VERSION_1_4
		static uint16 screenBuff[512*478] __attribute__ ((aligned (8)));
		GFX.Screen = (uint8*)screenBuff;
		#endif
		Memory.Init();
		S9xGraphicsInit();
		S9xInitAPU();
		assert(Settings.Stereo == TRUE);
		#ifndef SNES9X_VERSION_1_4
		S9xInitSound(0);
		S9xUnmapAllControls();
		S9xCheatsEnable();
		#else
		S9xInitSound(Settings.SoundPlaybackRate, Settings.Stereo, 0);
		assert(Settings.H_Max == SNES_CYCLES_PER_SCANLINE);
		assert(Settings.HBlankStart == (256 * Settings.H_Max) / SNES_HCOUNTER_MAX);
		#endif
	}
	void setupSNESInput(VController &);
	static bool hasBiosExtension(std::string_view name);
	FloatSeconds frameTimeSecs() const { return videoSystem() == VideoSystem::PAL ? palFrameTimeSecs : ntscFrameTimeSecs; }

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const;
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
	void renderFramebuffer(EmuVideo &);
	WSize multiresVideoBaseSize() const;
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	VideoSystem videoSystem() const;
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState,
		Input::DragTrackerState prevDragState, IG::WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);

protected:
	void applyInputPortOption(int portVal, VController &vCtrl);
	WPt updateAbsolutePointerPosition(WRect gameRect, WPt pos);
	IOBuffer readSufamiTurboBios() const;
};

using MainSystem = Snes9xSystem;

inline Snes9xSystem &gSnes9xSystem() { return static_cast<Snes9xSystem&>(gSystem()); }

void setSuperFXSpeedMultiplier(unsigned val);

}

#ifndef SNES9X_VERSION_1_4
uint16 *S9xGetJoypadBits(unsigned idx);
uint8 *S9xGetMouseBits(unsigned idx);
uint8 *S9xGetMouseDeltaBits(unsigned idx);
int16 *S9xGetMousePosBits(unsigned idx);
uint8 *S9xGetSuperscopeBits();
uint8 *S9xGetJustifierBits();
CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons);
void DoGunLatch (int, int);
#endif
