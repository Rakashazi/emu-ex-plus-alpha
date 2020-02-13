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
#include <imagine/base/baseDefs.hh>
#include <imagine/base/Timer.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/string.h>
#include <emuframework/config.hh>
#include <optional>
#include <stdexcept>

class EmuInputView;
class EmuSystemTask;
class EmuAudio;
class EmuVideo;

struct AspectRatioInfo
{
	constexpr AspectRatioInfo(const char *name, uint n, uint d): name(name), aspect{n, d} {}
	const char *name;
	IG::Point2D<uint> aspect;
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", 1, 1}, {"Full Screen", 0, 1}

struct BundledGameInfo
{
	const char *displayName;
	const char *assetName;
};

enum { STATE_RESULT_OK, STATE_RESULT_NO_FILE, STATE_RESULT_NO_FILE_ACCESS, STATE_RESULT_IO_ERROR,
	STATE_RESULT_INVALID_DATA, STATE_RESULT_OTHER_ERROR };

class EmuSystem
{
private:
	static FS::PathString gamePath_, fullGamePath_;
	static FS::FileString gameName_, fullGameName_, originalGameName_;
	static FS::PathString defaultSavePath_;
	static FS::PathString gameSavePath_;

public:
	enum class State
	{
		OFF,
		STARTING,
		PAUSED,
		ACTIVE
	};

	enum class LoadProgress : uint8_t
	{
		UNSET,
		FAILED,
		OK,
		SET,
		UPDATE
	};

	struct LoadProgressMessage
	{
		constexpr LoadProgressMessage() {}
		constexpr LoadProgressMessage(LoadProgress progress, int intArg, int intArg2, int intArg3):
			intArg{intArg}, intArg2{intArg2}, intArg3{intArg3}, progress{progress} {}
		explicit operator bool() const { return progress != LoadProgress::UNSET; }
		int intArg{};
		int intArg2{};
		int intArg3{};
		LoadProgress progress{LoadProgress::UNSET};
	};

	using OnLoadProgressDelegate = DelegateFunc<bool(int pos, int max, const char *label)>;

	using Error = std::optional<std::runtime_error>;
	using NameFilterFunc = bool(*)(const char *name);
	static State state;
	static FS::PathString savePath_;
	static Base::Timer autoSaveStateTimer;
	static int saveStateSlot;
	static Base::FrameTimeBase startFrameTime;
	static Base::FrameTimeBase timePerVideoFrame;
	static uint emuFrameNow;
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
	static bool inputHasKeyboard;
	static bool hasBundledGames;
	static bool hasPALVideoSystem;
	enum VideoSystem { VIDSYS_NATIVE_NTSC, VIDSYS_PAL };
	static double frameTimeNative;
	static double frameTimePAL;
	static double audioFramesPerVideoFrameFloat;
	static double currentAudioFramesPerVideoFrame;
	static uint32_t audioFramesPerVideoFrame;
	static bool hasResetModes;
	enum ResetMode { RESET_HARD, RESET_SOFT };
	static bool handlesArchiveFiles;
	static bool handlesGenericIO;
	static bool hasCheats;
	static bool hasSound;
	static int forcedSoundRate;
	static bool constFrameRate;
	static NameFilterFunc defaultFsFilter;
	static NameFilterFunc defaultBenchmarkFsFilter;
	static const char *creditsViewStr;
	static bool sessionOptionsSet;

	static Error onInit();
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static void cancelAutoSaveStateTimer();
	static void startAutoSaveStateTimer();
	static Error loadState(const char *path);
	static Error saveState(const char *path);
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(uint idx);
	static const char *gamePath() { return gamePath_.data(); }
	static const char *fullGamePath() { return fullGamePath_.data(); }
	static FS::FileString gameName() { return gameName_; }
	static FS::FileString fullGameName() { return strlen(fullGameName_.data()) ? fullGameName_ : gameName_; }
	static FS::FileString gameFileName() { return FS::basename(fullGamePath_); }
	static FS::FileString originalGameFileName() { return strlen(originalGameName_.data()) ? originalGameName_ : gameFileName(); }
	static void setFullGameName(const char *name) { string_copy(fullGameName_, name); }
	static FS::FileString fullGameNameForPathDefaultImpl(const char *path);
	static FS::FileString fullGameNameForPath(const char *path);
	static FS::PathString baseSavePath();
	static FS::PathString makeDefaultBaseSavePath();
	static void makeDefaultSavePath();
	static const char *defaultSavePath();
	static const char *savePath();
	static FS::PathString sprintStateFilename(int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName_.data());
	static char saveSlotChar(int slot);
	static char saveSlotCharUpper(int slot);
	static void saveBackupMem();
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void initOptions();
	static Error onOptionsLoaded();
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, uint key, uint readSize);
	static bool resetSessionOptions();
	static void sessionOptionSet();
	static void onSessionOptionsLoaded();
	static void writeSessionConfig(IO &io);
	static bool readSessionConfig(IO &io, uint key, uint readSize);
	static void createWithMedia(GenericIO io, const char *path, const char *name,
		Error &err, OnLoadProgressDelegate onLoadProgress);
	static Error loadGame(IO &io, OnLoadProgressDelegate onLoadProgress);
	static FS::PathString willLoadGameFromPath(FS::PathString path);
	static Error loadGameFromPath(const char *path, OnLoadProgressDelegate onLoadProgress);
	static Error loadGameFromFile(GenericIO io, const char *name, OnLoadProgressDelegate onLoadProgress);
	[[gnu::hot]] static void runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio);
	static void skipFrames(EmuSystemTask *task, uint frames, EmuAudio *audio);
	static bool shouldFastForward();
	static void onPrepareAudio(EmuAudio &audio);
	static void onPrepareVideo(EmuVideo &video);
	static bool vidSysIsPAL();
	static uint32_t updateAudioFramesPerVideoFrame();
	static double frameTime();
	static double frameTime(VideoSystem system);
	static double defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, double time);
	static bool setFrameTime(VideoSystem system, double time);
	static uint multiresVideoBaseX();
	static uint multiresVideoBaseY();
	static void configAudioRate(double frameTime, uint32_t rate);
	static void configAudioPlayback(uint32_t rate);
	static void configFrameTime(uint32_t rate);
	static void configFrameTime();
	static void clearInputBuffers(EmuInputView &view);
	static void handleInputAction(uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static uint translateInputAction(uint input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool touchControlsApplicable();
	static bool handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect);
	static uint advanceFramesWithTime(Base::FrameTimeBase time);
	static void setupGamePaths(const char *filePath);
	static void setGameSavePath(const char *path);
	static void setupGameSavePath();
	static void clearGamePaths();
	static FS::PathString baseDefaultGameSavePath();
	static IG::Time benchmark();
	static bool gameIsRunning()
	{
		return !string_equal(gameName_.data(), "");
	}
	static void resetFrameTime();
	static void prepareAudioVideo();
	static void pause();
	static void start();
	static void closeSystem();
	static void closeRuntimeSystem(bool allowAutosaveState = 1);
	[[gnu::format(printf, 1, 2)]]
	static Error makeError(const char *msg, ...);
	static Error makeError(std::error_code ec);
	static Error makeFileReadError();
	static Error makeFileWriteError();
	static Error makeBlankError();
};

static const char *stateNameStr(int slot)
{
	assert(slot >= -1 && slot < 10);
	static const char *str[] = { "Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	return str[slot+1];
}
