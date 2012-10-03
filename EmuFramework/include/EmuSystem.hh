/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <io/Io.hh>
#include <fs/sys.hh>
#include <util/time/sys.hh>
#include <util/audio/PcmFormat.hh>
#include <config/env.hh>
#include <gui/FSPicker/FSPicker.hh>
#include <util/gui/ViewStack.hh>

extern BasicNavView viewNav;

class EmuSystem
{
	public:
	static bool active;
	static FsSys::cPath gamePath, fullGamePath;
	static char gameName[256], fullGameName[256];
	static Base::CallbackRef *autoSaveStateCallbackRef;
	static int saveStateSlot;
	static TimeSys startTime;
	static int emuFrameNow;
	static Audio::PcmFormat pcmFormat;
	static const uint optionFrameSkipAuto;
	static uint aspectRatioX, aspectRatioY;
	static const uint maxPlayers;

	static void cancelAutoSaveStateTimer();
	static void startAutoSaveStateTimer();
	static int loadState(int slot = saveStateSlot);
	static int saveState();
	static bool stateExists(int slot);
	static void sprintStateFilename(char *str, size_t size, int slot,
		const char *gamePath = EmuSystem::gamePath, const char *gameName = EmuSystem::gameName);
	template <size_t S>
	static void sprintStateFilename(char (&str)[S], int slot,
		const char *gamePath = EmuSystem::gamePath, const char *gameName = EmuSystem::gameName)
	{
		sprintStateFilename(str, S, slot, gamePath, gameName);
	}
	static bool loadAutoState();
	static void saveAutoState();
	static void saveBackupMem();
	static void resetGame();
	static void initOptions();
	static void writeConfig(Io *io);
	static bool readConfig(Io *io, uint key, uint readSize);
	static int loadGame(const char *path);
	typedef Delegate<void (uint result, const InputEvent &e)> LoadGameCompleteDelegate;
	static LoadGameCompleteDelegate loadGameCompleteDel;
	static LoadGameCompleteDelegate &loadGameCompleteDelegate() { return loadGameCompleteDel; }
	static void runFrame(bool renderGfx, bool processGfx, bool renderAudio) ATTRS(hot);
	static bool vidSysIsPAL();
	static void configAudioRate();
	static void clearInputBuffers();
	static void handleInputAction(uint player, uint state, uint emuKey);
	static uint translateInputAction(uint input, bool &turbo);
	static uint translateInputAction(uint input)
	{
		bool turbo;
		return translateInputAction(input, turbo);
	}
	static void handleOnScreenInputAction(uint state, uint emuKey);
	static void stopSound();
	static void startSound();
	static int setupFrameSkip(uint optionVal);

	static TimeSys benchmark()
	{
		TimeSys now, after;
		now.setTimeNow();
		iterateTimes(180, i)
		{
			runFrame(0, 1, 0);
		}
		after.setTimeNow();
		return after-now;
	}

	static bool gameIsRunning()
	{
		return !string_equal(gameName, "");
	}

	static void pause()
	{
		active = 0;
		stopSound();
		cancelAutoSaveStateTimer();
	}

	static void start()
	{
		active = 1;
		clearInputBuffers();
		emuFrameNow = -1;
		startSound();
		startTime.setTimeNow();
		startAutoSaveStateTimer();
	}

	static void closeSystem();
	static void closeGame(bool allowAutosaveState = 1)
	{
		if(gameIsRunning())
		{
			if(allowAutosaveState)
				saveAutoState();
			logMsg("closing game %s", gameName);
			closeSystem();
			strcpy(gameName, "");
			strcpy(fullGameName, "");
			cancelAutoSaveStateTimer();
			viewNav.setRightBtnActive(0);
		}
	}
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

enum TriggerPosType { TRIGGERS_INLINE = 0, TRIGGERS_RIGHT = 1, TRIGGERS_LEFT = 2, TRIGGERS_SPLIT = 3 };

#include <CreditsView.hh>

struct ConstKeyProfile
{
	const char *name;
	const uint *key;
};

#include <inGameActionKeys.hh>
#include <main/EmuConfig.hh>
