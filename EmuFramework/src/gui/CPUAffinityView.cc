/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include "CPUAffinityView.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <cstdio>

namespace EmuEx
{

CPUAffinityView::CPUAffinityView(ViewAttachParams attach, int cpuCount):
	TableView{"Override CPU Affinity", attach, cpuAffinityItems}
{
	for(int i : iotaCount(cpuCount))
	{
		cpuAffinityItems.emplace_back([&](FILE *maxFreqFile)
			{
				if(!maxFreqFile)
					return fmt::format("{} (Offline)", i);
				int freq{};
				auto items = fscanf(maxFreqFile, "%d", &freq);
				fclose(maxFreqFile);
				return fmt::format("{} ({}MHz)", i, freq / 1000);
			}(fopen(fmt::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", i).c_str(), "r")),
			&defaultFace(), app().cpuAffinity(i),
			[this, i](BoolMenuItem &item)
			{
				app().setCPUAffinity(i, item.flipBoolValue());
			});
	}
}

}
