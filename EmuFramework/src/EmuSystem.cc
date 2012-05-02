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

#include <EmuSystem.hh>
#include <Option.hh>
#include <audio/Audio.hh>
extern BasicByteOption optionSound;

bool EmuSystem::active = 0;
FsSys::cPath EmuSystem::gamePath = "";
FsSys::cPath EmuSystem::fullGamePath = "";
char EmuSystem::gameName[256] = "";
char EmuSystem::fullGameName[256] = "";
int EmuSystem::autoSaveStateFrameCount = 0;
int EmuSystem::autoSaveStateFrames = 0;
TimeSys EmuSystem::startTime;
int EmuSystem::emuFrameNow;
int EmuSystem::saveStateSlot = 0;
Audio::PcmFormat EmuSystem::pcmFormat = Audio::pPCM;
const uint EmuSystem::optionFrameSkipAuto = 32;
EmuSystem::LoadGameCompleteDelegate EmuSystem::loadGameCompleteDel;

void EmuSystem::startSound()
{
	if(optionSound)
	{
		Audio::openPcm(pcmFormat);
	}
}

void EmuSystem::stopSound()
{
	if(optionSound)
	{
		//logMsg("stopping sound");
		Audio::closePcm();
	}
}

bool EmuSystem::stateExists(int slot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, slot);
	return Fs::fileExists(saveStr);
}

//static int autoFrameSkipLevel = 0;
//static int lowBufferFrames = (audio_maxRate/60)*3, highBufferFrames = (audio_maxRate/60)*5;

int EmuSystem::setupFrameSkip(uint optionVal)
{
	static const uint maxFrameSkip = 6;
	static const uint ntscUSecs = 16666, palUSecs = 20000;
	if(!EmuSystem::vidSysIsPAL() && optionVal != optionFrameSkipAuto)
	{
		return optionVal; // constant frame-skip for NTSC source
	}

	TimeSys realTime;
	realTime.setTimeNow();
	TimeSys timeTotal = realTime - startTime;

	int emuFrame = timeTotal.divByUSecs(vidSysIsPAL() ? palUSecs : ntscUSecs);
	//logMsg("on frame %d, was %d", emuFrame, emuFrameNow);
	if(emuFrame <= emuFrameNow)
	{
		//logMsg("repeating frame");
		return -1;
	}
	else
	{
		uint skip = IG::min((emuFrame - emuFrameNow) - 1, (int)maxFrameSkip);
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
