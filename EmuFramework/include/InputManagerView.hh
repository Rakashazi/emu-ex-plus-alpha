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

#include <input/Input.hh>
#include <MenuView.hh>
#include <EmuInput.hh>

class IdentInputDeviceView : public View
{
	Rect2<int> viewFrame;
	Gfx::Text text;

public:
	typedef DelegateFunc<void (const Input::Event &e)> OnIdentInputDelegate;
	OnIdentInputDelegate onIdentInput;

	constexpr IdentInputDeviceView() {}
	Rect2<int> &viewRect() override { return viewFrame; }
	void init();
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
};

class InputManagerView : public BaseMenuView
{
private:
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	InputPlayerMapMenuItem pointerInput;
	#endif
	char deviceConfigStr[MAX_SAVED_INPUT_DEVICES][MAX_INPUT_DEVICE_NAME_SIZE] { {0} };
	void deleteDeviceConfigHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem deleteDeviceConfig {"Delete Saved Device Settings"};
	const char *profileStr[MAX_CUSTOM_KEY_CONFIGS] {nullptr};
	void deleteProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem deleteProfile {"Delete Saved Key Profile"};
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	BoolMenuItem notifyDeviceChange {"Notify If Devices Change"};
	#endif
	#ifdef CONFIG_BASE_ANDROID
	void rescanOSDevicesHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem rescanOSDevices {"Re-scan OS Input Devices"};
	#endif
	void identDeviceHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem identDevice {"Auto-detect Device To Setup"};
	TextMenuItem inputDevName[Input::MAX_DEVS];
	MenuItem *item[sizeofArrayConst(inputDevName) + 6] = {nullptr};
public:
	constexpr InputManagerView(): BaseMenuView("Input Device Setup") { }

	char inputDevNameStr[Input::MAX_DEVS][80] { {0} };
	void init(bool highlightFirst);
	void deinit() override;
	void onShow() override;
};

class InputManagerDeviceView : public BaseMenuView
{
private:
	void playerHandler(MultiChoiceMenuItem &item, int val);
	MultiChoiceSelectMenuItem player;

	//TextMenuItem deleteDeviceConfig {"Delete Device Settings"};

	char profileStr[128] {0};
	void profileChanged(const KeyConfig &profile);
	void loadProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem loadProfile;

	void renameProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem renameProfile {"Rename Profile"};

	void newProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem newProfile {"New Profile"};

	void deleteProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem deleteProfile {"Delete Profile"};

	//void enabledHandler(BoolMenuItem &, const Input::Event &e);
	//BoolMenuItem enabled {"Enabled"};

	void confirmICadeMode(const Input::Event &e);
	void iCadeModeHandler(BoolMenuItem &, const Input::Event &e);
	#if defined CONFIG_INPUT_ICADE
		BoolMenuItem iCadeMode {"iCade Mode"};
	#endif

	void joystickAxis1DPadHandler(BoolMenuItem &, const Input::Event &e);
	BoolMenuItem joystickAxis1DPad {"Joystick Axis 1 as D-Pad"};

	//TextMenuItem disconnect {"Disconnect"}; // TODO

	TextMenuItem inputCategory[EmuControls::categories];
	MenuItem *item[EmuControls::categories + 9] = {nullptr};
	InputDeviceConfig *devConf = nullptr;
public:
	constexpr InputManagerDeviceView() { }

	void init(bool highlightFirst, InputDeviceConfig &devConf);
	void onShow() override;
};
