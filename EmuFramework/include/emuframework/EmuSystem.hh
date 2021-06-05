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

#include <imagine/fs/FS.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/audio/SampleFormat.hh>
#include <imagine/util/rectangle2.h>
#include <emuframework/config.hh>
#include <optional>
#include <stdexcept>

namespace Base
{
class ApplicationContext;
}

namespace Input
{
class Event;
enum class Action : uint8_t;
}

namespace IG
{
class PixelFormat;
}

class IO;
class GenericIO;
class EmuInputView;
class EmuSystemTask;
class EmuAudio;
class EmuVideo;
class EmuApp;
struct EmuFrameTimeInfo;

struct AspectRatioInfo
{
	constexpr AspectRatioInfo(const char *name, unsigned n, unsigned d): name(name), aspect{n, d} {}
	constexpr explicit operator double() const { return aspect.ratio<double>(); }
	const char *name;
	IG::Point2D<unsigned> aspect;
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", 1, 1}, {"Full Screen", 0, 1}

struct BundledGameInfo
{
	const char *displayName;
	const char *assetName;
};

struct EmuSystemCreateParams
{
	uint8_t systemFlags;
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
	static int saveStateSlot;
	static unsigned aspectRatioX, aspectRatioY;
	static const unsigned maxPlayers;
	static const AspectRatioInfo aspectRatioInfo[];
	static const unsigned aspectRatioInfos;
	static const char *configFilename;
	static const char *inputFaceBtnName;
	static const char *inputCenterBtnName;
	static const unsigned inputCenterBtns;
	static const unsigned inputFaceBtns;
	static const bool inputHasTriggerBtns;
	static const bool inputHasRevBtnLayout;
	static bool inputHasKeyboard;
	static bool inputHasShortBtnTexture;
	static bool hasBundledGames;
	static bool hasPALVideoSystem;
	enum VideoSystem { VIDSYS_NATIVE_NTSC, VIDSYS_PAL };
	static IG::FloatSeconds frameTimeNative;
	static IG::FloatSeconds frameTimePAL;
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
	static IG::Audio::SampleFormat audioSampleFormat;
	static bool constFrameRate;
	static NameFilterFunc defaultFsFilter;
	static NameFilterFunc defaultBenchmarkFsFilter;
	static const char *creditsViewStr;
	static bool sessionOptionsSet;

	static Error onInit(Base::ApplicationContext);
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static Error loadState(EmuApp &, const char *path);
	static Error loadState(const char *path);
	static Error saveState(EmuApp &, const char *path);
	static Error saveState(const char *path);
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(unsigned idx);
	static const char *gamePath() { return gamePath_.data(); }
	static FS::PathString gamePathString() { return gamePath_; }
	static const char *fullGamePath() { return fullGamePath_.data(); }
	static FS::FileString gameName() { return gameName_; }
	static FS::FileString fullGameName();
	static FS::FileString gameFileName();
	static FS::FileString originalGameFileName();
	static void setFullGameName(const char *name);
	static FS::FileString fullGameNameForPathDefaultImpl(const char *path);
	static FS::FileString fullGameNameForPath(Base::ApplicationContext, const char *path);
	static void setInitialLoadPath(const char *path);
	static FS::PathString baseSavePath(Base::ApplicationContext);
	static FS::PathString makeDefaultBaseSavePath(Base::ApplicationContext);
	static void makeDefaultSavePath(Base::ApplicationContext);
	static const char *defaultSavePath(Base::ApplicationContext);
	static const char *savePath();
	static FS::PathString sprintStateFilename(int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName_.data());
	static char saveSlotChar(int slot);
	static char saveSlotCharUpper(int slot);
	static void saveBackupMem();
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void reset(EmuApp &, ResetMode mode);
	static void initOptions();
	static Error onOptionsLoaded(Base::ApplicationContext);
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, unsigned key, unsigned readSize);
	static bool resetSessionOptions(EmuApp &);
	static void sessionOptionSet();
	static void onSessionOptionsLoaded(EmuApp &);
	static void writeSessionConfig(IO &io);
	static bool readSessionConfig(IO &io, unsigned key, unsigned readSize);
	static void createWithMedia(Base::ApplicationContext, GenericIO, const char *path, const char *name,
		Error &errOut, EmuSystemCreateParams, OnLoadProgressDelegate);
	static Error loadGame(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static Error loadGame(Base::ApplicationContext, IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static FS::PathString willLoadGameFromPath(FS::PathString path);
	static Error loadGameFromPath(Base::ApplicationContext, const char *path, EmuSystemCreateParams, OnLoadProgressDelegate);
	static Error loadGameFromFile(Base::ApplicationContext, GenericIO, const char *name, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] static void runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio);
	static void renderFramebuffer(EmuVideo &);
	static bool shouldFastForward();
	static void onPrepareAudio(EmuAudio &);
	static void onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	static bool vidSysIsPAL();
	static uint32_t updateAudioFramesPerVideoFrame();
	static double frameRate();
	static double frameRate(VideoSystem system);
	static IG::FloatSeconds frameTime();
	static IG::FloatSeconds frameTime(VideoSystem system);
	static IG::FloatSeconds defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, IG::FloatSeconds time);
	static bool setFrameTime(VideoSystem system, IG::FloatSeconds time);
	static unsigned multiresVideoBaseX();
	static unsigned multiresVideoBaseY();
	static void configAudioRate(IG::FloatSeconds frameTime, uint32_t rate);
	static void configAudioPlayback(EmuAudio &, uint32_t rate);
	static void configFrameTime(uint32_t rate);
	static void clearInputBuffers(EmuInputView &view);
	static void handleInputAction(EmuApp *, Input::Action state, unsigned emuKey);
	static unsigned translateInputAction(unsigned input, bool &turbo);
	static unsigned translateInputAction(unsigned input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool touchControlsApplicable();
	static bool handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect);
	static void setStartFrameTime(Base::FrameTime time);
	static EmuFrameTimeInfo advanceFramesWithTime(IG::FrameTime time);
	static void setSpeedMultiplier(EmuAudio &, uint8_t speed);
	static void setupGamePaths(Base::ApplicationContext, const char *filePath);
	static void setGameSavePath(Base::ApplicationContext, const char *path);
	static void setupGameSavePath(Base::ApplicationContext);
	static void clearGamePaths();
	static FS::PathString baseDefaultGameSavePath(Base::ApplicationContext);
	static IG::Time benchmark(EmuVideo &video);
	static bool gameIsRunning();
	static void resetFrameTime();
	static void prepareAudio(EmuAudio &audio);
	static void pause(EmuApp &);
	static void start(EmuApp &);
	static void closeSystem();
	static void closeRuntimeSystem(EmuApp &, bool allowAutosaveState = 1);
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
