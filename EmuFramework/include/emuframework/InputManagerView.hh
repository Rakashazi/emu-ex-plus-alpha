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

#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/gfx/GfxText.hh>
#include <emuframework/EmuInput.hh>
#include <vector>
#include <array>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#endif

struct InputDeviceConfig;

class IdentInputDeviceView : public View
{
	IG::WindowRect viewFrame{};
	Gfx::Text text{};

public:
	using OnIdentInputDelegate = DelegateFunc<void (Input::Event e)>;
	OnIdentInputDelegate onIdentInput{};

	IdentInputDeviceView(ViewAttachParams attach);
	~IdentInputDeviceView() final;
	IG::WindowRect &viewRect() final { return viewFrame; }
	void place() final;
	bool inputEvent(Input::Event e) final;
	void draw(Gfx::RendererCommands &cmds) final;
};

class InputManagerView : public TableView
{
public:
	using DeviceNameString = std::array<char, MAX_INPUT_DEVICE_NAME_SIZE>;

	InputManagerView(ViewAttachParams attach);
	~InputManagerView();
	void onShow() final;
	const char *deviceName(uint idx) const;

private:
	std::vector<DeviceNameString> deviceConfigStr{};
	TextMenuItem deleteDeviceConfig{};
	std::vector<const char*> profileStr{};
	TextMenuItem deleteProfile{};
	#ifdef CONFIG_BASE_ANDROID
	TextMenuItem rescanOSDevices{};
	#endif
	TextMenuItem identDevice{};
	TextMenuItem generalOptions{};
	TextHeadingMenuItem deviceListHeading{};
	std::vector<TextMenuItem> inputDevName{};
	std::vector<DeviceNameString> inputDevNameStr{};
	std::vector<MenuItem*> item{};

	void loadItems();
};

class InputManagerOptionsView : public TableView
{
private:
	#ifdef CONFIG_BASE_ANDROID
	TextMenuItem relativePointerDecelItem[3];
	MultiChoiceMenuItem relativePointerDecel{};
	#endif
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	BoolMenuItem mogaInputSystem{};
	#endif
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	BoolMenuItem notifyDeviceChange{};
	#endif
	#ifdef CONFIG_BLUETOOTH
	TextHeadingMenuItem bluetoothHeading{};
	BoolMenuItem keepBtActive{};
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

public:
	InputManagerOptionsView(ViewAttachParams attach);
};

class InputManagerDeviceView : public TableView
{
private:
	InputManagerView &rootIMView;
	TextMenuItem playerItem[6];
	MultiChoiceMenuItem player{};
	char profileStr[128]{};
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
	//TextMenuItem disconnect {"Disconnect"}; // TODO
	TextMenuItem inputCategory[EmuControls::MAX_CATEGORIES]{};
	StaticArrayList<MenuItem*, EmuControls::MAX_CATEGORIES + 11> item{};
	InputDeviceConfig *devConf{};
	uint inputCategories = 0;

	void confirmICadeMode(Input::Event e);
	void loadItems();

public:
	InputManagerDeviceView(ViewAttachParams attach, InputManagerView &rootIMView, InputDeviceConfig &devConf);
	void setPlayer(int playerVal);
	void onShow() final;
};
