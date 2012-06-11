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

#define thisModuleName "audio:ps3"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <cell/sysmodule.h>
#include <cell/audio.h>

namespace Audio
{

PcmFormat preferredPcmFormat { 48000, &SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static uint32_t port;
static float *portBuffer;
static sys_addr_t readIndexAddr;
static bool pcmPlaying = 0;
static uint currBlock;
static const uint blocks = 8;
static uint blockBytes;
static const uint maxBlockBytes = 256*4;

CallResult openPcm(const PcmFormat &format)
{
	CellAudioPortParam param { CELL_AUDIO_PORT_2CH, blocks == 16 ? CELL_AUDIO_BLOCK_16 : CELL_AUDIO_BLOCK_8, 0, 0 };
	cellAudioPortOpen(&param, &port);

	CellAudioPortConfig config;
	cellAudioGetPortConfig(port, &config);
	portBuffer = (float*)config.portAddr;
	readIndexAddr = config.readIndexAddr;
	currBlock = 0;
	pcmPlaying = 1;
	cellAudioPortStart(port);
	logMsg("opened audio port %d, addr %p, size %d", (int)port, portBuffer, (int)config.portSize);
	assert(format.rate == 48000 && format.channels == 2);
	pcmFormat = format;
	blockBytes = pcmFormat.framesToBytes(256);
	return OK;
}

void writePcm(uchar *buffer, uint framesToWrite)
{
	uint readingBlock = *(uint64_t *)readIndexAddr;

	static int debugCount = 0;
	if(debugCount == 120)
	{
		uint freeBlocks = 0;
		uint block = currBlock;
		while(block != readingBlock)
		{
			freeBlocks++;
			IG::incWrappedSelf(block, blocks);
		}
		logMsg("%d blocks queued", blocks - freeBlocks);
		debugCount = 0;
	}
	else
		debugCount++;

	static uchar partialBlock[maxBlockBytes];
	static uint partialBlockSamples = 0;
	uint samples = framesToWrite*2;

	while(samples != 0)
	{
		if(currBlock == readingBlock)
		{
			// TODO: fix overrun/underrun detection
			logMsg("can't write data with full ring buffer");
			return;
		}

		float *block = portBuffer + (512 * currBlock);

		// finish partial block
		if(partialBlockSamples && samples + partialBlockSamples >= 512)
		{
			iterateTimes(partialBlockSamples, i)
			{
				*block = float(((int16*)partialBlock)[i]) / 32768.f;
				block++;
			}
			iterateTimes(512 - partialBlockSamples, i)
			{
				*block = float(*((int16*)buffer)) / 32768.f;
				block++;
				buffer += 2;
			}
			assert(samples >= 512 - partialBlockSamples);
			samples -= 512 - partialBlockSamples;
			partialBlockSamples = 0;
		}
		else if (samples >= 512) // write full block
		{
			iterateTimes(512, i)
			{
				*block = float(*((int16*)buffer)) / 32768.f;
				block++;
				buffer += 2;
			}
			samples -= 512;
		}
		else // copy leftover into temp block
		{
			memcpy(partialBlock + partialBlockSamples*2, buffer, samples*2);
			partialBlockSamples += samples;
			return;
		}

		IG::incWrappedSelf(currBlock, blocks);
	}
}

void closePcm()
{
	if(pcmPlaying)
	{
		logMsg("closing audio port %d", (int)port);
		cellAudioPortClose(port);
		pcmPlaying = 0;
	}
}

int frameDelay()
{
	return 0; // TODO
}

int framesFree()
{
	return 0; // TODO
}

CallResult init()
{
	//logMsg("init audio");
	cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO);
	cellAudioInit();
	return(OK);
}

}

#undef thisModuleName
