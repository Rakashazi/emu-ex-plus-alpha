/*  This file is part of Lynx.emu.

	Lynx.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Lynx.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Lynx.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/AudioOptionView.hh>
#include "MainApp.hh"
#include <imagine/util/string.h>
#include <imagine/util/format.hh>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper<CustomFilePathOptionView>
{
	using MainAppHelper<CustomFilePathOptionView>::app;
	using MainAppHelper<CustomFilePathOptionView>::system;

	static bool hasBiosExtension(std::string_view name)
	{
		return endsWithAnyCaseless(name, ".img");
	}

	TextMenuItem biosPath
	{
		biosMenuEntryStr(system().biosPath), &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView<ArchivePathSelectMode::exclude>>("BIOS",
				app().validSearchPath(FS::dirnameUri(system().biosPath)),
				[this](CStringView path, FS::file_type type)
				{
					system().biosPath = path;
					logMsg("set BIOS:%s", system().biosPath.data());
					biosPath.compile(biosMenuEntryStr(path), renderer());
					return true;
				}, hasBiosExtension), e);
		}
	};

	std::string biosMenuEntryStr(std::string_view path) const
	{
		return std::format("BIOS: {}", appContext().fileUriDisplayName(path));
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&biosPath);
	}
};
class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	TextMenuItem::SelectDelegate setRotationDel()
	{
		return [this](TextMenuItem &item) { system().setRotation((LynxRotation)item.id()); };
	}

	TextMenuItem rotationItem[4]
	{
		{"Auto",           &defaultFace(), setRotationDel(), to_underlying(LynxRotation::Auto)},
		{"Horizontal",     &defaultFace(), setRotationDel(), to_underlying(LynxRotation::Horizontal)},
		{"Vertical Left",  &defaultFace(), setRotationDel(), to_underlying(LynxRotation::VerticalLeft)},
		{"Vertical Right", &defaultFace(), setRotationDel(), to_underlying(LynxRotation::VerticalRight)},
	};

	MultiChoiceMenuItem rotation
	{
		"Handheld Rotation", &defaultFace(),
		MenuItem::Id(system().rotation),
		rotationItem
	};

	std::array<MenuItem*, 1> menuItem
	{
		&rotation,
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		} {}
};

class CustomSystemActionsView : public SystemActionsView
{
private:
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](Input::Event e) { pushAndShow(makeView<ConsoleOptionView>(), e); }
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): SystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;
	using MainAppHelper<CustomAudioOptionView>::app;

	BoolMenuItem lowpassFilter
	{
		"Low-pass Filter", &defaultFace(),
		system().lowpassFilter,
		[this](BoolMenuItem &item) { system().setLowpassFilter(item.flipBoolValue(*this)); }
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&lowpassFilter);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		default: return nullptr;
	}
}

}
