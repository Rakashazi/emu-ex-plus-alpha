/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuInput.hh>
#include "MainApp.hh"
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad", &defaultFace(),
		(bool)system().option6BtnPad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().option6BtnPad = item.flipBoolValue(*this);
			set6ButtonPadEnabled(app(), system().option6BtnPad);
		}
	};

	BoolMenuItem arcadeCard
	{
		"Arcade Card", &defaultFace(),
		(bool)system().optionArcadeCard,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionArcadeCard = item.flipBoolValue(*this);
			app().promptSystemReloadDueToSetOption(attachParams(), e);
		}
	};

	std::array<MenuItem*, 2> menuItem
	{
		&sixButtonPad,
		&arcadeCard
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShow(makeView<ConsoleOptionView>(), e);
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper<CustomFilePathOptionView>
{
	using MainAppHelper<CustomFilePathOptionView>::system;

	TextMenuItem sysCardPath
	{
		biosMenuEntryStr(appContext().fileUriDisplayName(system().sysCardPath)), &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto biosSelectMenu = makeViewWithName<BiosSelectMenu>("System Card", &system().sysCardPath,
				[this](std::string_view displayName)
				{
					logMsg("set bios %s", system().sysCardPath.data());
					sysCardPath.compile(biosMenuEntryStr(displayName), renderer(), projP);
				},
				hasHuCardExtension);
			pushAndShow(std::move(biosSelectMenu), e);
		}
	};

	std::string biosMenuEntryStr(std::string_view displayName) const
	{
		return fmt::format("System Card: {}", displayName);
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&sysCardPath);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}
