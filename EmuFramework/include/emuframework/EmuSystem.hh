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

namespace IG
{
class ApplicationContext;
class PixelFormat;
class IO;
class GenericIO;
}

namespace IG::Input
{
class Event;
class MotionEvent;
class DragTrackerState;
enum class Action : uint8_t;
}

namespace EmuEx
{

using namespace IG;
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

	using OnLoadProgressDelegate = IG::DelegateFunc<bool(int pos, int max, const char *label)>;

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
	static bool canRenderRGB565;
	static bool canRenderRGBA8888;
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

	static void onInit(IG::ApplicationContext);
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static bool isPaused() { return state == State::PAUSED; }
	static void loadState(EmuApp &, IG::CStringView uri);
	static void loadState(IG::CStringView path);
	static void saveState(IG::ApplicationContext, IG::CStringView uri);
	static void saveState(IG::CStringView path);
	static bool stateExists(IG::ApplicationContext, int slot);
	static const char *systemName();
	static const char *shortSystemName();
	static const BundledGameInfo &bundledGameInfo(unsigned idx);
	static auto contentDirectory() { return contentDirectory_; }
	static FS::PathString contentDirectory(IG::ApplicationContext, std::string_view name);
	static auto contentLocation() { return contentLocation_; }
	static const char *contentLocationPtr() { return contentLocation_.data(); }
	static FS::FileString contentName() { return contentName_; }
	static FS::FileString contentFileName();
	static std::string contentDisplayName();
	static void setContentDisplayName(std::string_view name);
	static FS::FileString contentDisplayNameForPathDefaultImpl(IG::ApplicationContext, IG::CStringView path);
	static FS::FileString contentDisplayNameForPath(IG::ApplicationContext, IG::CStringView path);
	static void setInitialLoadPath(IG::CStringView path);
	static FS::PathString fallbackSaveDirectory(IG::ApplicationContext, bool create = false);
	static FS::PathString contentSaveDirectory();
	static FS::PathString contentSavePath(IG::ApplicationContext, std::string_view name);
	static const char *contentSaveDirectoryPtr() { return contentSaveDirectory_.data(); }
	static FS::PathString contentSaveFilePath(IG::ApplicationContext, std::string_view ext);
	static FS::PathString userSaveDirectory();
	static void setUserSaveDirectory(IG::ApplicationContext, IG::CStringView path);
	static FS::PathString firmwarePath();
	static void setFirmwarePath(std::string_view path);
	static FS::FileString stateFilename(int slot, std::string_view name = EmuSystem::contentName_);
	static FS::PathString statePath(IG::ApplicationContext, std::string_view filename, std::string_view basePath = contentSaveDirectory());
	static FS::PathString statePath(IG::ApplicationContext, int slot, std::string_view basePath = contentSaveDirectory());
	static void clearGamePaths();
	static char saveSlotChar(int slot);
	static char saveSlotCharUpper(int slot);
	static void saveBackupMem(IG::ApplicationContext);
	static void savePathChanged();
	static void reset(ResetMode mode);
	static void reset(EmuApp &, ResetMode mode);
	static void initOptions(EmuApp &);
	static void onOptionsLoaded(IG::ApplicationContext);
	static void writeConfig(IO &io);
	static bool readConfig(IO &io, unsigned key, unsigned readSize);
	static bool resetSessionOptions(EmuApp &);
	static void sessionOptionSet();
	static void onSessionOptionsLoaded(EmuApp &);
	static void writeSessionConfig(IO &io);
	static bool readSessionConfig(IO &io, unsigned key, unsigned readSize);
	static void createWithMedia(IG::ApplicationContext, GenericIO, IG::CStringView path,
		std::string_view displayName, EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGame(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGame(IG::ApplicationContext, IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	static FS::PathString willLoadGameFromPath(IG::ApplicationContext, std::string_view path, std::string_view displayName);
	static void loadGameFromPath(IG::ApplicationContext, IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	static void loadGameFromFile(IG::ApplicationContext, GenericIO, IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] static void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	static void renderFramebuffer(EmuVideo &);
	static bool shouldFastForward();
	static void onPrepareAudio(EmuAudio &);
	static bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
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
	static double videoAspectRatioScale();
	static void configAudioRate(IG::FloatSeconds frameTime, uint32_t rate);
	static void configAudioPlayback(EmuAudio &, uint32_t rate);
	static void configFrameTime(uint32_t rate);
	static void clearInputBuffers(EmuInputView &view);
	static void handleInputAction(EmuApp *, IG::Input::Action state, unsigned emuKey);
	static void handleInputAction(EmuApp *, IG::Input::Action state, unsigned emuKey, uint32_t metaState);
	static unsigned translateInputAction(unsigned input, bool &turbo);
	static unsigned translateInputAction(unsigned input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static bool onPointerInputStart(const Input::MotionEvent &, IG::Input::DragTrackerState, IG::WindowRect gameRect);
	static bool onPointerInputUpdate(const Input::MotionEvent &, IG::Input::DragTrackerState current, IG::Input::DragTrackerState previous, IG::WindowRect gameRect);
	static bool onPointerInputEnd(const Input::MotionEvent &, IG::Input::DragTrackerState, IG::WindowRect gameRect);
	static void onVKeyboardShown(VControllerKeyboard &, bool shown);
	static bool inputHasTriggers();
	static void setStartFrameTime(IG::FrameTime time);
	static EmuFrameTimeInfo advanceFramesWithTime(IG::FrameTime time);
	static void setSpeedMultiplier(EmuAudio &, uint8_t speed);
	static IG::Time benchmark(EmuVideo &video);
	static bool gameIsRunning();
	static void resetFrameTime();
	static void prepareAudio(EmuAudio &audio);
	static void pause(EmuApp &);
	static void start(EmuApp &);
	static void closeSystem(IG::ApplicationContext ctx);
	static void closeRuntimeSystem(EmuApp &, bool allowAutosaveState = 1);
	static void throwFileReadError();
	static void throwFileWriteError();
	static void throwMissingContentDirError();

protected:
	static FS::PathString contentDirectory_; // full directory path of content on disk, if any
	static FS::PathString contentLocation_; // full path or URI to content
	static FS::FileString contentFileName_; // name + extension of content, inside archive if any
	static FS::FileString contentName_; // name of content from the original location without extension
	static std::string contentDisplayName_; // more descriptive content name set by system
	static FS::PathString contentSaveDirectory_;
	static FS::PathString userSaveDirectory_;
	static FS::PathString firmwarePath_;

	static void setupContentUriPaths(IG::ApplicationContext, IG::CStringView uri, std::string_view displayName);
	static void setupContentFilePaths(IG::ApplicationContext, IG::CStringView filePath, std::string_view displayName);
	static void updateContentSaveDirectory(IG::ApplicationContext);
	static void closeAndSetupNew(IG::ApplicationContext, IG::CStringView path, std::string_view displayName);
};

static const char *stateNameStr(int slot)
{
	assert(slot >= -1 && slot < 10);
	static const char *str[] = { "Auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	return str[slot+1];
}

}
