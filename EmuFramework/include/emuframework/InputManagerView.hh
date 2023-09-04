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
#include <imagine/input/config.hh>
#include <imagine/util/container/ArrayList.hh>
#include <vector>
#include <array>
#include <string>

namespace EmuEx
{

using namespace IG;
class InputDeviceConfig;
class EmuInputView;
struct KeyCategory;

class IdentInputDeviceView : public View
{
public:
	using OnIdentInputDelegate = DelegateFunc<void (const Input::KeyEvent &)>;
	OnIdentInputDelegate onIdentInput;

	IdentInputDeviceView(ViewAttachParams attach);
	void place() final;
	bool inputEvent(const Input::Event &) final;
	void draw(Gfx::RendererCommands &__restrict__) final;

private:
	Gfx::Text text;
};

class InputManagerView final: public TableView, public EmuAppHelper<InputManagerView>
{
public:
	InputManagerView(ViewAttachParams attach, InputManager &);
	~InputManagerView() final;
	void onShow() final;
	void pushAndShowDeviceView(const Input::Device &, const Input::Event &);

private:
	InputManager &inputManager;
	TextMenuItem deleteDeviceConfig;
	TextMenuItem deleteProfile;
	IG_UseMemberIf(Config::envIsAndroid, TextMenuItem, rescanOSDevices);
	TextMenuItem identDevice;
	TextMenuItem generalOptions;
	TextHeadingMenuItem deviceListHeading;
	std::vector<TextMenuItem> inputDevName;
	std::vector<MenuItem*> item;

	void loadItems();
};

class InputManagerOptionsView : public TableView, public EmuAppHelper<InputManagerOptionsView>
{
public:
	InputManagerOptionsView(ViewAttachParams attach, EmuInputView *emuInputView);

private:
	IG_UseMemberIf(MOGA_INPUT, BoolMenuItem, mogaInputSystem);
	IG_UseMemberIf(Config::Input::DEVICE_HOTSWAP, BoolMenuItem, notifyDeviceChange);
	IG_UseMemberIf(Config::Input::BLUETOOTH, TextHeadingMenuItem, bluetoothHeading);
	IG_UseMemberIf(Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, BoolMenuItem, keepBtActive);
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	TextMenuItem btScanSecsItem[5];
	MultiChoiceMenuItem btScanSecs;
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	BoolMenuItem btScanCache;
	#endif
	BoolMenuItem altGamepadConfirm;
	StaticArrayList<MenuItem*, 10> item;
	EmuInputView *emuInputView{};
};

class InputManagerDeviceView : public TableView, public EmuAppHelper<InputManagerDeviceView>
{
public:
	InputManagerDeviceView(UTF16String name, ViewAttachParams,
		InputManagerView &rootIMView, const Input::Device &, InputManager &);
	void onShow() final;

private:
	InputManager &inputManager;
	InputManagerView &rootIMView;
	TextMenuItem playerItem[6];
	MultiChoiceMenuItem player;
	TextMenuItem loadProfile;
	TextMenuItem renameProfile;
	TextMenuItem newProfile;
	TextMenuItem deleteProfile;
	IG_UseMemberIf(hasICadeInput, BoolMenuItem, iCadeMode);
	BoolMenuItem joystickAxis1DPad;
	BoolMenuItem joystickAxis2DPad;
	BoolMenuItem joystickAxisHatDPad;
	IG_UseMemberIf(Config::envIsAndroid, BoolMenuItem, consumeUnboundKeys);
	//TextMenuItem disconnect {"Disconnect"}; // TODO
	TextHeadingMenuItem categories;
	TextHeadingMenuItem options;
	std::vector<TextMenuItem> inputCategory;
	std::vector<MenuItem*> item;
	InputDeviceConfig &devConf;

	void confirmICadeMode();
	void loadItems();
	void addCategoryItem(const KeyCategory &cat);
};

}
