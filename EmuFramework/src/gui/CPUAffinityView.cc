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
#include <cstdio>
#include <format>

namespace EmuEx
{

CPUAffinityView::CPUAffinityView(ViewAttachParams attach, int cpuCount):
	TableView{"Configure CPU Affinity", attach, menuItems},
	affinityModeItems
	{
		{"Auto (Use only performance cores or hints for low latency)", attach, {.id = CPUAffinityMode::Auto}},
		{"Any (Use any core even if it increases latency)",            attach, {.id = CPUAffinityMode::Any}},
		{"Manual (Use cores set in previous menu)",                    attach, {.id = CPUAffinityMode::Manual}},
	},
	affinityMode
	{
		"CPU Affinity Mode", attach,
		MenuId{app().cpuAffinityMode.value()},
		affinityModeItems,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(wise_enum::to_string(CPUAffinityMode(affinityModeItems[idx].id.val)));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().cpuAffinityMode = CPUAffinityMode(item.id.val); }
		},
	},
	cpusHeading{"Manual CPU Affinity", attach}
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
					return std::format("{} (Offline)", i);
				return std::format("{} ({}MHz)", i, freq / 1000);
			}(),
			attach, app().cpuAffinity(i),
			[this, i](BoolMenuItem &item) { app().setCPUAffinity(i, item.flipBoolValue(*this)); });
		menuItems.emplace_back(&item);
	}
}

void CPUAffinityView::onShow()
{
	bool isInManualMode = app().cpuAffinityMode == CPUAffinityMode::Manual;
	for(auto &item : cpuAffinityItems)
	{
		item.setActive(isInManualMode);
	}
}

}
