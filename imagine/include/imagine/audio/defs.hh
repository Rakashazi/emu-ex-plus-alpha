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
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/audio/PcmFormat.hh>
#include <imagine/audio/AudioManager.hh>
#include <system_error>

namespace Audio
{

using OnSamplesNeededDelegate = DelegateFunc<bool(void *buff, uint bytes)>;

class OutputStreamConfig
{
public:
	constexpr OutputStreamConfig() {}
	constexpr OutputStreamConfig(PcmFormat format, OnSamplesNeededDelegate onSamplesNeeded):
		format_{format}, onSamplesNeeded_{onSamplesNeeded}
		{}

	PcmFormat format() const;

	OnSamplesNeededDelegate onSamplesNeeded() const
	{
		return onSamplesNeeded_;
	}

	void setLowLatencyHint(bool on)
	{
		lowLatency = on;
	}

	bool lowLatencyHint() const
	{
		return lowLatency;
	}

protected:
	PcmFormat format_{};
	OnSamplesNeededDelegate onSamplesNeeded_{};
	bool lowLatency = false;
};

class OutputStream
{
public:
	virtual ~OutputStream();
	virtual std::error_code open(OutputStreamConfig config) = 0;
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void close() = 0;
	virtual void flush() = 0;
	virtual bool isOpen() = 0;
	virtual bool isPlaying() = 0;
};

}
