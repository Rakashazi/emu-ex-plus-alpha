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
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/memory/DynArray.hh>
#include <vector>
#include <array>
#include <string>

namespace EmuEx
{

using namespace IG;
class EmuInputView;

class IdentInputDeviceView : public View
{
public:
	using OnIdentInputDelegate = DelegateFunc<void (const Input::KeyEvent&)>;
	OnIdentInputDelegate onIdentInput;

	IdentInputDeviceView(ViewAttachParams);
	void place() final;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) final;
	void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const final;

private:
	Gfx::Text text;
	Gfx::IQuads quads;
};

class InputManagerView final: public TableView, public EmuAppHelper
{
public:
	InputManagerView(ViewAttachParams, InputManager&);
	~InputManagerView() final;
	void onShow() final;
	void pushAndShowDeviceView(const Input::Device &, const Input::Event &);

private:
	InputManager& inputManager;
	TextMenuItem deleteDeviceConfig;
	TextMenuItem deleteProfile;
	ConditionalMember<Config::envIsAndroid, TextMenuItem> rescanOSDevices;
	TextMenuItem identDevice;
	TextMenuItem generalOptions;
	TextHeadingMenuItem deviceListHeading;
	std::vector<TextMenuItem> inputDevName;
	std::vector<MenuItem*> item;

	void loadItems();
};

class InputManagerOptionsView : public TableView, public EmuAppHelper
{
public:
	InputManagerOptionsView(ViewAttachParams, EmuInputView&);

private:
	ConditionalMember<MOGA_INPUT, BoolMenuItem> mogaInputSystem;
	ConditionalMember<Config::Input::DEVICE_HOTSWAP, BoolMenuItem> notifyDeviceChange;
	ConditionalMember<Config::Input::BLUETOOTH, TextHeadingMenuItem> bluetoothHeading;
	ConditionalMember<Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, BoolMenuItem> keepBtActive;
	ConditionalMember<Config::Bluetooth::scanTime, TextMenuItem> btScanSecsItem[5];
	ConditionalMember<Config::Bluetooth::scanTime, MultiChoiceMenuItem> btScanSecs;
	ConditionalMember<Config::Bluetooth::scanCache, BoolMenuItem> btScanCache;
	BoolMenuItem altGamepadConfirm;
	StaticArrayList<MenuItem*, 10> item;
	EmuInputView& emuInputView;
};

class InputManagerDeviceView : public TableView, public EmuAppHelper
{
public:
	InputManagerDeviceView(UTF16String name, ViewAttachParams,
		InputManagerView& rootIMView, const Input::Device&, InputManager&);
	void onShow() final;

private:
	InputManager& inputManager;
	InputManagerView& rootIMView;
	DynArray<TextMenuItem> playerItems;
	MultiChoiceMenuItem player;
	TextMenuItem loadProfile;
	TextMenuItem renameProfile;
	TextMenuItem newProfile;
	TextMenuItem deleteProfile;
	ConditionalMember<hasICadeInput, BoolMenuItem> iCadeMode;
	ConditionalMember<Config::envIsAndroid, BoolMenuItem> consumeUnboundKeys;
	BoolMenuItem joystickAxisStick1Keys;
	BoolMenuItem joystickAxisStick2Keys;
	BoolMenuItem joystickAxisHatKeys;
	BoolMenuItem joystickAxisTriggerKeys;
	BoolMenuItem joystickAxisPedalKeys;
	//TextMenuItem disconnect {"Disconnect"}; // TODO
	TextHeadingMenuItem categories;
	TextHeadingMenuItem options;
	TextHeadingMenuItem joystickSetup;
	std::vector<TextMenuItem> inputCategory;
	std::vector<MenuItem*> item;
	InputDeviceConfig& devConf;

	void confirmICadeMode();
	void loadItems();
	void addCategoryItem(const KeyCategory&);
};

}
