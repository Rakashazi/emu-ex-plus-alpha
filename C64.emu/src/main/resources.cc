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

#include "MainSystem.hh"

extern "C"
{
	#include "resources.h"
	#include "drive.h"
}

#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"C64.emu"};

int C64System::intResource(const char *name) const
{
	int val{};
	auto failed = plugin.resources_get_int(name, &val);
	if(failed)
	{
		log.error("error getting int resource:{}", name);
	}
	return val;
}

void C64System::setIntResource(const char *name, int val)
{
	plugin.resources_set_int(name, val);
}

bool C64System::updateIntResourceInCPUTrap(const char *name, int val)
{
	if(intResource(name) == val)
		return false;
	log.info("updating resource:{} to:{}", name, val);
	enterCPUTrap();
	setIntResource(name, val);
	return true;
}

void C64System::resetIntResource(const char *name)
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return;
	}
	log.info("setting resource:{} to default:{}", name, val);
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
		log.error("error getting string resource:{}", name);
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
	log.info("set SID engine:{}", engine);
	setIntResource("SidEngine", engine);
}

int C64System::sidEngine() const
{
	return intResource("SidEngine");
}

void C64System::setReSidSampling(int sampling)
{
	log.info("set ReSID sampling:{}", sampling);
	setIntResource("SidResidSampling", sampling);
}

int C64System::reSidSampling() const
{
	return intResource("SidResidSampling");
}

void C64System::setVirtualDeviceTraps(bool on)
{
	assert(inCPUTrap);
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
	log.info("set TDE:{}", on);
	enterCPUTrap();
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

constexpr const char *driveTypeName[4]
{
	"Drive8Type",
	"Drive9Type",
	"Drive10Type",
	"Drive11Type",
};

void C64System::setDriveType(int idx, int type)
{
	auto name = driveTypeName[idx - 8];
	updateIntResourceInCPUTrap(name, type);
}

int C64System::driveType(int idx)
{
	auto name = driveTypeName[idx - 8];
	return intResource(name);
}

int C64System::colorSetting(ColorSetting s) const
{
	return intResource(colorSettingResStr[size_t(s)].data());
}

std::string C64System::colorSettingAsString(ColorSetting s) const
{
	auto v = colorSetting(s);
	return std::format("{}%", v / 10);
}

void C64System::setColorSetting(ColorSetting s, int v)
{
	setIntResource(colorSettingResStr[size_t(s)].data(), v);
}

void C64System::setReuSize(int size)
{
	if(!currSystemIsC64Or128())
		return;
	enterCPUTrap();
	if(size)
	{
		log.info("enabling REU size:{}", size);
		setIntResource("REUsize", size);
		setIntResource("REU", 1);
	}
	else
	{
		log.info("disabling REU");
		setIntResource("REU", 0);
	}
}

}
