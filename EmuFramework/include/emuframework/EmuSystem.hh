#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/IO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/audio/Audio.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/audio/PcmFormat.hh>
#include <imagine/util/Rational.hh>

struct AspectRatioInfo
{
	constexpr AspectRatioInfo(const char *name, int n, int d): name(name), aspect{Rational::make<uint>(n, d)} {}
	const char *name;
	IG::Point2D<uint> aspect;
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", 1, 1}, {"Full Screen", 0, 1}

struct BundledGameInfo
{
	const char *displayName;
	const char *assetName;
};

class EmuNavView;

enum { STATE_RESULT_OK, STATE_RESULT_NO_FILE, STATE_RESULT_NO_FILE_ACCESS, STATE_RESULT_IO_ERROR,
	STATE_RESULT_INVALID_DATA, STATE_RESULT_OTHER_ERROR };

class EmuSystem
{
private:
	using GameNameArr = char[256];
	static FS::PathString gamePath_, fullGamePath_;
	static GameNameArr gameName_, fullGameName_;
	static FS::PathString defaultSavePath_;
	static FS::PathString gameSavePath_;

public:
	enum class State { OFF, STARTING, PAUSED, ACTIVE };
	static State state;
	static FS::PathString savePath_;
	static Base::Timer autoSaveStateTimer;
	static int saveStateSlot;
	static Base::FrameTimeBase startFrameTime;
	static Base::FrameTimeBase timePerVideoFrame;
	static uint emuFrameNow;
	static bool runFrameOnDraw;
	static Audio::PcmFormat pcmFormat;
	static uint audioFramesPerVideoFrame;
	static uint aspectRatioX, aspectRatioY;
	static const uint maxPlayers;
	static const AspectRatioInfo aspectRatioInfo[];
	static const uint aspectRatioInfos;
	static const char *configFilename;
	static const char *inputFaceBtnName;
	static const char *inputCenterBtnName;
	static const uint inputCenterBtns;
	static const uint inputFaceBtns;
	static const bool inputHasTriggerBtns;
	static const bool inputHasRevBtnLayout;
	static const bool inputHasKeyboard;
	static const bool hasBundledGames;
	static const bool hasPALVideoSystem;
	enum VideoSystem { VIDSYS_NATIVE_NTSC, VIDSYS_PAL };
	static double frameTimeNative;
	static double frameTimePAL;
	static const bool hasResetModes;
	enum ResetMode { RESET_HARD, RESET_SOFT };

	static CallResult onInit();
	static void onMainWindowCreated(Base::Window &win);
	static void onCustomizeNavView(EmuNavView &view);
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static void cancelAutoSaveStateTimer();
	static void startAutoSaveStateTimer();
	static int loadState(int slot = saveStateSlot);
	static int saveState();
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(uint idx);
	static const char *gamePath() { return gamePath_.data(); }
	static const char *fullGamePath() { return fullGamePath_.data(); }
	static const GameNameArr &gameName() { return gameName_; }
	static const GameNameArr &fullGameName() { return strlen(fullGameName_) ? fullGameName_ : gameName_; }
	static void setFullGameName(const char *name) { string_copy(fullGameName_, name); }
	static void makeDefaultSavePath();
	static const char *defaultSavePath();
	static const char *savePath();
	static FS::PathString sprintStateFilename(int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName_);
	static bool loadAutoState();
	static void saveAutoState();
	static void saveBackupMem();
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void initOptions();
	static void onOptionsLoaded();
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, uint key, uint readSize);
	static int loadGame(const char *path);
	static int loadGameFromIO(IO &io, const char *origFilename);
	typedef DelegateFunc<void (uint result, Input::Event e)> LoadGameCompleteDelegate;
	static LoadGameCompleteDelegate loadGameCompleteDel;
	static LoadGameCompleteDelegate &onLoadGameComplete() { return loadGameCompleteDel; }
	[[gnu::hot]] static void runFrame(bool renderGfx, bool processGfx, bool renderAudio);
	static bool vidSysIsPAL();
	static double frameTime();
	static double frameTime(VideoSystem system);
	static double defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, double time);
	static bool setFrameTime(VideoSystem system, double time);
	static uint multiresVideoBaseX();
	static uint multiresVideoBaseY();
	static void configAudioRate(double frameTime);
	static void configAudioPlayback();
	static void configFrameTime();
	static void clearInputBuffers();
	static void handleInputAction(uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static uint translateInputAction(uint input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool hasInputOptions();
	static void stopSound();
	static void startSound();
	static void writeSound(const void *samples, uint framesToWrite);
	static void commitSound(Audio::BufferContext buffer, uint frames);
	static uint advanceFramesWithTime(Base::FrameTimeBase time);
	static void setupGamePaths(const char *filePath);
	static void setGameSavePath(const char *path);
	static void setupGameSavePath();
	static void setupGameName(const char *name);
	static void clearGamePaths();
	static FS::PathString baseDefaultGameSavePath();
	static IG::Time benchmark();
	static bool gameIsRunning()
	{
		return !string_equal(gameName_, "");
	}
	static void resetFrameTime();
	static void pause();
	static void start();
	static void closeSystem();
	static void closeGame(bool allowAutosaveState = 1);
};

static const char *stateResultToStr(int res)
{
	switch(res)
	{
		case STATE_RESULT_NO_FILE: return "No State Exists";
		case STATE_RESULT_NO_FILE_ACCESS: return "File Permission Denied";
		case STATE_RESULT_IO_ERROR: return "File I/O Error";
		case STATE_RESULT_INVALID_DATA: return "Invalid State Data";
		default: bug_branch("%d", res); return 0;
	}
}

static const char *stateNameStr(int slot)
{
	assert(slot >= -1 && slot < 10);
	static const char *str[] = { "Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	return str[slot+1];
}

#if defined CONFIG_INPUT_POINTING_DEVICES
#define CONFIG_EMUFRAMEWORK_VCONTROLS
#endif

#if defined CONFIG_INPUT_KEYBOARD_DEVICES
#define CONFIG_INPUT_ICADE
#endif
