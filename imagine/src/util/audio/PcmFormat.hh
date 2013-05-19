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

#include <util/operators.hh>
#include "SampleFormat.hh"

namespace Audio
{

class PcmFormat : public NotEquals<PcmFormat>
{
public:
	constexpr PcmFormat() { }
	constexpr PcmFormat(int rate, const SampleFormat *sample, int channels) :
		rate(rate), sample(sample), channels(channels) { }
	int rate = 0;
	const SampleFormat *sample = nullptr;
	int channels = 0;

	bool canSupport(PcmFormat *p2) const
	{
		return rate >= p2->rate &&
			sample->toBits() >= p2->sample->toBits() &&
			channels >= p2->channels;
	}

	bool operator ==(PcmFormat const& rhs) const
	{
		return rate == rhs.rate && sample == rhs.sample && channels == rhs.channels;
	}

	uint framesToBytes(uint frames) const
	{
		return frames * sample->toBytes() * channels;
	}

	float framesToMSecs(uint frames) const
	{
		return ((float)frames / rate) * 1000.;
	}

	float framesToUSecs(uint frames) const
	{
		return ((float)frames / rate) * 1000000.;
	}

	uint mSecsToFrames(float mSecs) const
	{
		return (mSecs / 1000.) * rate;
	}

	uint bytesToFrames(uint bytes) const
	{
		return bytes / sample->toBytes() / channels;
	}

	uint secsToBytes(uint secs) const
	{
		return (rate * sample->toBytes() * channels) * secs;
	}
};

}
