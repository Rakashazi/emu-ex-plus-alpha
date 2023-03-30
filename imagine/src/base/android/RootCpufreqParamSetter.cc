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

#define LOGTAG "CpufreqParam"
#include <imagine/base/android/RootCpufreqParamSetter.hh>
#include <imagine/io/PosixIO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <cstdlib>
#include <array>

#define TIMER_RATE_PATH "/sys/devices/system/cpu/cpufreq/interactive/timer_rate"
#define UP_THRESHOLD_PATH "/sys/devices/system/cpu/cpufreq/ondemand/up_threshold"
#define SAMPLING_RATE_PATH "/sys/devices/system/cpu/cpufreq/ondemand/sampling_rate"
#define SAMPLING_RATE_MiN_PATH "/sys/devices/system/cpu/cpufreq/ondemand/sampling_rate_min"

namespace IG
{

static int readIntFileValue(const char *path)
{
	try
	{
		PosixIO f{path};
		std::array<char, 32> buff{};
		f.read(buff.data(), buff.size() - 1, 0);
		int val = -1;
		sscanf(buff.data(), "%d", &val);
		return val;
	}
	catch(...)
	{
		return -1;
	}
}

RootCpufreqParamSetter::RootCpufreqParamSetter()
{
	origTimerRate = readIntFileValue(TIMER_RATE_PATH);
	if(origTimerRate <= 0)
	{
		origUpThreshold = readIntFileValue(UP_THRESHOLD_PATH);
		logMsg("default up_threshold:%d", origUpThreshold);
		origSamplingRate = readIntFileValue(SAMPLING_RATE_PATH);
		logMsg("default sampling_rate:%d", origSamplingRate);
	}
	else
	{
		logMsg("default timer_rate:%d", origTimerRate);
	}

	if(origTimerRate <= 0 && origUpThreshold <= 0 && origSamplingRate <= 0)
	{
		logErr("couldn't read any cpufreq parameters");
		return;
	}

	rootShell = popen("su", "w");
	if(!rootShell)
	{
		logErr("error running root shell");
		return;
	}
	logMsg("opened root shell");
}

RootCpufreqParamSetter::~RootCpufreqParamSetter()
{
	if(rootShell)
	{
		pclose(rootShell);
		logMsg("closed root shell");
	}
}

void RootCpufreqParamSetter::setLowLatency()
{
	assumeExpr(rootShell);
	if(origTimerRate > 0)
	{
		// interactive
		logMsg("setting low-latency interactive governor values");
		fprintf(rootShell, "echo -n 6000 > " TIMER_RATE_PATH "\n");
	}
	else
	{
		// ondemand
		logMsg("setting low-latency ondemand governor values");
		if(origUpThreshold > 0)
			fprintf(rootShell, "echo -n 40 > " UP_THRESHOLD_PATH "\n");
		if(origSamplingRate > 0)
			fprintf(rootShell, "echo -n `cat " SAMPLING_RATE_MiN_PATH "` > " SAMPLING_RATE_PATH "\n");
	}
	fflush(rootShell);
}

void RootCpufreqParamSetter::setDefaults()
{
	assumeExpr(rootShell);
	if(origTimerRate > 0)
	{
		// interactive
		logMsg("setting default interactive governor values");
		fprintf(rootShell, "echo -n %d > " TIMER_RATE_PATH "\n", origTimerRate);
	}
	else
	{
		// ondemand
		logMsg("setting default ondemand governor values");
		if(origUpThreshold > 0)
			fprintf(rootShell, "echo -n %d > " UP_THRESHOLD_PATH "\n", origUpThreshold);
		if(origSamplingRate > 0)
			fprintf(rootShell, "echo -n %d > " SAMPLING_RATE_PATH "\n", origSamplingRate);
	}
	fflush(rootShell);
}

}
