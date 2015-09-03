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
#include <imagine/util/assume.h>
#include <algorithm>

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FS::PathString EmuSystem::gamePath_{};
FS::PathString EmuSystem::fullGamePath_{};
FS::PathString EmuSystem::savePath_{};
FS::PathString EmuSystem::defaultSavePath_{};
FS::PathString EmuSystem::gameSavePath_{};
char EmuSystem::gameName_[256]{};
char EmuSystem::fullGameName_[256]{};
Base::FrameTimeBase EmuSystem::startFrameTime = 0;
Base::FrameTimeBase EmuSystem::timePerVideoFrame = 0;
uint EmuSystem::emuFrameNow = 0;
bool EmuSystem::runFrameOnDraw = false;
int EmuSystem::saveStateSlot = 0;
Audio::PcmFormat EmuSystem::pcmFormat = {44100, Audio::SampleFormats::s16, 2};
uint EmuSystem::audioFramesPerVideoFrame = 0;
EmuSystem::LoadGameCompleteDelegate EmuSystem::loadGameCompleteDel;
Base::Timer EmuSystem::autoSaveStateTimer;
[[gnu::weak]] const bool EmuSystem::inputHasKeyboard = false;
[[gnu::weak]] const bool EmuSystem::hasBundledGames = false;
[[gnu::weak]] const bool EmuSystem::hasPALVideoSystem = false;
double EmuSystem::frameTimeNative = 1./60.;
double EmuSystem::frameTimePAL = 1./50.;
[[gnu::weak]] const bool EmuSystem::hasResetModes = false;

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
			uint wantedLatency = std::round(optionSoundBuffers * (1000000. * frameTime()));
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
	return FS::exists(saveStr.data());
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

uint EmuSystem::advanceFramesWithTime(Base::FrameTimeBase time)
{
	if(unlikely(!startFrameTime))
	{
		// first frame
		startFrameTime = time;
		emuFrameNow = 0;
		return 1;
	}
	assumeExpr(timePerVideoFrame > 0);
	assumeExpr(startFrameTime > 0);
	assumeExpr(time > startFrameTime);
	auto timeTotal = time - startFrameTime;
	auto frameNow = IG::divRoundClosest(timeTotal, timePerVideoFrame);
	auto elapsedEmuFrames = frameNow - emuFrameNow;
	emuFrameNow = frameNow;
	return elapsedEmuFrames;
}

void EmuSystem::setupGamePaths(const char *filePath)
{
	{
		// find the realpath of the dirname portion separately in case the file is a symlink
		strcpy(gamePath_.data(), FS::dirname(filePath).data());
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

	string_copy(gameName_, FS::basename(filePath).data());

	string_printf(fullGamePath_, "%s/%s", gamePath_.data(), gameName_);
	logMsg("set full game path: %s", fullGamePath_.data());

	// If gameName has an extension, truncate it
	auto dotPos = strrchr(gameName_, '.');
	if(dotPos)
		*dotPos = 0;
	logMsg("set game name: %s", gameName_);

	setupGameSavePath();
}

void EmuSystem::setupGameName(const char *name)
{
	{
		string_copy(gameName_, FS::basename(name).data());

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
	auto hasAccess = FS::access(path, FS::acc::w);
	#ifdef CONFIG_BASE_ANDROID
	// only Android 4.4 also test file creation since
	// access() can still claim an SD card is writable
	// even though parts are locked-down by the OS
	if(Base::androidSDK() >= 19)
	{
		auto testFilePath = FS::makePathStringPrintf("%s/.safe-to-delete-me", path);
		FileIO testFile;
		if(testFile.create(testFilePath.data()) != OK)
		{
			hasAccess = false;
		}
		else
		{
			FS::remove(testFilePath);
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
	FS::PathString pathTemp = Base::storagePath();
	string_cat(pathTemp, "/Game Data");
	FS::create_directory(pathTemp);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, shortSystemName());
	FS::create_directory(pathTemp);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, gameName_);
	FS::create_directory(pathTemp);
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
		string_printf(defaultSavePath_, "%s/Game Data/%s/%s", Base::storagePath().data(), shortSystemName(), gameName_);
		logMsg("game default save path: %s", defaultSavePath_.data());
	}
	if(!FS::exists(defaultSavePath_.data()))
		makeDefaultSavePath();
	return defaultSavePath_.data();
}

FS::PathString EmuSystem::baseDefaultGameSavePath()
{
	return FS::makePathStringPrintf("%s/Game Data/%s", Base::storagePath().data(), shortSystemName());
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

IG::Time EmuSystem::benchmark()
{
	auto now = IG::Time::now();
	iterateTimes(180, i)
	{
		runFrame(0, 1, 0);
	}
	auto after = IG::Time::now();
	return after-now;
}

void EmuSystem::configFrameTime()
{
	configAudioRate(frameTime());
	audioFramesPerVideoFrame = std::ceil(pcmFormat.rate * frameTime());
	timePerVideoFrame = Base::frameTimeBaseFromSecs(frameTime());
	resetFrameTime();
}

void EmuSystem::configAudioPlayback()
{
	auto prevFormat = pcmFormat;
	configFrameTime();
	if(prevFormat != pcmFormat && Audio::isOpen())
	{
		logMsg("PCM format has changed, closing existing playback");
		Audio::closePcm();
	}
}

double EmuSystem::frameTime()
{
	return frameTime(vidSysIsPAL() ? VIDSYS_PAL : VIDSYS_NATIVE_NTSC);
}

double EmuSystem::frameTime(VideoSystem system)
{
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return frameTimeNative;
		case VIDSYS_PAL: return frameTimePAL;
	}
	return 0;
}

double EmuSystem::defaultFrameTime(VideoSystem system)
{
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return 1./60.;
		case VIDSYS_PAL: return 1./50.;
	}
	return 0;
}

bool EmuSystem::frameTimeIsValid(VideoSystem system, double time)
{
	auto rate = 1. / time; // convert to frames per second
	switch(system)
	{
		case VIDSYS_NATIVE_NTSC: return rate >= 55 && rate <= 65;
		case VIDSYS_PAL: return rate >= 45 && rate <= 65;
	}
	return false;
}

bool EmuSystem::setFrameTime(VideoSystem system, double time)
{
	if(!frameTimeIsValid(system, time))
		return false;
	switch(system)
	{
		bcase VIDSYS_NATIVE_NTSC: frameTimeNative = time;
		bcase VIDSYS_PAL: frameTimePAL = time;
	}
	return true;
}

[[gnu::weak]] void EmuSystem::onMainWindowCreated(Base::Window &win) {}

[[gnu::weak]] void EmuSystem::onCustomizeNavView(EmuNavView &view) {}
