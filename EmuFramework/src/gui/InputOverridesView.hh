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

#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/memory/DynArray.hh>
#include <vector>

namespace EmuEx
{

using namespace IG;

class InputOverridesView final: public TableView, public EmuAppHelper
{
public:
	InputOverridesView(ViewAttachParams attach, InputManager&);
	~InputOverridesView() final;
	void pushAndShowDeviceView(const Input::Device&, const Input::Event&);

private:
	InputManager& inputManager;
	TextMenuItem deleteDeviceConfig;
	TextHeadingMenuItem deviceListHeading;
	std::vector<TextMenuItem> inputDevNames;
	std::vector<MenuItem*> items;

	void loadItems();
};

class InputOverridesDeviceView : public TableView, public EmuAppHelper
{
public:
	InputOverridesDeviceView(UTF16String name, ViewAttachParams,
		InputOverridesView& rootIMView, const Input::Device&, InputManager&);
	void onShow() final;

private:
	InputManager& inputManager;
	InputOverridesView& rootIMView;
	DynArray<TextMenuItem> playerItems;
	MultiChoiceMenuItem player;
	TextMenuItem loadProfile;
	std::vector<MenuItem*> items;
	InputDeviceConfig& devConf;

	void loadItems();
};

}
