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

#include <imagine/fs/FSDefs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/audio/SampleFormat.hh>
#include <imagine/util/rectangle2.h>
#include <emuframework/config.hh>
#include <optional>
#include <string>

namespace Base
{
class ApplicationContext;
}

namespace Input
{
class Event;
class DragTrackerState;
enum class Action : uint8_t;
}

namespace IG
{
class PixelFormat;
}

class IO;
class GenericIO;
class EmuInputView;
class EmuSystemTaskContext;
class EmuAudio;
class EmuVideo;
class EmuApp;
struct EmuFrameTimeInfo;
class VControllerKeyboard;

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

	using NameFilterFunc = bool(*)(std::string_view name);
	static State state;
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
	static int inputLTriggerIndex;
	static int inputRTriggerIndex;
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
	static constexpr unsigned MAX_CENTER_BTNS = 2;
	static constexpr unsigned MAX_FACE_BTNS = 8;
	static std::array<int, MAX_FACE_BTNS> vControllerImageMap;

	static void onInit(Base::ApplicationContext);
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static void loadState(EmuApp &, const char *path);
	static void loadState(const char *path);
	static void saveState(EmuApp &, const char *path);
	static void saveState(const char *path);
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(unsigned idx);
	static auto contentDirectory() { return contentDirectory_; }
	static auto contentLocation() { return contentLocation_; }
	static const char *contentLocationPtr() { return contentLocation_.data(); }
	static FS::FileString contentName() { return contentName_; }
	static FS::FileString contentFileName();
	static std::string contentDisplayName();
	static void setContentDisplayName(std::string_view name);
	static std::string contentDisplayNameForPathDefaultImpl(IG::CStringView path);
	static std::string contentDisplayNameForPath(Base::ApplicationContext, IG::CStringView path);
	static void setInitialLoadPath(IG::CStringView path);
	static FS::PathString makeDefaultBaseSavePath(Base::ApplicationContext);
	static void makeDefaultSavePath(Base::ApplicationContext);
	static FS::PathString defaultSavePath(Base::ApplicationContext);
	static FS::PathString contentSavePath();
	static const char *contentSavePathPtr() { return contentSavePath_.data(); }
	static FS::PathString contentSaveFilePath(std::string_view extension);
	static FS::PathString userSavePath();
	static void setUserSavePath(Base::ApplicationContext, IG::CStringView path);
	static FS::PathString firmwarePath();
	static void setFirmwarePath(IG::CStringView path);
	static FS::FileString stateFilename(int slot, std::string_view name = EmuSystem::contentName_.data());
	static FS::PathString statePath(std::string_view filename, std::string_view basePath = contentSavePath());
	static FS::PathString statePath(int slot, std::string_view basePath = contentSavePath());
	static void clearGamePaths();
	static FS::PathString baseDefaultGameSavePath(Base::ApplicationContext);
	static char saveSlotChar(int slot);
	static char saveSlotCharUpper(int slot);
	static void saveBackupMem();
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void reset(EmuApp &, ResetMode mode);
	static void initOptions(EmuApp &);
	static void onOptionsLoaded(Base::ApplicationContext);
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, unsigned key, unsigned readSize);
	static bool resetSessionOptions(EmuApp &);
	static void sessionOptionSet();
	static void onSessionOptionsLoaded(EmuApp &);
	static void writeSessionConfig(IO &io);
	static bool readSessionConfig(IO &io, unsigned key, unsigned readSize);
	static void createWithMedia(Base::ApplicationContext, GenericIO, IG::CStringView path,
		bool pathIsUri, EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGame(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGame(Base::ApplicationContext, IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static FS::PathString willLoadGameFromPath(std::string_view path);
	static void loadGameFromPath(Base::ApplicationContext, IG::CStringView path, bool pathIsUri,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGameFromFile(Base::ApplicationContext, GenericIO, IG::CStringView path, bool pathIsUri,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] static void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
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
	static void handleInputAction(EmuApp *, Input::Action state, unsigned emuKey, uint32_t metaState);
	static unsigned translateInputAction(unsigned input, bool &turbo);
	static unsigned translateInputAction(unsigned input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool onPointerInputStart(Input::Event, Input::DragTrackerState, IG::WindowRect gameRect);
	static bool onPointerInputUpdate(Input::Event, Input::DragTrackerState current, Input::DragTrackerState previous, IG::WindowRect gameRect);
	static bool onPointerInputEnd(Input::Event, Input::DragTrackerState, IG::WindowRect gameRect);
	static void onVKeyboardShown(VControllerKeyboard &, bool shown);
	static bool inputHasTriggers();
	static void setStartFrameTime(Base::FrameTime time);
	static EmuFrameTimeInfo advanceFramesWithTime(IG::FrameTime time);
	static void setSpeedMultiplier(EmuAudio &, uint8_t speed);
	static IG::Time benchmark(EmuVideo &video);
	static bool gameIsRunning();
	static void resetFrameTime();
	static void prepareAudio(EmuAudio &audio);
	static void pause(EmuApp &);
	static void start(EmuApp &);
	static void closeSystem();
	static void closeRuntimeSystem(EmuApp &, bool allowAutosaveState = 1);
	static void throwFileReadError();
	static void throwFileWriteError();
	static void throwBlankError();

protected:
	static FS::PathString contentDirectory_; // full directory path of content on disk, if any
	static FS::PathString contentLocation_; // full path or URI to content
	static FS::FileString contentFileName_; // name + extension of content, inside archive if any
	static FS::FileString contentName_; // name of content from the original location without extension
	static std::string contentDisplayName_; // more descriptive content name set by system
	static FS::PathString contentSavePath_;
	static FS::PathString userSavePath_;
	static FS::PathString firmwarePath_;

	static void setupContentUriPaths(Base::ApplicationContext, IG::CStringView uri);
	static void setupGamePaths(Base::ApplicationContext, IG::CStringView filePath);
	static void updateContentSavePath(Base::ApplicationContext);
	static void closeAndSetupNew(Base::ApplicationContext, IG::CStringView path, bool pathIsUri);
};

static const char *stateNameStr(int slot)
{
	assert(slot >= -1 && slot < 10);
	static const char *str[] = { "Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	return str[slot+1];
}
