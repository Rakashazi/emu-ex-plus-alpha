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

#include <imagine/time/Time.hh>
#include "SampleFormat.hh"
#include <compare>
#include <cmath>

namespace IG::Audio
{

class Format
{
public:
	uint32_t rate = 0;
	SampleFormat sample{};
	uint8_t channels = 0;

	constexpr Format() {}
	constexpr Format(uint32_t rate, SampleFormat sample, uint8_t channels) :
		rate{rate}, sample{sample}, channels{channels} {}
	constexpr bool operator ==(Format const& rhs) const = default;

	constexpr explicit operator bool() const
	{
		return rate != 0 && sample && channels != 0;
	}

	constexpr uint32_t bytesPerFrame() const
	{
		return sample.bytes() * channels;
	}

	constexpr uint32_t framesToBytes(uint32_t frames) const
	{
		return frames * bytesPerFrame();
	}

	constexpr uint32_t bytesToFrames(uint32_t bytes) const
	{
		return bytes / bytesPerFrame();
	}

	template<class T = IG::FloatSeconds>
	constexpr T framesToTime(uint32_t frames) const
	{
		return T{IG::FloatSeconds{(double)frames / rate}};
	}

	template<class T = IG::FloatSeconds>
	constexpr T bytesToTime(uint32_t bytes) const
	{
		return framesToTime(bytesToFrames(bytes));
	}

	constexpr uint32_t timeToFrames(IG::ChronoDuration auto time) const
	{
		return std::ceil(std::chrono::duration_cast<IG::FloatSeconds>(time).count() * rate);
	}

	constexpr uint32_t timeToBytes(IG::ChronoDuration auto time) const
	{
		return framesToBytes(timeToFrames(time));
	}

	void *copyFrames(void *dest, const void *src, unsigned frames, Format srcFormat, float volume = 1.f) const;
};

}
