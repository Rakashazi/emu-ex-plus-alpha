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

#include <EmuSystem.hh>
#include <EmuOptions.hh>
#include <EmuApp.hh>
#include <imagine/audio/Audio.hh>
#include <algorithm>

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FsSys::cPath EmuSystem::gamePath_ = "";
FsSys::cPath EmuSystem::fullGamePath_ = "";
FsSys::cPath EmuSystem::savePath_ = "";
FsSys::cPath EmuSystem::defaultSavePath_ = "";
char EmuSystem::gameName_[256] = "";
char EmuSystem::fullGameName_[256] = "";
TimeSys EmuSystem::startTime;
Base::FrameTimeBase EmuSystem::startFrameTime = 0;
int EmuSystem::emuFrameNow;
int EmuSystem::saveStateSlot = 0;
Audio::PcmFormat EmuSystem::pcmFormat = Audio::pPCM;
uint EmuSystem::audioFramesPerVideoFrame = 0;
const uint EmuSystem::optionFrameSkipAuto = 32;
EmuSystem::LoadGameCompleteDelegate EmuSystem::loadGameCompleteDel;
Base::Timer EmuSystem::autoSaveStateTimer;
void fixFilePermissions(const char *path);

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
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, slot);
	return FsSys::fileExists(saveStr);
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

//static int autoFrameSkipLevel = 0;
//static int lowBufferFrames = (audio_maxRate/60)*3, highBufferFrames = (audio_maxRate/60)*5;

int EmuSystem::setupFrameSkip(uint optionVal, Base::FrameTimeBase frameTime)
{
	static const uint maxFrameSkip = 6;
	static const auto ntscFrameTime = Base::decimalFrameTimeBaseFromSec(1./60.),
			palFrameTime = Base::decimalFrameTimeBaseFromSec(1./50.);
	if(!EmuSystem::vidSysIsPAL() && optionVal != optionFrameSkipAuto)
	{
		return optionVal; // constant frame-skip for NTSC source
	}

	int emuFrame;
	if(!startFrameTime)
	{
		startFrameTime = frameTime;
		emuFrame = 0;
		//logMsg("first frame time %f", (double)frameTime);
	}
	else
	{
		auto timeTotal = frameTime - startFrameTime;
		auto frame = std::round(timeTotal / (vidSysIsPAL() ? palFrameTime : ntscFrameTime));
		emuFrame = frame;
		//logMsg("last frame time %f, on frame %d, was %d, total time %f", (double)frameTime, emuFrame, emuFrameNow, (double)timeTotal);
	}
	assert(emuFrame >= emuFrameNow);
	if(emuFrame == emuFrameNow)
	{
		//logMsg("repeating frame %d", emuFrame);
		return -1;
	}
	else
	{
		uint skip = std::min((emuFrame - emuFrameNow) - 1, (int)maxFrameSkip);
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
		FsSys::cPath dirnameTemp;
		strcpy(gamePath_, string_dirname(filePath, dirnameTemp));
		char realPath[PATH_MAX];
		if(!realpath(gamePath_, realPath))
		{
			gamePath_[0] = 0;
			logErr("error in realpath()");
			return;
		}
		strcpy(gamePath_, realPath); // destination is always large enough
		logMsg("set game directory: %s", gamePath_);
		#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath_);
		#endif
	}

	{
		FsSys::cPath basenameTemp;
		string_copy(gameName_, string_basename(filePath, basenameTemp));

		string_printf(fullGamePath_, "%s/%s", gamePath_, gameName_);
		logMsg("set full game path: %s", fullGamePath_);

		// If gameName has an extension, truncate it
		auto dotPos = strrchr(gameName_, '.');
		if(dotPos)
			*dotPos = 0;
		logMsg("set game name: %s", gameName_);
	}

	string_printf(defaultSavePath_, "%s/Game Data/%s/%s", Base::storagePath(), shortSystemName(), gameName_);
	logMsg("set default save path: %s", defaultSavePath_);
}

void EmuSystem::setupGameName(const char *name)
{
	{
		FsSys::cPath basenameTemp;
		string_copy(gameName_, string_basename(name, basenameTemp));

		// If gameName has an extension, truncate it
		auto dotPos = strrchr(gameName_, '.');
		if(dotPos)
			*dotPos = 0;
		logMsg("set game name: %s", gameName_);
	}

	string_printf(defaultSavePath_, "%s/Game Data/%s/%s", Base::storagePath(), shortSystemName(), gameName_);
	logMsg("set default save path: %s", defaultSavePath_);
}

void EmuSystem::makeDefaultSavePath()
{
	FsSys::cPath pathTemp;
	string_printf(pathTemp, "%s/Game Data", Base::storagePath());
	FsSys::mkdir(pathTemp);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, shortSystemName());
	FsSys::mkdir(pathTemp);
	string_cat(pathTemp, "/");
	string_cat(pathTemp, gameName_);
	FsSys::mkdir(pathTemp);
}

void EmuSystem::clearGamePaths()
{
	strcpy(gameName_, "");
	strcpy(fullGameName_, "");
	strcpy(gamePath_, "");
	strcpy(fullGamePath_, "");
	strcpy(defaultSavePath_, "");
}

const char *EmuSystem::savePath()
{
	if(strlen(savePath_))
	{
		return savePath_;
	}
	// check if the game's path is writable
	if(strlen(gamePath_) && FsSys::hasWriteAccess(gamePath_))
	{
		return gamePath_;
	}
	// fallback to a default path
	assert(strlen(defaultSavePath()));
	if(!FsSys::fileExists(defaultSavePath()))
		makeDefaultSavePath();
	return defaultSavePath();
}

const char *EmuSystem::defaultSavePath()
{
	return defaultSavePath_;
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
