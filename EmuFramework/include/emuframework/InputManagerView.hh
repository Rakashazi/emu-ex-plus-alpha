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

class IdentInputDeviceView : public View
{
public:
	using OnIdentInputDelegate = DelegateFunc<void (const Input::KeyEvent &)>;
	OnIdentInputDelegate onIdentInput{};

	IdentInputDeviceView(ViewAttachParams attach);
	void place() final;
	bool inputEvent(const Input::Event &) final;
	void draw(Gfx::RendererCommands &cmds) final;

private:
	Gfx::Text text{};
};

class InputManagerView final: public TableView, public EmuAppHelper<InputManagerView>
{
public:
	InputManagerView(ViewAttachParams attach, KeyConfigContainer &, InputDeviceSavedConfigContainer &);
	~InputManagerView() final;
	void onShow() final;
	void pushAndShowDeviceView(const Input::Device &, const Input::Event &);

private:
	KeyConfigContainer *customKeyConfigsPtr{};
	InputDeviceSavedConfigContainer *savedInputDevsPtr{};
	TextMenuItem deleteDeviceConfig{};
	TextMenuItem deleteProfile{};
	IG_UseMemberIf(Config::envIsAndroid, TextMenuItem, rescanOSDevices);
	TextMenuItem identDevice{};
	TextMenuItem generalOptions{};
	TextHeadingMenuItem deviceListHeading{};
	std::vector<TextMenuItem> inputDevName{};
	std::vector<MenuItem*> item{};

	void loadItems();
	KeyConfigContainer &customKeyConfigs() const { return *customKeyConfigsPtr; };
	InputDeviceSavedConfigContainer &savedInputDevs() const { return *savedInputDevsPtr; };
};

class InputManagerOptionsView : public TableView, public EmuAppHelper<InputManagerOptionsView>
{
public:
	InputManagerOptionsView(ViewAttachParams attach, EmuInputView *emuInputView);

private:
	IG_UseMemberIf(MOGA_INPUT, BoolMenuItem, mogaInputSystem){};
	IG_UseMemberIf(Config::Input::DEVICE_HOTSWAP, BoolMenuItem, notifyDeviceChange){};
	#ifdef CONFIG_BLUETOOTH
	TextHeadingMenuItem bluetoothHeading{};
	IG_UseMemberIf(Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, BoolMenuItem, keepBtActive);
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	TextMenuItem btScanSecsItem[5];
	MultiChoiceMenuItem btScanSecs{};
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	BoolMenuItem btScanCache{};
	#endif
	BoolMenuItem altGamepadConfirm{};
	StaticArrayList<MenuItem*, 10> item{};
	EmuInputView *emuInputView{};
};

class InputManagerDeviceView : public TableView, public EmuAppHelper<InputManagerDeviceView>
{
public:
	InputManagerDeviceView(IG::utf16String name, ViewAttachParams,
		InputManagerView &rootIMView, const Input::Device &,
		KeyConfigContainer &, InputDeviceSavedConfigContainer &);
	void setPlayer(int playerVal);
	void onShow() final;

private:
	KeyConfigContainer *customKeyConfigsPtr{};
	InputDeviceSavedConfigContainer *savedInputDevsPtr{};
	InputManagerView &rootIMView;
	TextMenuItem playerItem[6];
	MultiChoiceMenuItem player{};
	TextMenuItem loadProfile{};
	TextMenuItem renameProfile{};
	TextMenuItem newProfile{};
	TextMenuItem deleteProfile{};
	#if defined CONFIG_INPUT_ICADE
	BoolMenuItem iCadeMode{};
	#endif
	BoolMenuItem joystickAxis1DPad{};
	BoolMenuItem joystickAxis2DPad{};
	BoolMenuItem joystickAxisHatDPad{};
	IG_UseMemberIf(Config::envIsAndroid, BoolMenuItem, consumeUnboundKeys){};
	//TextMenuItem disconnect {"Disconnect"}; // TODO
	StaticArrayList<TextMenuItem, Controls::MAX_CATEGORIES> inputCategory{};
	StaticArrayList<MenuItem*, Controls::MAX_CATEGORIES + 11> item{};
	InputDeviceConfig *devConf{};

	void confirmICadeMode();
	void loadItems();
	KeyConfigContainer &customKeyConfigs() const { return *customKeyConfigsPtr; };
	InputDeviceSavedConfigContainer &savedInputDevs() const { return *savedInputDevsPtr; };
};

}
