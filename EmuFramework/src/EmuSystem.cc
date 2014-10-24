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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/FileUtils.hh>
#include <imagine/audio/Audio.hh>
#include <algorithm>

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FsSys::PathString EmuSystem::gamePath_{};
FsSys::PathString EmuSystem::fullGamePath_{};
FsSys::PathString EmuSystem::savePath_{};
FsSys::PathString EmuSystem::defaultSavePath_{};
FsSys::PathString EmuSystem::gameSavePath_{};
char EmuSystem::gameName_[256]{};
char EmuSystem::fullGameName_[256]{};
Base::FrameTimeBase EmuSystem::startFrameTime = 0;
uint EmuSystem::emuFrameNow = 0;
bool EmuSystem::runFrameOnDraw = false;
int EmuSystem::saveStateSlot = 0;
Audio::PcmFormat EmuSystem::pcmFormat = Audio::pPCM;
uint EmuSystem::audioFramesPerVideoFrame = 0;
const uint EmuSystem::optionFrameSkipAuto = 32;
EmuSystem::LoadGameCompleteDelegate EmuSystem::loadGameCompleteDel;
Base::Timer EmuSystem::autoSaveStateTimer;
[[gnu::weak]] const bool EmuSystem::inputHasKeyboard = false;
[[gnu::weak]] const bool EmuSystem::hasBundledGames = false;

void saveAutoStateFromTimer();

void EmuSystem::cancelAutoSaveStateTimer()
{
	autoSaveStateTimer.deinit();
}

void EmuSystem::startAutoSaveStateTimer()
{
	if(optionAutoSaveState > 1)
	{
		auto secs = 60*optionAutoSaveState; // minutes to seconds
		autoSaveStateTimer.callbackAfterSec(
			[]()
			{
				logMsg("auto-save state timer fired");
				EmuSystem::saveAutoState();
			}, secs, secs);
	}
}

void EmuSystem::startSound()
{
	assert(audioFramesPerVideoFrame);
	if(optionSound)
	{
		if(!Audio::isOpen())
		{
			#ifdef CONFIG_AUDIO_LATENCY_HINT
			uint wantedLatency = std::round((float)optionSoundBuffers * (vidSysIsPAL() ? 20000.f : 1000000.f/60.f));
			logMsg("requesting audio latency %dus", wantedLatency);
			Audio::setHintOutputLatency(wantedLatency);
			#endif
			Audio::openPcm(pcmFormat);
		}
		else if(Audio::framesFree() <= (int)audioFramesPerVideoFrame)
			Audio::resumePcm();
	}
}

void EmuSystem::stopSound()
{
	if(optionSound)
	{
		//logMsg("stopping sound");
		Audio::pausePcm();
	}
}

void EmuSystem::writeSound(const void *samples, uint framesToWrite)
{
	Audio::writePcm(samples, framesToWrite);
	if(!Audio::isPlaying() && Audio::framesFree() <= (int)audioFramesPerVideoFrame)
	{
		logMsg("starting audio playback with %d frames free in buffer", Audio::framesFree());
		Audio::resumePcm();
	}
}

void EmuSystem::commitSound(Audio::BufferContext buffer, uint frames)
{
	Audio::commitPlayBuffer(buffer, frames);
	if(!Audio::isPlaying() && Audio::framesFree() <= (int)audioFramesPerVideoFrame)
	{
		logMsg("starting audio playback with %d frames free in buffer", Audio::framesFree());
		Audio::resumePcm();
	}
}

bool EmuSystem::stateExists(int slot)
{
	auto saveStr = sprintStateFilename(slot);
	return FsSys::fileExists(saveStr.data());
}

bool EmuSystem::loadAutoState()
{
	if(optionAutoSaveState)
	{
		if(loadState(-1))
		{
			logMsg("loaded autosave-state");
			return 1;
		}
	}
	return 0;
}

bool EmuSystem::shouldOverwriteExistingState()
{
	return !optionConfirmOverwriteState || !EmuSystem::stateExists(EmuSystem::saveStateSlot);
}

int EmuSystem::setupFrameSkip(uint optionVal, Base::FrameTimeBase frameTime)
{
	static const uint maxFrameSkip = 6;
	static constexpr auto ntscFrameTime = Base::frameTimeBaseFromS(1./60.),
		palFrameTime = Base::frameTimeBaseFromS(1./50.);
	if(!EmuSystem::vidSysIsPAL() && optionVal != optionFrameSkipAuto)
	{
		return optionVal; // constant frame-skip for NTSC source
	}

	uint emuFrame;
	if(unlikely(!startFrameTime))
	{
		//logMsg("first frame time %f", (double)frameTime);
		startFrameTime = frameTime;
		emuFrameNow = 0;
		return 0;
	}
	else
	{
		auto timeTotal = frameTime - startFrameTime;
		auto frame = IG::divRoundClosest(timeTotal, vidSysIsPAL() ? palFrameTime : ntscFrameTime);
		emuFrame = frame;
		//logMsg("last frame time %f, on frame %d, was %d, total time %f", (double)frameTime, emuFrame, emuFrameNow, (double)timeTotal);
	}
	if(emuFrame < emuFrameNow)
	{
		bug_exit("current frame %d is in the past (last one was %d)", emuFrame, emuFrameNow);
	}
	if(emuFrame == emuFrameNow)
	{
		//logMsg("repeating frame %d", emuFrame);
		return -1;
	}
	else
	{
		uint skip = std::min((emuFrame - emuFrameNow) - 1, maxFrameSkip);
		emuFrameNow = emuFrame;
		if(skip)
		{
			//logMsg("skipping %u frames", skip);
		}
		return skip;
	}
}

void EmuSystem::setupGamePaths(const char *filePath)
{
	{
		// find the realpath of the dirname portion separately in case the file is a symlink
		FsSys::PathString dirnameTemp;
		strcpy(gamePath_.data(), string_dirname(filePath, dirnameTemp));
		char realPath[PATH_MAX];
		if(!realpath(gamePath_.data(), realPath))
		{
			gamePath_[0] = 0;
			logErr("error in realpath()");
			return;
		}
		strcpy(gamePath_.data(), realPath); // destination is always large enough
		logMsg("set game directory: %s", gamePath_.data());
		fixFilePermissions(gamePath_);
	}

	{
		FsSys::PathString basenameTemp;
		string_copy(gameName_, string_basename(filePath, basenameTemp));

		string_printf(fullGamePath_, "%s/%s", gamePath_.data(), gameName_);
		logMsg("set full game path: %s", fullGamePath_.data());

		// If gameName has an extension, truncate it
		auto dotPos = strrchr(gameName_, '.');
		if(dotPos)
			*dotPos = 0;
		logMsg("set game name: %s", gameName_);
	}

	setupGameSavePath();
}

void EmuSystem::setupGameName(const char *name)
{
	{
		FsSys::PathString basenameTemp;
		string_copy(gameName_, string_basename(name, basenameTemp));

		// If gameName has an extension, truncate it
		auto dotPos = strrchr(gameName_, '.');
		if(dotPos)
			*dotPos = 0;
		logMsg("set game name: %s", gameName_);
	}

	setupGameSavePath();
}

void EmuSystem::setupGameSavePath()
{
	if(!strlen(gameName_))
		return;
	if(strlen(savePath_.data()))
	{
		if(string_equal(savePath_.data(), optionSavePathDefaultToken))
			setGameSavePath(defaultSavePath());
		else
			setGameSavePath(savePath_.data());
	}
	else
	{
		setGameSavePath(gamePath_.data());
	}
}

static bool hasWriteAccessToDir(const char *path)
{
	auto hasAccess = FsSys::hasWriteAccess(path);
	#ifdef CONFIG_BASE_ANDROID
	// only Android 4.4 also test file creation since
	// access() can still claim an SD card is writable
	// even though parts are locked-down by the OS
	if(Base::androidSDK() >= 19)
	{
		auto testFilePath = makeFSPathStringPrintf("%s/.safe-to-delete-me", path);
		FileIO testFile;
		if(testFile.create(testFilePath.data()) != OK)
		{
			hasAccess = false;
		}
		else
		{
			FsSys::remove(testFilePath.data());
		}
	}
	#endif
	return hasAccess;
}

void EmuSystem::setGameSavePath(const char *path)
{
	if(!strlen(gameName_))
		return;
	bool reportNoWriteAccess = false;
	// check if the path is writable
	if(path && strlen(path))
	{
		fixFilePermissions(path);
		if(optionCheckSavePathWriteAccess && !hasWriteAccessToDir(path))
		{
			reportNoWriteAccess = true;
		}
		else
		{
			logMsg("set game save path: %s", path);
			string_copy(gameSavePath_, path);
			return;
		}
	}
	// fallback to a default path
	logMsg("set game save path to default: %s", defaultSavePath());
	string_copy(gameSavePath_, defaultSavePath());
	fixFilePermissions(gameSavePath_);
	if(reportNoWriteAccess)
	{
		popup.printf(4, true, "Save path lacks write access, using default:\n%s", gameSavePath_.data());
	}
}

void EmuSystem::makeDefaultSavePath()
{
	assert(strlen(gameName_));
	FsSys::PathString pathTemp;
	string_printf(pathTemp, "%s/Game Data", Base::storagePath());
	FsSys::mkdir(pathTemp.data());
	string_cat(pathTemp, "/");
	string_cat(pathTemp, shortSystemName());
	FsSys::mkdir(pathTemp.data());
	string_cat(pathTemp, "/");
	string_cat(pathTemp, gameName_);
	FsSys::mkdir(pathTemp.data());
}

void EmuSystem::clearGamePaths()
{
	strcpy(gameName_, "");
	strcpy(fullGameName_, "");
	strcpy(gamePath_.data(), "");
	strcpy(fullGamePath_.data(), "");
	strcpy(defaultSavePath_.data(), "");
	strcpy(gameSavePath_.data(), "");
}

const char *EmuSystem::savePath()
{
	assert(strlen(gameSavePath_.data()));
	return gameSavePath_.data();
}

const char *EmuSystem::defaultSavePath()
{
	assert(strlen(gameName_));
	if(!strlen(defaultSavePath_.data()))
	{
		string_printf(defaultSavePath_, "%s/Game Data/%s/%s", Base::storagePath(), shortSystemName(), gameName_);
		logMsg("game default save path: %s", defaultSavePath_.data());
	}
	if(!FsSys::fileExists(defaultSavePath_.data()))
		makeDefaultSavePath();
	return defaultSavePath_.data();
}

FsSys::PathString EmuSystem::baseDefaultGameSavePath()
{
	return makeFSPathStringPrintf("%s/Game Data/%s", Base::storagePath(), shortSystemName());
}

void EmuSystem::closeGame(bool allowAutosaveState)
{
	if(gameIsRunning())
	{
		if(Audio::isOpen())
			Audio::clearPcm();
		if(allowAutosaveState)
			saveAutoState();
		logMsg("closing game %s", gameName_);
		closeSystem();
		clearGamePaths();
		cancelAutoSaveStateTimer();
		viewNav.setRightBtnActive(0);
		state = State::OFF;
	}
}

void EmuSystem::resetFrameTime()
{
	startFrameTime = 0;
}

void EmuSystem::pause()
{
	if(isActive())
		state = State::PAUSED;
	stopSound();
	cancelAutoSaveStateTimer();
}

void EmuSystem::start()
{
	state = State::ACTIVE;
	clearInputBuffers();
	resetFrameTime();
	startSound();
	startAutoSaveStateTimer();
}
