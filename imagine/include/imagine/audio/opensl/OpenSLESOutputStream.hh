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
#include <imagine/audio/defs.hh>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

namespace IG::Audio
{

class OpenSLESOutputStream : public OutputStream
{
public:
	OpenSLESOutputStream();
	std::error_code open(OutputStreamConfig config) final;
	void play() final;
	void pause() final;
	void close() final;
	void flush() final;
	bool isOpen() final;
	bool isPlaying() final;
	explicit operator bool() const;

private:
	SLEngineItf slI{};
	SLObjectItf outMix{}, player{};
	SLPlayItf playerI{};
	SLAndroidSimpleBufferQueueItf slBuffQI{};
	OnSamplesNeededDelegate onSamplesNeeded{};
	char *buffer{};
	uint32_t bufferBytes = 0;
	PcmFormat pcmFormat{};
	bool isPlaying_ = false;
	bool bufferQueued = false;

	void doBufferCallback(SLAndroidSimpleBufferQueueItf queue);
};

using SysOutputStream = OpenSLESOutputStream;

}
