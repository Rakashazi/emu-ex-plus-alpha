#pragma once

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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/input/inputDefs.hh>
#include <string_view>

namespace EmuEx
{

using namespace IG;

struct ProfileSelectViewDesc
{
	bool hasDefaultItem{};
};

class ProfileSelectView : public TextTableView
{
public:
	using ProfileChangeDelegate = DelegateFunc<void (std::string_view profile)>;

	ProfileChangeDelegate onProfileChange;

	ProfileSelectView(ViewAttachParams attach, Input::Map map, std::string_view selectedName,
		const EmuApp& app, ProfileSelectViewDesc desc = {}):
		TextTableView
		{
			"Key Profile",
			attach,
			app.inputManager.customKeyConfigs.size() + 8 // reserve space for built-in configs
		}
	{
		if(desc.hasDefaultItem)
		{
			activeItem = 0;
			textItem.emplace_back("Default", attach, [this]()
			{
				auto del = onProfileChange;
				dismiss();
				del("");
			});
		}
		app.forEachKeyConfig(map, [&](const auto& conf)
		{
			if(selectedName == conf.name)
			{
				activeItem = textItem.size();
			}
			textItem.emplace_back(conf.name, attach, [this, &name = conf.name]()
			{
				auto del = onProfileChange;
				dismiss();
				del(name);
			});
		});
	}
};

}
