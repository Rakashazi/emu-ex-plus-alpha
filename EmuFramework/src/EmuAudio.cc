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
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuSystem.hh>
#include "EmuOptions.hh"
#include "private.hh"

struct AudioStats
{
	constexpr AudioStats() {}
	uint underruns = 0;
	uint overruns = 0;
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
static Base::Timer audioStatsTimer{"audioStatsTimer"};
#endif

static void startAudioStats()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStats.reset();
	audioStatsTimer.callbackAfterSec(
		[]()
		{
			auto frames = format.bytesToFrames(audioStats.callbackBytes);
			updateEmuAudioStats(audioStats.underruns, audioStats.overruns,
				audioStats.callbacks, frames / (double)audioStats.callbacks, frames);
			audioStats.callbacks = 0;
			audioStats.callbackBytes = 0;
		}, 1, 1, {});
	#endif
}

static void stopAudioStats()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsTimer.deinit();
	clearEmuAudioStats();
	#endif
}

uint EmuAudio::framesFree() const
{
	return format.bytesToFrames(rBuff.freeSpace());
}

uint EmuAudio::framesWritten() const
{
	return format.bytesToFrames(rBuff.size());
}

uint EmuAudio::framesCapacity() const
{
	return format.bytesToFrames(rBuff.capacity());
}

bool EmuAudio::shouldStartAudioWrites(uint32_t bytesToWrite) const
{
	// audio starts when the buffer reaches target size
	return rBuff.size() + bytesToWrite >= targetBufferFillBytes;
}

template<typename T>
static void simpleResample(T *dest, uint destFrames, const T *src, uint srcFrames)
{
	if(!destFrames)
		return;
	float ratio = (float)srcFrames/(float)destFrames;
	iterateTimes(destFrames, i)
	{
		uint srcPos = round(i * ratio);
		if(unlikely(srcPos > srcFrames))
		{
			logMsg("resample pos %u too high", srcPos);
			srcPos = srcFrames-1;
		}
		dest[i] = src[srcPos];
	}
}

static uint32_t makeWantedLatencyUSecs(uint8_t buffers)
{
	return std::round(buffers * (1000000. * EmuSystem::frameTime()));
}

void EmuAudio::resizeAudioBuffer(uint32_t buffers)
{
	auto targetBufferFillUSecs = makeWantedLatencyUSecs(buffers);
	targetBufferFillBytes = format.uSecsToBytes(targetBufferFillUSecs);
	auto oldCapacity = rBuff.capacity();
	auto bufferSizeUSecs = makeWantedLatencyUSecs(buffers + 1);
	rBuff.setMinCapacity(format.uSecsToBytes(bufferSizeUSecs));
	if(Config::DEBUG_BUILD && rBuff.capacity() != oldCapacity)
	{
		logMsg("created audio buffer:%d frames (%uus), fill target:%d frames (%uus)",
			format.bytesToFrames(rBuff.freeSpace()), bufferSizeUSecs,
			format.bytesToFrames(targetBufferFillBytes), targetBufferFillUSecs);
	}
}

void EmuAudio::start()
{
	if(!optionSound)
		return;

	lastUnderrunTime = {};
	extraSoundBuffers = 0;
	if(!audioStream)
	{
		audioStream = std::make_unique<IG::Audio::SysOutputStream>();
	}
	if(!audioStream->isOpen())
	{
		resizeAudioBuffer(optionSoundBuffers);
		audioWriteState = AudioWriteState::BUFFER;
		IG::Audio::OutputStreamConfig outputConf
		{
			format,
			[this](void *samples, uint bytes)
			{
				#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
				audioStats.callbacks++;
				audioStats.callbackBytes += bytes;
				#endif
				if(audioWriteState == AudioWriteState::ACTIVE)
				{
					auto bytesRead = rBuff.read(samples, bytes);
					if(unlikely(bytesRead < bytes))
					{
						uint padBytes = bytes - bytesRead;
						std::fill_n(&((char*)samples)[bytesRead], padBytes, 0);
						//logMsg("underrun, %d bytes ready out of %d", bytesReady, bytes);
						auto now = IG::steadyClockTimestamp();
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
					std::fill_n((char*)samples, bytes, 0);
					return false;
				}
			}
		};
		outputConf.setWantedLatencyHint(0);
		startAudioStats();
		audioStream->open(outputConf);
	}
	else
	{
		startAudioStats();
		if(shouldStartAudioWrites())
		{
			logMsg("resuming audio writes with buffer fill %u/%u bytes", rBuff.size(), rBuff.capacity());
			audioWriteState = AudioWriteState::ACTIVE;
		}
		else
		{
			audioWriteState = AudioWriteState::BUFFER;
		}
		audioStream->play();
	}
}

void EmuAudio::stop()
{
	stopAudioStats();
	audioWriteState = AudioWriteState::BUFFER;
	if(audioStream)
		audioStream->close();
	rBuff.clear();
}

void EmuAudio::close()
{
	stop();
	rBuff = {};
}

void EmuAudio::flush()
{
	stopAudioStats();
	audioWriteState = AudioWriteState::BUFFER;
	if(audioStream)
		audioStream->flush();
	rBuff.clear();
}

void EmuAudio::writeFrames(const void *samples, uint framesToWrite)
{
	assert(optionSound.val);
	switch(audioWriteState)
	{
		case AudioWriteState::MULTI_UNDERRUN:
			logWarn("multiple underruns within a short time");
			if(optionAddSoundBuffersOnUnderrun && format.bytesToSecs(rBuff.capacity()) <= 1.) // hard cap buffer increase to 1 sec
			{
				extraSoundBuffers++;
				resizeAudioBuffer(optionSoundBuffers + extraSoundBuffers);
			}
			[[fallthrough]];
		case AudioWriteState::UNDERRUN:
			audioWriteState = AudioWriteState::BUFFER;
		break;
		default:
		break;
	}
	uint bytes = format.framesToBytes(framesToWrite);
	uint freeBytes = rBuff.freeSpace();
	if(bytes <= freeBytes)
	{
		rBuff.write(samples, bytes);
	}
	else
	{
		bool logOverrun = true;
		#ifndef NDEBUG
		logOverrun = !doingFrameSkip;
		#endif
		if(logOverrun)
		{
			logMsg("overrun, only %d out of %d bytes free", freeBytes, bytes);
		}
		#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
		audioStats.overruns++;
		#endif
		auto freeFrames = format.bytesToFrames(freeBytes);
		switch(format.channels)
		{
			bcase 1: simpleResample<int16_t>((int16_t*)rBuff.writeAddr(), freeFrames, (int16_t*)samples, framesToWrite);
			bcase 2: simpleResample<int32_t>((int32_t*)rBuff.writeAddr(), freeFrames, (int32_t*)samples, framesToWrite);
			bdefault: bug_unreachable("channels == %d", format.channels);
		}
		rBuff.commitWrite(freeBytes);
	}
	if(audioWriteState == AudioWriteState::BUFFER && shouldStartAudioWrites(bytes))
	{
		if(Config::DEBUG_BUILD)
		{
			auto bytes = rBuff.size();
			auto capacity = rBuff.capacity();
			logMsg("starting audio writes with buffer fill %u/%u bytes %.2f/%.2f secs",
				bytes, capacity, format.bytesToSecs(bytes), format.bytesToSecs(capacity));
		}
		audioWriteState = AudioWriteState::ACTIVE;
	}
}

void EmuAudio::setRate(uint32_t rate)
{
	auto prevFormat = format;
	format.rate = rate;
	if(prevFormat != format)
	{
		logMsg("rate changed:%u -> %u", prevFormat.rate, rate);
		close();
	}
}

void EmuAudio::setFormat(IG::Audio::SampleFormat sample, uint8_t channels)
{
	auto prevFormat = format;
	format.sample = sample;
	format.channels = channels;
	if(prevFormat != format)
	{
		close();
	}
}

void EmuAudio::setDefaultMonoFormat()
{
	setFormat(IG::Audio::SampleFormats::s16, 1);
}

void EmuAudio::setDoingFrameSkip(bool on)
{
	#ifndef NDEBUG
	doingFrameSkip = on;
	#endif
}

bool EmuAudio::shouldRenderAudioForSkippedFrame() const
{
	return framesFree() > EmuSystem::audioFramesPerVideoFrame;
}

IG::Audio::PcmFormat EmuAudio::pcmFormat() const
{
	return format;
}
