#pragma once

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

#include <imagine/audio/OutputStream.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/ringbuffer/RingBuffer.hh>
#include <memory>
#include <atomic>

class EmuAudio
{
public:
	enum class AudioWriteState : uint8_t
	{
		BUFFER,
		ACTIVE,
		UNDERRUN,
		MULTI_UNDERRUN
	};

	constexpr EmuAudio() {}
	void open(IG::Audio::Api api);
	void start(IG::Microseconds targetBufferFillUSecs, IG::Microseconds bufferIncrementUSecs);
	void stop();
	void close();
	void flush();
	void writeFrames(const void *samples, uint32_t framesToWrite);
	void setRate(uint32_t rate);
	void setStereo(bool on);
	void setSpeedMultiplier(uint8_t speed);
	void setAddSoundBuffersOnUnderrun(bool on);
	void setVolume(uint8_t vol);
	IG::Audio::Format format() const;
	explicit operator bool() const;

protected:
	std::unique_ptr<IG::Audio::OutputStream> audioStream{};
	IG::RingBuffer rBuff{};
	IG::Time lastUnderrunTime{};
	uint32_t targetBufferFillBytes = 0;
	uint32_t bufferIncrementBytes = 0;
	uint32_t rate{44100};
	float volume = 1.0;
	std::atomic<AudioWriteState> audioWriteState = AudioWriteState::BUFFER;
	bool addSoundBuffersOnUnderrun = false;
	uint8_t speedMultiplier = 1;
	uint8_t channels = 2;

	uint32_t framesFree() const;
	uint32_t framesWritten() const;
	uint32_t framesCapacity() const;
	bool shouldStartAudioWrites(uint32_t bytesToWrite = 0) const;
	void resizeAudioBuffer(uint32_t targetBufferFillBytes);
};
