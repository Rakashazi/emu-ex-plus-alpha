/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "resources"
#include "internal.hh"

extern "C"
{
	#include "resources.h"
}

namespace EmuEx
{

int intResource(const char *name)
{
	int val{};
	auto failed = plugin.resources_get_int(name, &val);
	if(failed)
	{
		logErr("error getting int resource:%s", name);
	}
	return val;
}

void setIntResource(const char *name, int val)
{
	plugin.resources_set_int(name, val);
}

void resetIntResource(const char *name)
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return;
	}
	logMsg("setting resource:%s to default:%d", name, val);
	setIntResource(name, val);
}

const char *stringResource(const char *name)
{
	const char *val{};
	auto failed = plugin.resources_get_string(name, &val);
	if(failed)
	{
		logErr("error getting string resource:%s", name);
	}
	return val;
}

void setStringResource(const char *name, const char *val)
{
	plugin.resources_set_string(name, val);
}

void setAutostartWarp(bool on)
{
	setIntResource("AutostartWarp", on);
}

static bool autostartWarp()
{
	return intResource("AutostartWarp");
}

void setAutostartTDE(bool on)
{
	setIntResource("AutostartHandleTrueDriveEmulation", on);
}

static bool autostartTDE()
{
	return intResource("AutostartHandleTrueDriveEmulation");
}

void setAutostartBasicLoad(bool on)
{
	setIntResource("AutostartBasicLoad", on);
}

bool autostartBasicLoad()
{
	return intResource("AutostartBasicLoad");
}

void setBorderMode(int mode)
{
	if(!plugin.borderModeStr)
		return;
	setIntResource(plugin.borderModeStr, mode);
}

static int borderMode()
{
	if(!plugin.borderModeStr)
		return -1;
	return intResource(plugin.borderModeStr);
}

void setSidEngine(int engine)
{
	logMsg("set SID engine %d", engine);
	setIntResource("SidEngine", engine);
}

static int sidEngine()
{
	return intResource("SidEngine");
}

void setReSidSampling(int sampling)
{
	logMsg("set ReSID sampling %d", sampling);
	setIntResource("SidResidSampling", sampling);
}

static int reSidSampling()
{
	return intResource("SidResidSampling");
}

static void setVirtualDeviceTraps(bool on)
{
	setIntResource("VirtualDevice8", on);
	setIntResource("VirtualDevice9", on);
	setIntResource("VirtualDevice10", on);
	setIntResource("VirtualDevice11", on);
}

static bool virtualDeviceTraps()
{
	return intResource("VirtualDevice8");
}

void setDriveTrueEmulation(bool on)
{
	setIntResource("Drive8TrueEmulation", on);
	setIntResource("Drive9TrueEmulation", on);
	setIntResource("Drive10TrueEmulation", on);
	setIntResource("Drive11TrueEmulation", on);
	setVirtualDeviceTraps(!on);
}

bool driveTrueEmulation()
{
	return intResource("Drive8TrueEmulation");
}

}
