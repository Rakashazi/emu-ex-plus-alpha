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

#include <io/Io.hh>
#include <fs/sys.hh>
#include <audio/Audio.hh>
#include <util/time/sys.hh>
#include <util/audio/PcmFormat.hh>
#include <config/env.hh>
#include <gui/FSPicker/FSPicker.hh>
#include <util/gui/ViewStack.hh>

struct AspectRatioInfo
{
	constexpr AspectRatioInfo(const char *name, int n, int d): name(name), aspect{Rational::make<uint>(n, d)} {}
	const char *name;
	IG::Point2D<uint> aspect;
};

#define EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT {"1:1", 1, 1}, {"Full Screen", 0, 1}

class EmuSystem
{
	public:
	enum class State { OFF, STARTING, PAUSED, ACTIVE };
	static State state;
	static bool isActive() { return state == State::ACTIVE; }
	static bool isStarted() { return state == State::ACTIVE || state == State::PAUSED; }
	static FsSys::cPath gamePath, fullGamePath;
	static char gameName[256], fullGameName[256];
	static FsSys::cPath savePath_;
	static Base::CallbackRef *autoSaveStateCallbackRef;
	static int saveStateSlot;
	static TimeSys startTime;
	static Gfx::FrameTimeBase startFrameTime;
	static int emuFrameNow;
	static Audio::PcmFormat pcmFormat;
	static uint audioFramesPerVideoFrame;
	static const uint optionFrameSkipAuto;
	static uint aspectRatioX, aspectRatioY;
	static const uint maxPlayers;
	static const AspectRatioInfo aspectRatioInfo[];
	static const uint aspectRatioInfos;

	static void cancelAutoSaveStateTimer();
	static void startAutoSaveStateTimer();
	static int loadState(int slot = saveStateSlot);
	static int saveState();
	static bool stateExists(int slot);
	static bool shouldOverwriteExistingState();
	static const char *savePath() { return strlen(savePath_) ? savePath_ : gamePath; }
	static void sprintStateFilename(char *str, size_t size, int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName);
	template <size_t S>
	static void sprintStateFilename(char (&str)[S], int slot,
		const char *statePath = savePath(), const char *gameName = EmuSystem::gameName)
	{
		sprintStateFilename(str, S, slot, statePath, gameName);
	}
	static bool loadAutoState();
	static void saveAutoState();
	static void saveBackupMem();
	static void savePathChanged();
	static void resetGame();
	static void initOptions();
	static void onOptionsLoaded();
	static void writeConfig(Io *io);
	static bool readConfig(Io *io, uint key, uint readSize);
	static int loadGame(const char *path);
	typedef DelegateFunc<void (uint result, const Input::Event &e)> LoadGameCompleteDelegate;
	static LoadGameCompleteDelegate loadGameCompleteDel;
	static LoadGameCompleteDelegate &onLoadGameComplete() { return loadGameCompleteDel; }
	static void runFrame(bool renderGfx, bool processGfx, bool renderAudio) ATTRS(hot);
	static bool vidSysIsPAL();
	static uint multiresVideoBaseX();
	static uint multiresVideoBaseY();
	static void configAudioRate();
	static void configAudioPlayback()
	{
		auto prevFormat = pcmFormat;
		configAudioRate();
		audioFramesPerVideoFrame = pcmFormat.rate / (vidSysIsPAL() ? 50 : 60);
		if(prevFormat != pcmFormat && Audio::isOpen())
		{
			logMsg("PCM format has changed, closing existing playback");
			Audio::closePcm();
		}
	}
	static void clearInputBuffers();
	static void handleInputAction(uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static uint translateInputAction(uint input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static void stopSound();
	static void startSound();
	static void writeSound(const void *samples, uint framesToWrite);
	static void commitSound(Audio::BufferContext buffer, uint frames);
	static int setupFrameSkip(uint optionVal, Gfx::FrameTimeBase frameTime);
	static void setupGamePaths(const char *filePath);

	static void clearGamePaths()
	{
		strcpy(gameName, "");
		strcpy(fullGameName, "");
		strcpy(gamePath, "");
		strcpy(fullGamePath, "");
	}

	static TimeSys benchmark()
	{
		auto now = TimeSys::timeNow();
		iterateTimes(180, i)
		{
			runFrame(0, 1, 0);
		}
		auto after = TimeSys::timeNow();
		return after-now;
	}

	static bool gameIsRunning()
	{
		return !string_equal(gameName, "");
	}

	static void pause()
	{
		if(isActive())
			state = State::PAUSED;
		stopSound();
		cancelAutoSaveStateTimer();
	}

	static void start()
	{
		state = State::ACTIVE;
		clearInputBuffers();
		emuFrameNow = -1;
		startSound();
		startTime = {};
		startFrameTime = 0;
		startAutoSaveStateTimer();
	}

	static void closeSystem();
	static void closeGame(bool allowAutosaveState = 1);
};

enum { STATE_RESULT_OK, STATE_RESULT_NO_FILE, STATE_RESULT_NO_FILE_ACCESS, STATE_RESULT_IO_ERROR,
	STATE_RESULT_INVALID_DATA, STATE_RESULT_OTHER_ERROR };

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

enum { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };
Gfx::BufferImage &getAsset(uint assetID);

class EmuNavView : public BasicNavView
{
public:
	constexpr EmuNavView() { }
	void onLeftNavBtn(const Input::Event &e) override;
	void onRightNavBtn(const Input::Event &e) override;
};

extern StackAllocator menuAllocator;
extern uint8 modalViewStorage[2][1024] __attribute__((aligned));
extern uint modalViewStorageIdx;
template<typename T, typename... ARGS>
static T *allocModalView(ARGS&&... args)
{
	static_assert(sizeof(T) <= sizeof(modalViewStorage[0]), "out of modal view storage");
	auto obj = new(modalViewStorage[modalViewStorageIdx]) T(std::forward<ARGS>(args)...);
	modalViewStorageIdx = (modalViewStorageIdx + 1) % 2;
	return obj;
}

#if defined INPUT_SUPPORTS_POINTER
#define CONFIG_EMUFRAMEWORK_VCONTROLS
#endif

#include <CreditsView.hh>
#include <inGameActionKeys.hh>
#include <main/EmuConfig.hh>
