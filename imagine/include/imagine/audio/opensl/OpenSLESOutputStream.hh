#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/audio/OutputStream.hh>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <memory>

namespace IG::Audio
{

class Manager;

class OpenSLESOutputStream : public OutputStream
{
public:
	OpenSLESOutputStream(const Manager &);
	~OpenSLESOutputStream();
	IG::ErrorCode open(OutputStreamConfig config) final;
	void play() final;
	void pause() final;
	void close() final;
	void flush() final;
	bool isOpen() final;
	bool isPlaying() final;
	explicit operator bool() const;

private:
	SLObjectItf slE{};
	SLObjectItf outMix{};
	SLObjectItf player{};
	SLPlayItf playerI{};
	SLAndroidSimpleBufferQueueItf slBuffQI{};
	OnSamplesNeededDelegate onSamplesNeeded{};
	std::unique_ptr<uint8_t[]> buffer{};
	uint32_t bufferFrames = 0;
	uint32_t bufferBytes = 0;
	bool isPlaying_{};
	bool bufferQueued{};
	bool supportsFloatFormat{};
	uint8_t outputBuffers{};

	void doBufferCallback(SLAndroidSimpleBufferQueueItf queue);
};

}
