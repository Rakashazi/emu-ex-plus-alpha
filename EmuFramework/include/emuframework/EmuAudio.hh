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
#include <imagine/vmem/RingBuffer.hh>
#include <memory>
#include <atomic>

namespace IG::Audio
{
class Manager;
}

namespace EmuEx
{

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

	constexpr EmuAudio(const IG::Audio::Manager &audioManager):
		audioManagerPtr{&audioManager} {}
	void open(IG::Audio::Api);
	void start(IG::Microseconds targetBufferFillUSecs, IG::Microseconds bufferIncrementUSecs);
	void stop();
	void close();
	void flush();
	void writeFrames(const void *samples, size_t framesToWrite);
	void setRate(int rate);
	void setStereo(bool on);
	void setSpeedMultiplier(int8_t speed);
	void setAddSoundBuffersOnUnderrun(bool on);
	void setVolume(int8_t vol);
	IG::Audio::Format format() const;
	explicit operator bool() const;

protected:
	IG::Audio::OutputStream audioStream{};
	const IG::Audio::Manager *audioManagerPtr{};
	IG::RingBuffer rBuff{};
	IG::Time lastUnderrunTime{};
	size_t targetBufferFillBytes{};
	size_t bufferIncrementBytes{};
	int rate{};
	float volume = 1.0;
	std::atomic<AudioWriteState> audioWriteState = AudioWriteState::BUFFER;
	bool addSoundBuffersOnUnderrun = false;
	int8_t speedMultiplier = 1;
	int8_t channels = 2;

	size_t framesFree() const;
	size_t framesWritten() const;
	size_t framesCapacity() const;
	bool shouldStartAudioWrites(size_t bytesToWrite = 0) const;
	void resizeAudioBuffer(size_t targetBufferFillBytes);
	const IG::Audio::Manager &audioManager() const;
};

}
