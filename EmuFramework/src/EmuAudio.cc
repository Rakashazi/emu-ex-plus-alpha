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

#define LOGTAG "EmuAudio"
#include "EmuOptions.hh"
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/Option.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>

namespace EmuEx
{

struct AudioStats
{
	constexpr AudioStats() = default;
	int underruns{};
	int overruns{};
	std::atomic_uint callbacks{};
	std::atomic_uint callbackBytes{};

	void reset()
	{
		underruns = overruns = 0;
		callbacks = 0;
		callbackBytes = 0;
	}
};

#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
static AudioStats audioStats{};
static IG::Timer audioStatsTimer{"audioStatsTimer"};
#endif

static void startAudioStats(IG::Audio::Format format)
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStats.reset();
	audioStatsTimer.run(IG::Seconds(1), IG::Seconds(1), {},
		[format]()
		{
			auto frames = format.bytesToFrames(audioStats.callbackBytes);
			emuViewController.updateEmuAudioStats(audioStats.underruns, audioStats.overruns,
				audioStats.callbacks, frames / (double)audioStats.callbacks, frames);
			audioStats.callbacks = 0;
			audioStats.callbackBytes = 0;
		});
	#endif
}

static void stopAudioStats()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsTimer.deinit();
	clearEmuAudioStats();
	#endif
}

EmuAudio::EmuAudio(const IG::Audio::Manager &audioManager):
	audioManager{audioManager},
	defaultRate{EmuSystem::forcedSoundRate ? EmuSystem::forcedSoundRate : audioManager.nativeRate()},
	rate_{defaultRate} {}

size_t EmuAudio::framesFree() const
{
	return format().bytesToFrames(rBuff.freeSpace());
}

size_t EmuAudio::framesWritten() const
{
	return format().bytesToFrames(rBuff.size());
}

size_t EmuAudio::framesCapacity() const
{
	return format().bytesToFrames(rBuff.capacity());
}

bool EmuAudio::shouldStartAudioWrites(size_t bytesToWrite) const
{
	// audio starts when the buffer reaches target size
	return rBuff.size() + bytesToWrite >= targetBufferFillBytes;
}

template<typename T>
static void simpleResample(T * __restrict__ dest, size_t destFrames, const T * __restrict__ src, size_t srcFrames)
{
	if(!destFrames)
		return;
	float ratio = (float)srcFrames/(float)destFrames;
	for(auto i : iotaCount(destFrames))
	{
		size_t srcPos = std::floor((float)i * ratio);
		if(srcPos >= srcFrames) [[unlikely]]
		{
			logMsg("resample pos %zu too high", srcPos);
			srcPos = srcFrames-1;
		}
		dest[i] = src[srcPos];
	}
}

static void simpleResample(void *dest, size_t destFrames, const void *src, size_t srcFrames, IG::Audio::Format format)
{
	if(format.channels == 1)
	{
		if(format.sample.isFloat())
			simpleResample((uint32_t*)dest, destFrames, (uint32_t*)src, srcFrames);
		else
			simpleResample((uint16_t*)dest, destFrames, (uint16_t*)src, srcFrames);
	}
	else
	{
		if(format.channels != 2)
		{
			bug_unreachable("channels == %d", format.channels);
		}
		if(format.sample.isFloat())
			simpleResample((uint64_t*)dest, destFrames, (uint64_t*)src, srcFrames);
		else
			simpleResample((uint32_t*)dest, destFrames, (uint32_t*)src, srcFrames);
	}
}

void EmuAudio::resizeAudioBuffer(size_t targetBufferFillBytes)
{
	auto oldCapacity = rBuff.capacity();
	rBuff.setMinCapacity(targetBufferFillBytes + bufferIncrementBytes);
	if(Config::DEBUG_BUILD && rBuff.capacity() != oldCapacity)
	{
		logMsg("created audio buffer:%zu frames (%.4fs), fill target:%zu frames (%.4fs)",
			format().bytesToFrames(rBuff.freeSpace()), format().bytesToTime(rBuff.freeSpace()).count(),
			format().bytesToFrames(targetBufferFillBytes), format().bytesToTime(targetBufferFillBytes).count());
	}
}

void EmuAudio::open()
{
	close();
	if(isEnabled())
		audioStream.setApi(audioManager, outputAPI());
}

void EmuAudio::start(FloatSeconds bufferDuration)
{
	FloatSeconds targetBufferFillDuration = soundBuffers * bufferDuration;
	if(!audioStream)
	{
		logMsg("sound is disabled");
		return;
	}
	lastUnderrunTime = {};
	auto inputFormat = format();
	targetBufferFillBytes = inputFormat.timeToBytes(targetBufferFillDuration);
	bufferIncrementBytes = inputFormat.timeToBytes(bufferDuration);
	if(!audioStream.isOpen())
	{
		resizeAudioBuffer(targetBufferFillBytes);
		audioWriteState = AudioWriteState::BUFFER;
		IG::Audio::Format outputFormat{inputFormat.rate, audioManager.nativeSampleFormat(), inputFormat.channels};
		IG::Audio::OutputStreamConfig outputConf
		{
			outputFormat,
			[this, outputSampleFormat = outputFormat.sample, inputSampleFormat = inputFormat.sample, channels = outputFormat.channels](void *samples, size_t frames)
			{
				IG::Audio::Format outputFormat{{}, outputSampleFormat, channels};
				#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
				audioStats.callbacks++;
				audioStats.callbackBytes += bytes;
				#endif
				if(audioWriteState == AudioWriteState::ACTIVE)
				{
					IG::Audio::Format inputFormat = {{}, inputSampleFormat, channels};
					auto framesReady = inputFormat.bytesToFrames(rBuff.size());
					auto const framesToRead = std::min(frames, framesReady);
					auto frameEndAddr = (char*)outputFormat.copyFrames(samples, rBuff.readAddr(), framesToRead, inputFormat, currentVolume);
					rBuff.commitRead(inputFormat.framesToBytes(framesToRead));
					if(framesToRead < frames) [[unlikely]]
					{
						auto padFrames = frames - framesToRead;
						std::fill_n(frameEndAddr, outputFormat.framesToBytes(padFrames), 0);
						//logMsg("underrun, %d bytes ready out of %d", bytesReady, bytes);
						auto now = SteadyClock::now();
						if(now - lastUnderrunTime < IG::Seconds(1))
						{
							//logWarn("multiple underruns within a short time");
							audioWriteState = AudioWriteState::MULTI_UNDERRUN;
						}
						else
						{
							audioWriteState = AudioWriteState::UNDERRUN;
						}
						lastUnderrunTime = now;
						#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
						audioStats.underruns++;
						#endif
					}
					return true;
				}
				else
				{
					std::fill_n((char*)samples, outputFormat.framesToBytes(frames), 0);
					return false;
				}
			}
		};
		outputConf.wantedLatencyHint = {};
		startAudioStats(inputFormat);
		audioStream.open(outputConf);
	}
	else
	{
		startAudioStats(inputFormat);
		if(shouldStartAudioWrites())
		{
			if(Config::DEBUG_BUILD)
				logMsg("resuming audio writes with buffer fill %zu/%zu bytes", rBuff.size(), rBuff.capacity());
			audioWriteState = AudioWriteState::ACTIVE;
		}
		else
		{
			audioWriteState = AudioWriteState::BUFFER;
		}
		audioStream.play();
	}
}

void EmuAudio::stop()
{
	stopAudioStats();
	audioWriteState = AudioWriteState::BUFFER;
	if(audioStream)
		audioStream.close();
	rBuff.clear();
}

void EmuAudio::close()
{
	stop();
	audioStream.reset();
	rBuff = {};
}

void EmuAudio::flush()
{
	if(!audioStream) [[unlikely]]
		return;
	stopAudioStats();
	audioWriteState = AudioWriteState::BUFFER;
	if(audioStream)
		audioStream.flush();
	rBuff.clear();
}

void EmuAudio::writeFrames(const void *samples, size_t framesToWrite)
{
	if(!framesToWrite) [[unlikely]]
		return;
	assumeExpr(rBuff);
	auto inputFormat = format();
	switch(audioWriteState)
	{
		case AudioWriteState::MULTI_UNDERRUN:
			if(speedMultiplier == 1. && addSoundBuffersOnUnderrun &&
				inputFormat.bytesToTime(rBuff.capacity()).count() <= 1.) // hard cap buffer increase to 1 sec
			{
				logWarn("increasing buffer size due to multiple underruns within a short time");
				targetBufferFillBytes += bufferIncrementBytes;
				resizeAudioBuffer(targetBufferFillBytes);
			}
			[[fallthrough]];
		case AudioWriteState::UNDERRUN:
			audioWriteState = AudioWriteState::BUFFER;
		break;
		default:
		break;
	}
	const size_t sampleFrames = framesToWrite;
	if(speedMultiplier != 1.) [[unlikely]]
	{
		framesToWrite = std::ceil((double)framesToWrite / speedMultiplier);
		framesToWrite = std::max(framesToWrite, 1zu);
	}
	auto bytes = inputFormat.framesToBytes(framesToWrite);
	auto freeBytes = rBuff.freeSpace();
	if(bytes <= freeBytes)
	{
		if(sampleFrames != framesToWrite)
		{
			simpleResample(rBuff.writeAddr(), framesToWrite, samples, sampleFrames, inputFormat);
			rBuff.commitWrite(bytes);
		}
		else
			rBuff.writeUnchecked(samples, bytes);
	}
	else
	{
		logMsg("overrun, only %zu out of %zu bytes free", freeBytes, bytes);
		#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
		audioStats.overruns++;
		#endif
		auto freeFrames = inputFormat.bytesToFrames(freeBytes);
		simpleResample(rBuff.writeAddr(), freeFrames, samples, sampleFrames, inputFormat);
		rBuff.commitWrite(freeBytes);
	}
	if(audioWriteState == AudioWriteState::BUFFER && shouldStartAudioWrites(bytes))
	{
		if(Config::DEBUG_BUILD)
		{
			auto bytes = rBuff.size();
			auto capacity = rBuff.capacity();
			logMsg("starting audio writes with buffer fill %zu/%zu bytes %.2f/%.2f secs",
				bytes, capacity, inputFormat.bytesToTime(bytes).count(), inputFormat.bytesToTime(capacity).count());
		}
		audioWriteState = AudioWriteState::ACTIVE;
	}
}

void EmuAudio::setRate(int newRate)
{
	assert(newRate <= defaultRate);
	if(!newRate)
		newRate = defaultRate;
	if(rate_ == newRate)
		return;
	if(rate_)
		logMsg("set rate:%u -> %u", rate_, newRate);
	else
		logMsg("set rate:%u", newRate);
	rate_ = newRate;
	stop();
}

void EmuAudio::setStereo(bool on)
{
	auto newChannels = on ? 2 : 1;
	if(newChannels == channels)
		return;
	channels = newChannels;
	stop();
}

void EmuAudio::setSpeedMultiplier(double speed)
{
	if(speedMultiplier == speed)
		return;
	speedMultiplier = speed;
	logMsg("set speed multiplier:%f", speed);
	updateVolume();
	updateAddBuffersOnUnderrun();

}

void EmuAudio::updateVolume()
{
	assumeExpr(maxVolume_ >= 0.f && maxVolume_ <= 1.25f);
	if(speedMultiplier != 1.)
	{
		if(isEnabledDuringAltSpeed())
			currentVolume = maxVolume_ * .5f;
		else
			currentVolume = 0;
	}
	else
	{
		currentVolume = maxVolume_;
	}
}

void EmuAudio::updateAddBuffersOnUnderrun() { addSoundBuffersOnUnderrun = speedMultiplier == 1. ? addSoundBuffersOnUnderrunSetting : false; }

constexpr bool isValidVolumeSetting(int8_t vol) { return vol >= 0 && vol <= 125; }

bool EmuAudio::setMaxVolume(int8_t vol)
{
	if(!isValidVolumeSetting(vol))
		return false;
	maxVolume_ = vol / 100.f;
	updateVolume();
	return true;
}

void EmuAudio::setOutputAPI(IG::Audio::Api api)
{
	audioAPI = api;
	open();
}

bool EmuAudio::isEnabled() const
{
	return flags.enabled;
}

void EmuAudio::setEnabled(bool on)
{
	flags.enabled = on;
	if(on)
		open();
	else
		close();
}

bool EmuAudio::isEnabledDuringAltSpeed() const
{
	return flags.enabledDuringAltSpeed;
}

void EmuAudio::setEnabledDuringAltSpeed(bool on)
{
	flags.enabledDuringAltSpeed = on;
	updateAddBuffersOnUnderrun();
}

IG::Audio::Format EmuAudio::format() const
{
	assumeExpr(rate_ > 0);
	return {rate_, EmuSystem::audioSampleFormat, channels};
}

constexpr bool isValidSoundRate(int rate)
{
	switch(rate)
	{
		case 22050:
		case 32000:
		case 44100:
		case 48000: return true;
	}
	return false;
}

void EmuAudio::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_SOUND, flags, defaultAudioFlags);
	if(!EmuSystem::forcedSoundRate)
		writeOptionValueIfNotDefault(io, CFGKEY_SOUND_RATE, rate_, defaultRate);
	writeOptionValueIfNotDefault(io, CFGKEY_SOUND_BUFFERS, soundBuffers, defaultSoundBuffers);
	writeOptionValueIfNotDefault(io, CFGKEY_SOUND_VOLUME, maxVolume(), 100);
	writeOptionValueIfNotDefault(io, CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN, addSoundBuffersOnUnderrunSetting, false);
	if(used(audioAPI))
		writeOptionValueIfNotDefault(io, CFGKEY_AUDIO_API, audioAPI, Audio::Api::DEFAULT);
}

bool EmuAudio::readConfig(MapIO &io, unsigned key, size_t size)
{
	switch(key)
	{
		case CFGKEY_SOUND: return readOptionValue(io, size, flags);
		case CFGKEY_SOUND_RATE: return EmuSystem::forcedSoundRate ? false : readOptionValue(io, size, rate_, isValidSoundRate);
		case CFGKEY_SOUND_BUFFERS: return readOptionValue(io, size, soundBuffers, optionIsValidWithMinMax<1, 7, int8_t>);
		case CFGKEY_SOUND_VOLUME: return readOptionValue<int8_t>(io, size, [&](auto v){ setMaxVolume(v); }, isValidVolumeSetting);
		case CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN: return readOptionValue(io, size, addSoundBuffersOnUnderrunSetting);
		case CFGKEY_AUDIO_API: return used(audioAPI) ? readOptionValue(io, size, audioAPI) : false;
	}
	return false;
}

}
