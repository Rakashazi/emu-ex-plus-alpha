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
#include "MainSystem.hh"

extern "C"
{
	#include "resources.h"
}

namespace EmuEx
{

int C64System::intResource(const char *name) const
{
	int val{};
	auto failed = plugin.resources_get_int(name, &val);
	if(failed)
	{
		logErr("error getting int resource:%s", name);
	}
	return val;
}

void C64System::setIntResource(const char *name, int val)
{
	plugin.resources_set_int(name, val);
}

void C64System::resetIntResource(const char *name)
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return;
	}
	logMsg("setting resource:%s to default:%d", name, val);
	setIntResource(name, val);
}

int C64System::defaultIntResource(const char *name) const
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return 0;
	}
	return val;
}

const char *C64System::stringResource(const char *name) const
{
	const char *val{};
	auto failed = plugin.resources_get_string(name, &val);
	if(failed)
	{
		logErr("error getting string resource:%s", name);
	}
	return val;
}

void C64System::setStringResource(const char *name, const char *val)
{
	plugin.resources_set_string(name, val);
}

void C64System::setAutostartWarp(bool on)
{
	setIntResource("AutostartWarp", on);
}

bool C64System::autostartWarp() const
{
	return intResource("AutostartWarp");
}

void C64System::setAutostartTDE(bool on)
{
	setIntResource("AutostartHandleTrueDriveEmulation", on);
}

bool C64System::autostartTDE() const
{
	return intResource("AutostartHandleTrueDriveEmulation");
}

void C64System::setAutostartBasicLoad(bool on)
{
	setIntResource("AutostartBasicLoad", on);
}

bool C64System::autostartBasicLoad() const
{
	return intResource("AutostartBasicLoad");
}

void C64System::setBorderMode(int mode)
{
	if(!plugin.borderModeStr)
		return;
	setIntResource(plugin.borderModeStr, mode);
}

int C64System::borderMode() const
{
	if(!plugin.borderModeStr)
		return -1;
	return intResource(plugin.borderModeStr);
}

void C64System::setSidEngine(int engine)
{
	logMsg("set SID engine %d", engine);
	setIntResource("SidEngine", engine);
}

int C64System::sidEngine() const
{
	return intResource("SidEngine");
}

void C64System::setReSidSampling(int sampling)
{
	logMsg("set ReSID sampling %d", sampling);
	setIntResource("SidResidSampling", sampling);
}

int C64System::reSidSampling() const
{
	return intResource("SidResidSampling");
}

void C64System::setVirtualDeviceTraps(bool on)
{
	setIntResource("VirtualDevice8", on);
	setIntResource("VirtualDevice9", on);
	setIntResource("VirtualDevice10", on);
	setIntResource("VirtualDevice11", on);
}

bool C64System::virtualDeviceTraps() const
{
	return intResource("VirtualDevice8");
}

void C64System::setDriveTrueEmulation(bool on)
{
	setIntResource("Drive8TrueEmulation", on);
	setIntResource("Drive9TrueEmulation", on);
	setIntResource("Drive10TrueEmulation", on);
	setIntResource("Drive11TrueEmulation", on);
	setVirtualDeviceTraps(!on);
}

bool C64System::driveTrueEmulation() const
{
	return intResource("Drive8TrueEmulation");
}

}
