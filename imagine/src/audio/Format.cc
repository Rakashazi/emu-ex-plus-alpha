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

#include <imagine/audio/Format.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/math.hh>
#include <cmath>

namespace IG::Audio
{

static int16_t clamp16FromFloat(float x)
{
	assumeExpr(x >= -1.f && x <= 1.f);
	return remap(x, -1.f, 1.f, std::numeric_limits<int16_t>{});
}

static float *convertI16SamplesToFloat(float * __restrict__ dest, size_t samples, const int16_t * __restrict__ src, float volume)
{
	return transformN(src, samples, dest,
		[=](int16_t s)
		{
			return ((float)s / 32768.f) * volume;
		});
}

static int16_t *convertFloatSamplesToI16(int16_t * __restrict__ dest, size_t samples, const float * __restrict__ src, float volume)
{
	return transformN(src, samples, dest,
		[=](float s)
		{
			return clamp16FromFloat(s * volume);
		});
}

static int16_t *copyI16Samples(int16_t * __restrict__ dest, size_t samples, const int16_t * __restrict__ src, float volume)
{
	if(volume == 1.f)
	{
		return copy_n(src, samples, dest);
	}
	else
	{
		return transformN(src, samples, dest,
			[=](int16_t s)
			{
				return clamp16FromFloat(((float)s / 32768.f) * volume);
			});
	}
}

static float *copyFloatSamples(float * __restrict__ dest, size_t samples, const float * __restrict__ src, float volume)
{
	if(volume == 1.f)
	{
		return copy_n(src, samples, dest);
	}
	else
	{
		return transformN(src, samples, dest,
			[=](float s)
			{
				return s * volume;
			});
	}
}

void *Format::copyFrames(void * __restrict__ dest, const void * __restrict__ src, size_t frames, Format srcFormat, float volume) const
{
	assumeExpr(channels == srcFormat.channels);
	auto samples = frames * channels;
	switch(sample.bytes())
	{
		case 2:
		{
			if(srcFormat.sample.bytes() == 2)
			{
				return copyI16Samples((int16_t*)dest, samples, (int16_t*)src, volume);
			}
			else if(srcFormat.sample.isFloat())
			{
				return convertFloatSamplesToI16((int16_t*)dest, samples, (float*)src, volume);
			}
			else
			{
				bug_unreachable("unimplemented conversion");
			}
		}
		case 4:
		{
			if(srcFormat.sample.isFloat())
			{
				return copyFloatSamples((float*)dest, samples, (float*)src, volume);
			}
			else if(srcFormat.sample.bytes() == 2)
			{
				return convertI16SamplesToFloat((float*)dest, samples, (int16_t*)src, volume);
			}
			else
			{
				bug_unreachable("unimplemented conversion");
			}
		}
		default:
		{
			bug_unreachable("unimplemented conversion");
		}
	}
}

}
