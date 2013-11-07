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
#include <audio/Audio.hh>
#include <algorithm>

EmuSystem::State EmuSystem::state = EmuSystem::State::OFF;
FsSys::cPath EmuSystem::gamePath = "";
FsSys::cPath EmuSystem::fullGamePath = "";
FsSys::cPath EmuSystem::savePath_ = "";
char EmuSystem::gameName[256] = "";
char EmuSystem::fullGameName[256] = "";
TimeSys EmuSystem::startTime;
Gfx::FrameTimeBase EmuSystem::startFrameTime = 0;
int EmuSystem::emuFrameNow;
int EmuSystem::saveStateSlot = 0;
Audio::PcmFormat EmuSystem::pcmFormat = Audio::pPCM;
uint EmuSystem::audioFramesPerVideoFrame = 0;
const uint EmuSystem::optionFrameSkipAuto = 32;
EmuSystem::LoadGameCompleteDelegate EmuSystem::loadGameCompleteDel;
Base::CallbackRef *EmuSystem::autoSaveStateCallbackRef = nullptr;
extern EmuNavView viewNav;
void fixFilePermissions(const char *path);

void saveAutoStateFromTimer();
static Base::CallbackDelegate autoSaveStateCallback()
{
	return []()
		{
			logMsg("auto-save state timer fired");
			EmuSystem::saveAutoState();
			EmuSystem::autoSaveStateCallbackRef = Base::callbackAfterDelaySec(autoSaveStateCallback(), 60*optionAutoSaveState);
		};
}

void EmuSystem::cancelAutoSaveStateTimer()
{
	if(autoSaveStateCallbackRef)
	{
		Base::cancelCallback(autoSaveStateCallbackRef);
		autoSaveStateCallbackRef = nullptr;
	}
}

void EmuSystem::startAutoSaveStateTimer()
{
	if(optionAutoSaveState > 1)
	{
		assert(!autoSaveStateCallbackRef);
		autoSaveStateCallbackRef = Base::callbackAfterDelaySec(autoSaveStateCallback(), 60*optionAutoSaveState); // minutes to seconds
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

int EmuSystem::setupFrameSkip(uint optionVal, Gfx::FrameTimeBase frameTime)
{
	static const uint maxFrameSkip = 6;
	static const double ntscNSecs = 1000000000./60., palNSecs = 1000000000./50.;
	static const auto ntscFrameTime = Gfx::decimalFrameTimeBaseFromSec(1./60.),
			palFrameTime = Gfx::decimalFrameTimeBaseFromSec(1./50.);
	if(!EmuSystem::vidSysIsPAL() && optionVal != optionFrameSkipAuto)
	{
		return optionVal; // constant frame-skip for NTSC source
	}

	int emuFrame;
	if(Base::supportsFrameTime())
	{
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
	}
	else
	{
		if(!startTime)
		{
			startTime.setTimeNow();
			emuFrame = 0;
			//logMsg("first frame time %f", (double)startTime);
		}
		else
		{
			auto timeTotal = TimeSys::timeNow() - startTime;
			emuFrame = std::round((double)timeTotal.toNs() / (vidSysIsPAL() ? palNSecs : ntscNSecs));
			//emuFrame = timeTotal.divByNSecs(vidSysIsPAL() ? palNSecs : ntscNSecs);
			//logMsg("on frame %d, was %d, total time %f", emuFrame, emuFrameNow, (double)timeTotal);
		}
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

	/*uint skip = 0;
	gfx_updateFrameTime();
	//logMsg("%d real frames passed", gfx_frameTimeRel);
	if(gfx_frameTimeRel > 1)
	{
		skip = min(gfx_frameTimeRel - 1, maxFrameSkip);
		if(skip && Audio::framesFree() < Audio::maxRate/12)
		{
			logMsg("not skipping %d frames from full audio buffer", skip);
			skip = 0;
		}
		else
		{
			logMsg("skipping %u frames", skip);
		}
	}
	if(gfx_frameTimeRel == 0)
	{
		logMsg("no frames passed");
		return -1;
	}
	return skip;*/
}

void EmuSystem::setupGamePaths(const char *filePath)
{
	{
		// find the realpath of the dirname portion separately in case the file is a symlink
		FsSys::cPath dirnameTemp;
		strcpy(gamePath, string_dirname(filePath, dirnameTemp));
		char realPath[PATH_MAX];
		if(!realpath(gamePath, realPath))
		{
			gamePath[0] = 0;
			logErr("error in realpath()");
			return;
		}
		strcpy(gamePath, realPath); // destination is always large enough
		logMsg("set game directory: %s", gamePath);
		#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath);
		#endif
	}

	{
		FsSys::cPath basenameTemp;
		string_copy(gameName, string_basename(filePath, basenameTemp));

		string_printf(fullGamePath, "%s/%s", gamePath, gameName);
		logMsg("set full game path: %s", fullGamePath);

		// If gameName has an extension, truncate it
		auto dotPos = strrchr(gameName, '.');
		if(dotPos)
			*dotPos = 0;
		logMsg("set game name: %s", gameName);
	}
}

void EmuSystem::closeGame(bool allowAutosaveState)
{
	if(gameIsRunning())
	{
		if(Audio::isOpen())
			Audio::clearPcm();
		if(allowAutosaveState)
			saveAutoState();
		logMsg("closing game %s", gameName);
		closeSystem();
		clearGamePaths();
		cancelAutoSaveStateTimer();
		viewNav.setRightBtnActive(0);
		state = State::OFF;
	}
}
