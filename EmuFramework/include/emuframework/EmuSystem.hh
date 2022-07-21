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
#include <emuframework/EmuTiming.hh>
#include <emuframework/VController.hh>
#include <optional>
#include <string>
#include <string_view>

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
	std::string_view name{};
	IG::Point2D<int8_t> aspect{};

	constexpr explicit operator double() const { return aspect.ratio<double>(); }
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", {1, 1}}, {"Full Screen", {0, 1}}

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

enum class ConfigType : uint8_t
{
	MAIN, SESSION, CORE
};

struct InputAction
{
	unsigned key{};
	Input::Action state{};
	uint32_t metaState{};
};

enum class VideoSystem: uint8_t
{
	NATIVE_NTSC, PAL
};

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
		constexpr LoadProgressMessage() = default;
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
	using BackupMemoryDirtyFlags = uint8_t;
	enum class ResetMode: uint8_t { HARD, SOFT };

	// Static system configuration
	static const int maxPlayers;
	static const char *configFilename;
	static const char *inputFaceBtnName;
	static const char *inputCenterBtnName;
	static const int inputCenterBtns;
	static const int inputFaceBtns;
	static int inputLTriggerIndex;
	static int inputRTriggerIndex;
	static bool inputHasKeyboard;
	static bool inputHasShortBtnTexture;
	static bool hasBundledGames;
	static bool hasPALVideoSystem;
	static double staticFrameTime;
	static double staticPalFrameTime;
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
	static constexpr int MAX_CENTER_BTNS = 2;
	static constexpr int MAX_FACE_BTNS = 8;
	static std::array<int, MAX_FACE_BTNS> vControllerImageMap;

	EmuSystem(IG::ApplicationContext ctx): appCtx{ctx} {}

	// required sub-class API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, IO &io, unsigned key, size_t readSize);
	void writeConfig(ConfigType, IO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	unsigned translateInputAction(unsigned input, bool &turbo);
	VController::Map vControllerMap(int player);
	void configAudioRate(FloatSeconds frameTime, int rate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional sub-class API functions
	void closeSystem();
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState current, Input::DragTrackerState previous, WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, WindowRect gameRect);
	void onVKeyboardShown(VControllerKeyboard &, bool shown);
	VController::KbMap vControllerKeyboardMap(unsigned mode);
	VideoSystem videoSystem() const;
	void renderFramebuffer(EmuVideo &);
	WP multiresVideoBaseSize() const;
	double videoAspectRatioScale() const;
	bool onVideoRenderFormatChange(EmuVideo &, PixelFormat);
	void onFlushBackupMemory(BackupMemoryDirtyFlags);
	FS::FileString configName() const;
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	void savePathChanged();
	bool shouldFastForward() const;
	FS::FileString contentDisplayNameForPath(CStringView path) const;

	ApplicationContext appContext() const { return appCtx; }
	bool isActive() const { return state == State::ACTIVE; }
	bool isStarted() const { return state == State::ACTIVE || state == State::PAUSED; }
	bool isPaused() const { return state == State::PAUSED; }
	bool stateExists(int slot) const;
	static std::string_view stateSlotName(int slot);
	std::string_view stateSlotName() { return stateSlotName(stateSlot()); }
	int stateSlot() const { return saveStateSlot; }
	void setStateSlot(int slot) { saveStateSlot = slot; }
	void decStateSlot() { if(--saveStateSlot < -1) saveStateSlot = 9; }
	void incStateSlot() { if(++saveStateSlot > 9) saveStateSlot = -1; }
	const char *systemName() const;
	const char *shortSystemName() const;
	const BundledGameInfo &bundledGameInfo(int idx) const;
	auto contentDirectory() const { return contentDirectory_; }
	FS::PathString contentDirectory(std::string_view name) const;
	auto contentLocation() const { return contentLocation_; }
	const char *contentLocationPtr() { return contentLocation_.data(); }
	FS::FileString contentName() const { return contentName_; }
	FS::FileString contentFileName() const;
	std::string contentDisplayName() const;
	void setContentDisplayName(std::string_view name);
	FS::FileString contentDisplayNameForPathDefaultImpl(IG::CStringView path) const;
	void setInitialLoadPath(IG::CStringView path);
	FS::PathString fallbackSaveDirectory(bool create = false);
	FS::PathString contentSaveDirectory() const;
	FS::PathString contentSavePath(std::string_view name) const;
	const char *contentSaveDirectoryPtr() { return contentSaveDirectory_.data(); }
	FS::PathString contentSaveFilePath(std::string_view ext) const;
	FS::PathString userSaveDirectory() const;
	void setUserSaveDirectory(IG::CStringView path);
	FS::PathString firmwarePath() const;
	void setFirmwarePath(std::string_view path);
	FS::FileString stateFilename(int slot) const { return stateFilename(slot, contentName_); }
	FS::PathString statePath(std::string_view filename, std::string_view basePath) const;
	FS::PathString statePath(std::string_view filename) const;
	FS::PathString statePath(int slot, std::string_view basePath) const;
	FS::PathString statePath(int slot) const;
	void clearGamePaths();
	char saveSlotChar(int slot) const;
	char saveSlotCharUpper(int slot) const;
	void flushBackupMemory(BackupMemoryDirtyFlags flags = 0xFF);
	void onBackupMemoryWritten(BackupMemoryDirtyFlags flags = 0xFF);
	bool updateBackupMemoryCounter();
	void sessionOptionSet();
	void resetSessionOptionsSet() { sessionOptionsSet = false; }
	bool sessionOptionsAreSet() const { return sessionOptionsSet; }
	void createWithMedia(GenericIO, IG::CStringView path,
		std::string_view displayName, EmuSystemCreateParams, OnLoadProgressDelegate);
	FS::PathString willLoadContentFromPath(std::string_view path, std::string_view displayName);
	void loadContentFromPath(IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	void loadContentFromFile(GenericIO, IG::CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	int updateAudioFramesPerVideoFrame();
	double frameRate() const;
	double frameRate(VideoSystem) const;
	FloatSeconds frameTime() const;
	FloatSeconds frameTime(VideoSystem) const;
	static FloatSeconds defaultFrameTime(VideoSystem system);
	static bool frameTimeIsValid(VideoSystem system, IG::FloatSeconds time);
	bool setFrameTime(VideoSystem system, IG::FloatSeconds time);
	void configAudioPlayback(EmuAudio &, int rate);
	void configFrameTime(int rate);
	static bool inputHasTriggers();
	void setStartFrameTime(IG::FrameTime time);
	EmuFrameTimeInfo advanceFramesWithTime(IG::FrameTime time);
	void setSpeedMultiplier(EmuAudio &, int8_t speed);
	IG::Time benchmark(EmuVideo &video);
	bool hasContent() const;
	void resetFrameTime();
	void pause(EmuApp &);
	void start(EmuApp &);
	void closeRuntimeSystem(EmuApp &, bool allowAutosaveState = 1);
	static void throwFileReadError();
	static void throwFileWriteError();
	static void throwMissingContentDirError();

	unsigned translateInputAction(unsigned input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}

protected:
	IG::ApplicationContext appCtx{};
	EmuTiming emuTiming{};
	IG::FloatSeconds frameTimeNative{1./60.};
	IG::FloatSeconds frameTimePAL{1./50.};
	double audioFramesPerVideoFrameFloat{};
	double currentAudioFramesPerVideoFrame{};
	int audioFramesPerVideoFrame{};
	int saveStateSlot{};
	State state{};
	bool sessionOptionsSet{};
	BackupMemoryDirtyFlags backupMemoryDirtyFlags{};
	int8_t backupMemoryCounter{};
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

	static auto &frameTimeVar(auto &self, VideoSystem system)
	{
		switch(system)
		{
			case VideoSystem::NATIVE_NTSC: return self.frameTimeNative;
			case VideoSystem::PAL: return self.frameTimePAL;
		}
		__builtin_unreachable();
	}
	auto &frameTimeVar(VideoSystem system) { return frameTimeVar(*this, system); }
	auto &frameTimeVar(VideoSystem system) const { return frameTimeVar(*this, system); }

public:
	IG::OnFrameDelegate onFrameUpdate{};
	int8_t targetFastForwardSpeed{};
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuSystem &gSystem();

}
