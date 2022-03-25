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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/audio/SampleFormat.hh>
#include <imagine/util/rectangle2.h>
#include <emuframework/config.hh>
#include <emuframework/EmuTiming.hh>
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
	enum VideoSystem { VIDSYS_NATIVE_NTSC, VIDSYS_PAL };
	enum ResetMode { RESET_HARD, RESET_SOFT };

	// Static system configuration
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
	static bool canRenderRGB565;
	static bool canRenderRGBA8888;
	static bool hasResetModes;
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
	static constexpr unsigned MAX_CENTER_BTNS = 2;
	static constexpr unsigned MAX_FACE_BTNS = 8;
	static std::array<int, MAX_FACE_BTNS> vControllerImageMap;

	EmuSystem(IG::ApplicationContext ctx): appCtx{ctx} {}
	IG::ApplicationContext appContext() const { return appCtx; }
	void onInit();
	bool isActive() const { return state == State::ACTIVE; }
	bool isStarted() const { return state == State::ACTIVE || state == State::PAUSED; }
	bool isPaused() const { return state == State::PAUSED; }
	void loadState(EmuApp &, IG::CStringView uri);
	void loadState(IG::CStringView path);
	void saveState(IG::CStringView path);
	bool stateExists(int slot) const;
	static std::string_view stateSlotName(int slot);
	std::string_view stateSlotName() { return stateSlotName(stateSlot()); }
	int stateSlot() const { return saveStateSlot; }
	void setStateSlot(int slot) { saveStateSlot = slot; }
	void decStateSlot() { if(--saveStateSlot < -1) saveStateSlot = 9; }
	void incStateSlot() { if(++saveStateSlot > 9) saveStateSlot = -1; }
	const char *systemName() const;
	const char *shortSystemName() const;
	const BundledGameInfo &bundledGameInfo(unsigned idx) const;
	auto contentDirectory() const { return contentDirectory_; }
	FS::PathString contentDirectory(std::string_view name) const;
	auto contentLocation() const { return contentLocation_; }
	const char *contentLocationPtr() { return contentLocation_.data(); }
	FS::FileString contentName() const { return contentName_; }
	FS::FileString contentFileName() const;
	std::string contentDisplayName() const;
	void setContentDisplayName(std::string_view name);
	FS::FileString contentDisplayNameForPathDefaultImpl(IG::CStringView path);
	FS::FileString contentDisplayNameForPath(IG::CStringView path);
	void setInitialLoadPath(IG::CStringView path);
	FS::PathString fallbackSaveDirectory(bool create = false);
	FS::PathString contentSaveDirectory() const;
	FS::PathString contentSavePath(std::string_view name);
	const char *contentSaveDirectoryPtr() { return contentSaveDirectory_.data(); }
	FS::PathString contentSaveFilePath(std::string_view ext);
	FS::PathString userSaveDirectory() const;
	void setUserSaveDirectory(IG::CStringView path);
	FS::PathString firmwarePath() const;
	void setFirmwarePath(std::string_view path);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	FS::FileString stateFilename(int slot) const { return stateFilename(slot, contentName_); }
	FS::PathString statePath(std::string_view filename, std::string_view basePath) const;
	FS::PathString statePath(std::string_view filename) const;
	FS::PathString statePath(int slot, std::string_view basePath) const;
	FS::PathString statePath(int slot) const;
	void clearGamePaths();
	char saveSlotChar(int slot) const;
	char saveSlotCharUpper(int slot) const;
	void saveBackupMem();
	void savePathChanged();
	void reset(ResetMode mode);
	void reset(EmuApp &, ResetMode mode);
	void initOptions(EmuApp &);
	void onOptionsLoaded();
	void writeConfig(IO &io);
	bool readConfig(IO &io, unsigned key, unsigned readSize);
	bool resetSessionOptions(EmuApp &);
	void sessionOptionSet();
	void resetSessionOptionsSet() { sessionOptionsSet = false; }
	bool sessionOptionsAreSet() const { return sessionOptionsSet; }
	void onSessionOptionsLoaded(EmuApp &);
	void writeSessionConfig(IO &io);
	bool readSessionConfig(IO &io, unsigned key, unsigned readSize);
	void createWithMedia(GenericIO, IG::CStringView path,
		std::string_view displayName, EmuSystemCreateParams, OnLoadProgressDelegate);
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	FS::PathString willLoadContentFromPath(std::string_view path, std::string_view displayName);
	void loadContentFromPath(IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	void loadContentFromFile(GenericIO, IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	void renderFramebuffer(EmuVideo &);
	bool shouldFastForward();
	void onPrepareAudio(EmuAudio &);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	bool vidSysIsPAL();
	uint32_t updateAudioFramesPerVideoFrame();
	double frameRate();
	double frameRate(VideoSystem system);
	IG::FloatSeconds frameTime();
	IG::FloatSeconds frameTime(VideoSystem system);
	static IG::FloatSeconds defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, IG::FloatSeconds time);
	bool setFrameTime(VideoSystem system, IG::FloatSeconds time);
	unsigned multiresVideoBaseX();
	unsigned multiresVideoBaseY();
	double videoAspectRatioScale();
	void configAudioRate(IG::FloatSeconds frameTime, uint32_t rate);
	void configAudioPlayback(EmuAudio &, uint32_t rate);
	void configFrameTime(uint32_t rate);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, IG::Input::Action state, unsigned emuKey);
	void handleInputAction(EmuApp *, IG::Input::Action state, unsigned emuKey, uint32_t metaState);
	unsigned translateInputAction(unsigned input, bool &turbo);
	unsigned translateInputAction(unsigned input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	bool onPointerInputStart(const Input::MotionEvent &, IG::Input::DragTrackerState, IG::WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, IG::Input::DragTrackerState current, IG::Input::DragTrackerState previous, IG::WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, IG::Input::DragTrackerState, IG::WindowRect gameRect);
	void onVKeyboardShown(VControllerKeyboard &, bool shown);
	static bool inputHasTriggers();
	void setStartFrameTime(IG::FrameTime time);
	EmuFrameTimeInfo advanceFramesWithTime(IG::FrameTime time);
	void setSpeedMultiplier(EmuAudio &, uint8_t speed);
	IG::Time benchmark(EmuVideo &video);
	bool hasContent() const;
	void resetFrameTime();
	void pause(EmuApp &);
	void start(EmuApp &);
	void closeSystem();
	void closeRuntimeSystem(EmuApp &, bool allowAutosaveState = 1);
	static void throwFileReadError();
	static void throwFileWriteError();
	static void throwMissingContentDirError();

protected:
	IG::ApplicationContext appCtx{};
	EmuTiming emuTiming{};
	IG::FloatSeconds frameTimeNative{1./60.};
	IG::FloatSeconds frameTimePAL{1./50.};
	double audioFramesPerVideoFrameFloat{};
	double currentAudioFramesPerVideoFrame{};
	uint32_t audioFramesPerVideoFrame{};
	int saveStateSlot{};
	State state{};
	bool sessionOptionsSet{};
	FS::PathString contentDirectory_{}; // full directory path of content on disk, if any
	FS::PathString contentLocation_{}; // full path or URI to content
	FS::FileString contentFileName_{}; // name + extension of content, inside archive if any
	FS::FileString contentName_{}; // name of content from the original location without extension
	std::string contentDisplayName_{}; // more descriptive content name set by system
	FS::PathString contentSaveDirectory_{};
	FS::PathString userSaveDirectory_{};
	FS::PathString firmwarePath_{};

	void setupContentUriPaths(IG::CStringView uri, std::string_view displayName);
	void setupContentFilePaths(IG::CStringView filePath, std::string_view displayName);
	void updateContentSaveDirectory();
	void closeAndSetupNew(IG::CStringView path, std::string_view displayName);
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuSystem &gSystem();

}
