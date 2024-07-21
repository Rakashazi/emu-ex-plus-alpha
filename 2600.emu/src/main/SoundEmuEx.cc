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
  mixRate = {};
}

void SoundEmuEx::updateResampler()
{
	emulationTiming->updatePlaybackRate(mixRate);
	Resampler::Format formatFrom =
		Resampler::Format(emulationTiming->audioSampleRate(), audioQueue->fragmentSize(), audioQueue->isStereo());
	Resampler::Format formatTo =
		Resampler::Format(mixRate, audioQueue->fragmentSize(), false);
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
	logMsg("set sound mix rate:%d resampler type:%d", mixRate, (int)resampleQuality);
}

void SoundEmuEx::setMixRate(int mixRate_, AudioSettings::ResamplingQuality resampleQ)
{
	resampleQuality = resampleQ;
	if(!audioQueue)
	{
		logWarn("called setRate() without audio queue");
		return;
	}
	if(mixRate_ == mixRate)
		return;
	mixRate = mixRate_;
	updateResampler();
}

void SoundEmuEx::setResampleQuality(AudioSettings::ResamplingQuality quality)
{
	if(!audioQueue)
	{
		return;
	}
	resampleQuality = quality;
	if(!mixRate)
		return;
	updateResampler();
}

void SoundEmuEx::setEmuAudio(EmuEx::EmuAudio *audio)
{
	audioQueue->onFragmentEnqueued =
	[this, audio](AudioQueue &queue, uInt32 fragFrames)
	{
		const int samplesPerFrame = 1; //audioQueue->isStereo() ? 2 : 1;
		[[maybe_unused]] const int fragSamples = fragFrames * samplesPerFrame;
		[[maybe_unused]] int wroteFrames = 0;
		//logDMsg("%d fragments of %d size ready", audioQueue->size(), fragFrames);
		while(queue.size())
		{
			float buffF[512];
			assert(fragSamples <= std::ssize(buffF));
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
