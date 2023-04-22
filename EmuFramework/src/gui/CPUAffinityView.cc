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
	TableView{"Configure CPU Affinity", attach, menuItems},
	affinityModeItems
	{
		{"Auto (Use only performance cores or hints for low latency)", &defaultFace(), to_underlying(CPUAffinityMode::Auto)},
		{"Any (Use any core even if it increases latency)",            &defaultFace(), to_underlying(CPUAffinityMode::Any)},
		{"Manual (Use cores set in previous menu)",                    &defaultFace(), to_underlying(CPUAffinityMode::Manual)},
	},
	affinityMode
	{
		"CPU Affinity Mode", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(wise_enum::to_string(CPUAffinityMode(affinityModeItems[idx].id())));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().cpuAffinityMode = CPUAffinityMode(item.id()); }
		},
		MenuItem::Id(uint8_t(app().cpuAffinityMode)),
		affinityModeItems
	},
	cpusHeading{"Manual CPU Affinity", &defaultBoldFace()}
{
	menuItems.emplace_back(&affinityMode);
	menuItems.emplace_back(&cpusHeading);
	cpuAffinityItems.reserve(cpuCount);
	for(int i : iotaCount(cpuCount))
	{
		auto &item = cpuAffinityItems.emplace_back([&]
			{
				auto freq = appContext().maxCPUFrequencyKHz(i);
				if(!freq)
					return fmt::format("{} (Offline)", i);
				return fmt::format("{} ({}MHz)", i, freq / 1000);
			}(),
			&defaultFace(), app().cpuAffinity(i),
			[this, i](BoolMenuItem &item) { app().setCPUAffinity(i, item.flipBoolValue(*this)); });
		menuItems.emplace_back(&item);
	}

}

}
