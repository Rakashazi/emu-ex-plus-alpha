/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <stella/emucore/Settings.hxx>
#include <stella/emucore/EmulationTiming.hxx>
#include <stella/common/AudioQueue.hxx>
#include <stella/common/audio/SimpleResampler.hxx>
#include <stella/common/audio/LanczosResampler.hxx>
#include <OSystem.hxx>
#include <SoundEmuEx.hh>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAudio.hh>
#include <imagine/logger/logger.h>
#undef Debugger

using namespace EmuEx;

SoundEmuEx::SoundEmuEx(OSystem& osystem): Sound(osystem) {}

void SoundEmuEx::open(shared_ptr<AudioQueue> audioQueue, EmulationTiming* emulationTiming)
{
	audioQueue->ignoreOverflows(true);
	this->audioQueue = audioQueue;
	this->emulationTiming = emulationTiming;
	currentFragment = nullptr;
}

void SoundEmuEx::close()
{
	if(!audioQueue)
		return;
	audioQueue->closeSink(currentFragment);
  audioQueue.reset();
  myResampler.reset();
  inputFrameRate = {};
}

void SoundEmuEx::configForVideoFrameRate(double frameRate)
{
	logMsg("configuring audio rate:%d for video frame rate input:%.2f output:%.2f",
		outputRate, frameRate, 1. / outputFrameTime.count());
	auto tiaSoundRate = EmuSystem::audioMixRate(outputRate, frameRate, outputFrameTime);
	emulationTiming->updatePlaybackRate(tiaSoundRate);
	Resampler::Format formatFrom =
		Resampler::Format(emulationTiming->audioSampleRate(), audioQueue->fragmentSize(), audioQueue->isStereo());
	Resampler::Format formatTo =
		Resampler::Format(tiaSoundRate, audioQueue->fragmentSize(), false);
	Resampler::NextFragmentCallback fragCallback =
		[this]()
		{
			auto nextFragment = audioQueue->dequeue(currentFragment);
			if(nextFragment)
				currentFragment = nextFragment;
			return nextFragment;
		};
	switch(resampleQuality)
	{
		case AudioSettings::ResamplingQuality::nearestNeightbour:
			myResampler = make_unique<SimpleResampler>(formatFrom, formatTo, fragCallback);
		break;
		case AudioSettings::ResamplingQuality::lanczos_2:
			myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, fragCallback, 2);
		break;
		case AudioSettings::ResamplingQuality::lanczos_3:
			myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, fragCallback, 3);
		break;
	}
	logMsg("set sound rate:%.2f resampler type:%d", tiaSoundRate, (int)resampleQuality);
}

void SoundEmuEx::setRate(int outputRate, double inputFrameRate, FloatSeconds outputFrameTime, AudioSettings::ResamplingQuality resampleQ)
{
	this->outputRate = outputRate;
	this->inputFrameRate = inputFrameRate;
	this->outputFrameTime = outputFrameTime;
	resampleQuality = resampleQ;
	if(!audioQueue)
	{
		logWarn("called setRate() without audio queue");
		return;
	}
	configForVideoFrameRate(inputFrameRate);
}

void SoundEmuEx::setResampleQuality(AudioSettings::ResamplingQuality quality)
{
	if(!audioQueue)
	{
		return;
	}
	resampleQuality = quality;
	if(!inputFrameRate)
		return;
	configForVideoFrameRate(inputFrameRate);
}

void SoundEmuEx::setEmuAudio(EmuEx::EmuAudio *audio)
{
	audioQueue->onFragmentEnqueued =
	[this, audio](AudioQueue &queue, uInt32 fragFrames)
	{
		const uint32_t samplesPerFrame = 1; //audioQueue->isStereo() ? 2 : 1;
		const uint32_t fragSamples = fragFrames * samplesPerFrame;
		uint32_t wroteFrames = 0;
		//logDMsg("%d fragments of %d size ready", audioQueue->size(), fragFrames);
		while(queue.size())
		{
			float buffF[512];
			assert(fragSamples <= std::size(buffF));
			myResampler->fillFragment(buffF, fragFrames);
			if(audio)
			{
				audio->writeFrames(buffF, fragFrames);
				wroteFrames += fragFrames;
			}
		}
		//logDMsg("wrote %d audio frames", (int)wroteFrames);
	};
}

void SoundEmuEx::setEnabled(bool enable) {}

bool SoundEmuEx::mute(bool state) { return false; }

bool SoundEmuEx::toggleMute() { return false; }

void SoundEmuEx::setVolume(uInt32 percent) {}

void SoundEmuEx::adjustVolume(int direction) {}

string SoundEmuEx::about() const { return ""; }

void SoundEmuEx::queryHardware(VariantList& devices) {}
