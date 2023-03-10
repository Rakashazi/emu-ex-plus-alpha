/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/SystemOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include "MainApp.hh"
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

static constexpr unsigned MAX_SH2_CORES = 4;

class CustomSystemOptionView : public SystemOptionView
{
	TextMenuItem bios
	{
		biosMenuEntryStr(biosPath), &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView>("BIOS",
				app().validSearchPath(FS::dirnameUri(biosPath)),
				[this](CStringView path, FS::file_type type)
				{
					biosPath = path;
					logMsg("set bios:%s", biosPath.data());
					bios.compile(biosMenuEntryStr(path), renderer());
					return true;
				}, hasBIOSExtension), e);
		}
	};

	std::string biosMenuEntryStr(std::string_view path) const
	{
		return fmt::format("BIOS: {}", appContext().fileUriDisplayName(path));
	}

	StaticArrayList<TextMenuItem, MAX_SH2_CORES> sh2CoreItem;

	MultiChoiceMenuItem sh2Core
	{
		"SH2", &defaultFace(),
		[]() -> int
		{
			for(auto i : iotaCount(std::min(SH2Cores, MAX_SH2_CORES)))
			{
				if(SH2CoreList[i]->id == yinit.sh2coretype)
					return i;
			}
			return 0;
		}(),
		sh2CoreItem
	};

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		if(SH2Cores > 1)
		{
			for(auto i : iotaCount(std::min(SH2Cores, MAX_SH2_CORES)))
			{
				int id = SH2CoreList[i]->id;
				sh2CoreItem.emplace_back(SH2CoreList[i]->Name, &defaultFace(),
					[id]()
					{
						yinit.sh2coretype = id;
						optionSH2Core = id;
					});
			}
			item.emplace_back(&sh2Core);
		}
		item.emplace_back(&bios);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		default: return nullptr;
	}
}

}
