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

#include <imagine/fs/FSUtils.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/audio/SampleFormat.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/memory/DynArray.hh>
#include <imagine/util/enum.hh>
#include <emuframework/EmuTiming.hh>
#include <emuframework/VController.hh>
#include <emuframework/EmuInput.hh>
#include <string>
#include <string_view>

namespace IG
{
class ApplicationContext;
class PixelFormat;
class IO;
class FileIO;
class MapIO;
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
class Cheat;
class CheatCode;

struct CheatCodeDesc
{
	const char* str{};
	unsigned flags{};
};

struct AspectRatioInfo
{
	std::string_view name{};
	IG::Point2D<int8_t> aspect{};

	constexpr float asFloat() const { return aspect.ratio<float>(); }
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", {1, 1}}

struct BundledGameInfo
{
	const char *displayName;
	const char *assetName;
};

struct EmuSystemCreateParams
{
	uint8_t systemFlags;
};

enum class ConfigType : uint8_t
{
	MAIN, SESSION, CORE
};

struct InputAction
{
	KeyCode code{};
	KeyFlags flags{};
	Input::Action state{};
	uint32_t metaState{};

	constexpr bool isPushed() const { return state == Input::Action::PUSHED; }
	constexpr operator KeyInfo() const { return {code, flags}; }
};

enum class InputComponent : uint8_t
{
	ui, dPad, button, trigger
};

struct InputComponentFlags
{
	uint8_t
	altConfig:1{},
	rowSize:2{},
	staggeredLayout:1{};
};

struct InputComponentDesc
{
	const char *name{};
	std::span<const KeyInfo> keyCodes{};
	InputComponent type{};
	_2DOrigin layoutOrigin{};
	InputComponentFlags flags{};
};

struct SystemInputDeviceDesc
{
	const char *name;
	std::span<const InputComponentDesc> components;
};

enum class VideoSystem: uint8_t
{
	NATIVE_NTSC, PAL
};

WISE_ENUM_CLASS((DeinterlaceMode, uint8_t),
	Bob,
	Weave
);

using FrameTime = Nanoseconds;

constexpr const char *optionUserPathContentToken = ":CONTENT:";

struct SaveStateFlags
{
	uint8_t uncompressed:1{};
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
		int intArg{};
		int intArg2{};
		int intArg3{};
		LoadProgress progress{LoadProgress::UNSET};

		constexpr LoadProgressMessage() = default;
		constexpr LoadProgressMessage(LoadProgress progress, int intArg, int intArg2, int intArg3):
			intArg{intArg}, intArg2{intArg2}, intArg3{intArg3}, progress{progress} {}
	};

	using OnLoadProgressDelegate = IG::DelegateFunc<bool(int pos, int max, const char *label)>;
	using NameFilterFunc = bool(*)(std::string_view name);
	using BackupMemoryDirtyFlags = uint8_t;
	enum class ResetMode: uint8_t { HARD, SOFT };

	// Static system configuration
	static const int maxPlayers;
	static const char *configFilename;
	static bool inputHasKeyboard;
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
	static NameFilterFunc defaultFsFilter;
	static const char *creditsViewStr;
	static F2Size validFrameRateRange;
	static bool hasRectangularPixels;
	static bool stateSizeChangesAtRuntime;

	EmuSystem(IG::ApplicationContext ctx): appCtx{ctx} {}

	// required sub-class API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const;
	size_t stateSize();
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &io, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	FrameTime frameTime() const;
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;

	// optional sub-class API functions
	void onStart();
	void onStop();
	void closeSystem();
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState current, Input::DragTrackerState previous, WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, WindowRect gameRect);
	void onVKeyboardShown(VControllerKeyboard &, bool shown);
	VController::KbMap vControllerKeyboardMap(VControllerKbMode mode);
	VideoSystem videoSystem() const;
	void renderFramebuffer(EmuVideo &);
	WSize multiresVideoBaseSize() const;
	double videoAspectRatioScale() const;
	bool onVideoRenderFormatChange(EmuVideo &, PixelFormat);
	static bool canRenderMultipleFormats() {return canRenderRGBA8888 && canRenderRGB565;}
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	FS::FileString configName() const;
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	void savePathChanged();
	bool shouldFastForward() const;
	FS::FileString contentDisplayNameForPath(CStringView path) const;
	IG::Rotation contentRotation() const;
	void addThreadGroupIds(std::vector<ThreadId> &) const;
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

	ApplicationContext appContext() const { return appCtx; }
	bool isActive() const { return state == State::ACTIVE; }
	bool isStarted() const { return state == State::ACTIVE || state == State::PAUSED; }
	bool isPaused() const { return state == State::PAUSED; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView uri);
	DynArray<uint8_t> saveState();
	DynArray<uint8_t> uncompressGzipState(std::span<uint8_t> buff, size_t expectedSize = 0);
	bool stateExists(int slot) const;
	static std::string_view stateSlotName(int slot);
	std::string_view stateSlotName() { return stateSlotName(stateSlot()); }
	int stateSlot() const { return saveStateSlot; }
	void setStateSlot(int slot) { saveStateSlot = slot; }
	void decStateSlot() { if(--saveStateSlot < 0) saveStateSlot = 9; }
	void incStateSlot() { if(++saveStateSlot > 9) saveStateSlot = 0; }
	const char *systemName() const;
	const char *shortSystemName() const;
	const BundledGameInfo &bundledGameInfo(int idx) const;
	const auto &contentDirectory() const { return contentDirectory_; }
	FS::PathString contentDirectory(std::string_view name) const;
	FS::PathString contentFilePath(std::string_view ext) const;
	const auto &contentLocation() const { return contentLocation_; }
	FS::FileString contentNameExt(std::string_view ext) const
	{
		FS::FileString name{contentName_};
		name += ext;
		return name;
	}
	const auto &contentName() const { return contentName_; }
	FS::FileString contentFileName() const;
	std::string contentDisplayName() const;
	void setContentDisplayName(std::string_view name);
	FS::FileString contentDisplayNameForPathDefaultImpl(CStringView path) const;
	void setInitialLoadPath(CStringView path);
	FS::PathString fallbackSaveDirectory(bool create = false);
	const auto &contentSaveDirectory() const { return contentSaveDirectory_; }

	FS::PathString contentLocalSaveDirectory(auto &&...components) const
	{
		return contentLocalDirectory(contentSaveDirectory_, "saves", IG_forward(components)...);
	}

	bool createContentLocalSaveDirectory(auto &&...components)
	{
		return createContentLocalDirectory(contentSaveDirectory_, "saves", IG_forward(components)...);
	}

	FS::PathString contentLocalDirectory(std::string_view basePath, std::string_view name, auto &&...components) const
	{
		assert(!contentName_.empty());
		return FS::uriString(basePath, contentName_, name, IG_forward(components)...);
	}

	bool createContentLocalDirectory(std::string_view basePath, std::string_view name, auto &&...components)
	{
		assert(!contentName_.empty());
		try
		{
			FS::createDirectoryUriSegments(appContext(), basePath, contentName_, name, IG_forward(components)...);
		}
		catch(...)
		{
			return false;
		}
		return true;
	}

	FS::PathString contentSavePath(std::string_view name) const;
	const char *contentSaveDirectoryPtr() { return contentSaveDirectory_.data(); }
	FS::PathString contentSaveFilePath(std::string_view ext) const;
	const auto &userSaveDirectory() const { return userSaveDirectory_; }
	void setUserSaveDirectory(CStringView path);
	FS::FileString stateFilename(int slot) const { return stateFilename(slot, contentName_); }
	FS::FileString stateFilename(std::string_view name) const;
	FS::PathString statePath(std::string_view filename, std::string_view basePath) const;
	FS::PathString statePath(std::string_view filename) const;
	FS::PathString statePath(int slot, std::string_view basePath) const;
	FS::PathString statePath(int slot) const;
	FS::PathString userPath(std::string_view userDir, std::string_view filename) const;
	FS::PathString userPath(std::string_view userDir) const;
	FS::PathString userFilePath(std::string_view userDir, std::string_view ext) const;
	void clearGamePaths();
	char saveSlotChar(int slot) const;
	char saveSlotCharUpper(int slot) const;
	void flushBackupMemory(EmuApp &, BackupMemoryDirtyFlags flags = 0xFF);
	void onBackupMemoryWritten(BackupMemoryDirtyFlags flags = 0xFF);
	bool updateBackupMemoryCounter();
	bool usesBackupMemory() const;
	FileIO openStaticBackupMemoryFile(CStringView uri, size_t staticSize, uint8_t initValue = 0) const;
	void sessionOptionSet();
	void resetSessionOptionsSet() { sessionOptionsSet = false; }
	bool sessionOptionsAreSet() const { return sessionOptionsSet; }
	void createWithMedia(IG::IO, CStringView path,
		std::string_view displayName, EmuSystemCreateParams, OnLoadProgressDelegate);
	FS::PathString willLoadContentFromPath(std::string_view path, std::string_view displayName);
	void loadContentFromPath(CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	void loadContentFromFile(IG::IO, CStringView path, std::string_view displayName,
		EmuSystemCreateParams, OnLoadProgressDelegate);
	int updateAudioFramesPerVideoFrame();
	double frameRate() const { return toHz(frameTime()); }
	FrameTime scaledFrameTime() const
	{
		auto t = std::chrono::duration_cast<FloatSeconds>(frameTime()) * frameTimeMultiplier;
		return std::chrono::duration_cast<FrameTime>(t);
	}
	double scaledFrameRate() const
	{
		return toHz(std::chrono::duration_cast<FloatSeconds>(frameTime()) * frameTimeMultiplier);
	}
	void onFrameTimeChanged();
	static double audioMixRate(int outputRate, double inputFrameRate, FrameTime outputFrameTime);
	double audioMixRate(int outputRate, FrameTime outputFrameTime) const { return audioMixRate(outputRate, frameRate(), outputFrameTime); }
	void configFrameTime(int outputRate, FrameTime outputFrameTime);
	SteadyClockTime benchmark(EmuVideo &video);
	bool hasContent() const;
	void resetFrameTime();
	void pause(EmuApp &);
	void start(EmuApp &);
	void closeRuntimeSystem(EmuApp &);
	static void throwFileReadError();
	static void throwFileWriteError();
	static void throwMissingContentDirError();

protected:
	IG::ApplicationContext appCtx{};
public:
	EmuTiming timing;
protected:
	double audioFramesPerVideoFrameFloat{};
	double currentAudioFramesPerVideoFrame{};
	int audioFramesPerVideoFrame{};
	int saveStateSlot{};
	State state{};
	bool sessionOptionsSet{};
	BackupMemoryDirtyFlags backupMemoryDirtyFlags{};
	int8_t backupMemoryCounter{};
	FS::PathString contentDirectory_; // full directory path of content on disk, if any
	FS::PathString contentLocation_; // full path or URI to content
	FS::FileString contentFileName_; // name + extension of content, inside archive if any
	FS::FileString contentName_; // name of content from the original location without extension
	std::string contentDisplayName_; // more descriptive content name set by system
	FS::PathString contentSaveDirectory_;
	FS::PathString userSaveDirectory_;

	void setupContentUriPaths(CStringView uri, std::string_view displayName);
	void setupContentFilePaths(CStringView filePath, std::string_view displayName);
	void updateContentSaveDirectory();
	void closeAndSetupNew(CStringView path, std::string_view displayName);

public:
	IG::OnFrameDelegate onFrameUpdate;
	double frameTimeMultiplier{1.};
	static constexpr double minFrameRate = 48.;
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuSystem &gSystem();

}
