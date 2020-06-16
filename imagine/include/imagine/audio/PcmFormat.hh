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

#include <cmath>
#include <imagine/time/Time.hh>
#include <imagine/util/operators.hh>
#include "SampleFormat.hh"

namespace IG::Audio
{

class PcmFormat : public NotEquals<PcmFormat>
{
public:
	uint32_t rate = 0;
	SampleFormat sample{};
	uint8_t channels = 0;

	constexpr PcmFormat() {}
	constexpr PcmFormat(uint32_t rate, SampleFormat sample, uint8_t channels) :
		rate{rate}, sample{sample}, channels{channels} {}

	bool canSupport(const PcmFormat &p2) const
	{
		return rate >= p2.rate &&
			sample.bytes() >= p2.sample.bytes() &&
			channels >= p2.channels;
	}

	bool operator ==(PcmFormat const& rhs) const
	{
		return rate == rhs.rate && sample == rhs.sample && channels == rhs.channels;
	}

	explicit operator bool() const
	{
		return rate != 0 && sample && channels != 0;
	}

	uint32_t bytesPerFrame() const
	{
		return sample.bytes() * channels;
	}

	uint32_t framesToBytes(uint32_t frames) const
	{
		return frames * bytesPerFrame();
	}

	uint32_t bytesToFrames(uint32_t bytes) const
	{
		return bytes / bytesPerFrame();
	}

	template<class T = IG::FloatSeconds>
	T framesToTime(uint32_t frames) const
	{
		return T{IG::FloatSeconds{(double)frames / rate}};
	}

	template<class T = IG::FloatSeconds>
	T bytesToTime(uint32_t bytes) const
	{
		return framesToTime(bytesToFrames(bytes));
	}

	template<class T>
	uint32_t timeToFrames(T time) const
	{
		return std::ceil(std::chrono::duration_cast<IG::FloatSeconds>(time).count() * rate);
	}

	template<class T>
	uint32_t timeToBytes(T time) const
	{
		return framesToBytes(timeToFrames(time));
	}
};

}
