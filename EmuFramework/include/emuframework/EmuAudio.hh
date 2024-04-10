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
#include <imagine/audio/Manager.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/container/RingBuffer.hh>
#include <imagine/util/used.hh>
#include <memory>
#include <atomic>

namespace IG
{
class MapIO;
class FileIO;
}

namespace EmuEx
{

using namespace IG;

struct AudioFlags
{
	uint8_t
	enabled{},
	enabledDuringAltSpeed{};

	constexpr bool operator ==(AudioFlags const &) const = default;
};

constexpr AudioFlags defaultAudioFlags{.enabled = 1, .enabledDuringAltSpeed = 1};

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

	EmuAudio(IG::ApplicationContext);
	void open();
	void start(FloatSeconds bufferDuration);
	void stop();
	void close();
	void flush();
	void writeFrames(const void *samples, size_t framesToWrite);
	void setRate(int rate);
	int rate() const { return rate_; }
	int maxRate() const { return defaultRate; }
	void setStereo(bool on);
	void setSpeedMultiplier(double speed);
	float volume() const { return currentVolume; }
	bool setMaxVolume(int8_t vol);
	int8_t maxVolume() const { return std::round(maxVolume_ * 100.f); }
	void setOutputAPI(IG::Audio::Api);
	IG::Audio::Api outputAPI() const { return audioAPI; }
	void setEnabled(bool on);
	bool isEnabled() const;
	void setEnabledDuringAltSpeed(bool on);
	bool isEnabledDuringAltSpeed() const;
	IG::Audio::Format format() const;
	explicit operator bool() const { return bool(rBuff.capacity()); }
	void writeConfig(FileIO &) const;
	bool readConfig(MapIO &, unsigned key);

	IG::Audio::Manager manager;
protected:
	IG::Audio::OutputStream audioStream;
	RingBuffer<uint8_t, RingBufferConf{.mirrored = true}> rBuff;
	SteadyClockTimePoint lastUnderrunTime{};
	double speedMultiplier{1.};
	size_t targetBufferFillBytes{};
	size_t bufferIncrementBytes{};
	int defaultRate;
	int rate_;
	float maxVolume_{1.};
	float currentVolume{1.};
	std::atomic<AudioWriteState> audioWriteState{AudioWriteState::BUFFER};
	int8_t channels{2};
	AudioFlags flags{defaultAudioFlags};
	ConditionalMember<IG::Audio::Config::MULTIPLE_SYSTEM_APIS, IG::Audio::Api> audioAPI{};
	bool addSoundBuffersOnUnderrun{};
public:
	bool addSoundBuffersOnUnderrunSetting{};
	int8_t defaultSoundBuffers{3};
	int8_t soundBuffers{defaultSoundBuffers};

	size_t framesFree() const;
	size_t framesWritten() const;
	size_t framesCapacity() const;
	bool shouldStartAudioWrites(size_t bytesToWrite = 0) const;
	void resizeAudioBuffer(size_t targetBufferFillBytes);
	void updateVolume();
	void updateAddBuffersOnUnderrun();
};

}
