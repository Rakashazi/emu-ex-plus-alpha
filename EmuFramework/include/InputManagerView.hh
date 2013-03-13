#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <input/Input.hh>
#include <MenuView.hh>
#include <EmuInput.hh>

class IdentInputDeviceView : public View
{
	Rect2<int> viewFrame;
	Gfx::Text text;

public:
	constexpr IdentInputDeviceView() { }

	typedef Delegate<void (const Input::Event &e)> OnIdentInputDelegate;
	OnIdentInputDelegate onIdentInput;

	Rect2<int> &viewRect() { return viewFrame; }

	void init();
	void deinit();
	void place();
	void inputEvent(const Input::Event &e);
	void draw(Gfx::FrameTimeBase frameTime);
};

class InputManagerView : public BaseMenuView
{
private:
	#ifdef INPUT_SUPPORTS_POINTER
		InputPlayerMapMenuItem pointerInput;
	#endif
	char deviceConfigStr[MAX_SAVED_INPUT_DEVICES][MAX_INPUT_DEVICE_NAME_SIZE] { {0} };
	uint deleteDeviceConfigIdx = 0;
	void confirmDeleteDeviceConfig(const Input::Event &e);
	bool selectDeleteDeviceConfig(int i, const Input::Event &e);
	void deleteDeviceConfigHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem deleteDeviceConfig {"Delete Saved Device Settings"};
	const char *profileStr[MAX_CUSTOM_KEY_CONFIGS] {nullptr};
	uint deleteProfileIdx = 0;
	void confirmDeleteProfile(const Input::Event &e);
	bool selectDeleteProfile(int i, const Input::Event &e);
	void deleteProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem deleteProfile {"Delete Saved Key Profile"};
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	void notifyDeviceChangeHandler(BoolMenuItem &, const Input::Event &e);
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
	uint devStart = 0;
	void onIdentInput(const Input::Event &e);
	IdentInputDeviceView identView;
public:
	constexpr InputManagerView(): BaseMenuView("Input Device Setup") { }

	char inputDevNameStr[Input::MAX_DEVS][80] { {0} };
	void init(bool highlightFirst);
	void onShow();
	void onSelectElement(const GuiTable1D *, const Input::Event &e, uint i);
};

class InputManagerDeviceView : public BaseMenuView
{
private:
	void playerHandler(MultiChoiceMenuItem &item, int val);
	MultiChoiceSelectMenuItem player;

	//void confirmDeleteDeviceConfig(const Input::Event &e);
	//void deleteDeviceConfigHandler(TextMenuItem &, const Input::Event &e);
	//TextMenuItem deleteDeviceConfig {"Delete Device Settings"};

	char profileStr[128] {0};
	void profileChanged(const KeyConfig &profile);
	void loadProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem loadProfile;

	uint handleRenameProfileFromTextInput(const char *str);
	void renameProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem renameProfile {"Rename Profile"};

	uint handleNewProfileFromTextInput(const char *str);
	void confirmNewProfile(const Input::Event &e);
	void newProfileHandler(TextMenuItem &, const Input::Event &e);
	TextMenuItem newProfile {"New Profile"};

	void confirmDeleteProfile(const Input::Event &e);
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
	uint categoryStart = 0;
	uint categoryMap[EmuControls::categories] {0};
public:
	constexpr InputManagerDeviceView() { }

	void init(bool highlightFirst, InputDeviceConfig &devConf);
	void onShow();
	void onSelectElement(const GuiTable1D *, const Input::Event &e, uint i);
};
