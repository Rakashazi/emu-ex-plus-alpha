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

#pragma once

#include <gui/View.hh>
#include <gui/AlertView.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <util/gui/BaseMenuView.hh>
#include <audio/Audio.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <MultiChoiceView.hh>
#include <TouchConfigView.hh>
#include <EmuView.hh>
#include <FilePicker.hh>

extern YesNoAlertView ynAlertView;
extern ViewStack viewStack;
#ifdef INPUT_SUPPORTS_POINTER
extern TouchConfigView tcMenu;
#endif
extern EmuView emuView;
extern EmuFilePicker fPicker;
void setupStatusBarInMenu();
void setupFont();
void applyOSNavStyle();
ResourceImage *getArrowAsset();
extern WorkDirStack<1> workDirStack;
void onCloseModalPopWorkDir(const Input::Event &e);
void chdirFromFilePath(const char *path);

class OptionView : public BaseMenuView
{
protected:
	BoolMenuItem snd {"Sound", BoolMenuItem::SelectDelegate::create<&soundHandler>()};
	static void soundHandler(BoolMenuItem &item, const Input::Event &e);

	#ifdef CONFIG_AUDIO_OPENSL_ES
	BoolMenuItem sndUnderrunCheck {"Strict Underrun Check", BoolMenuItem::SelectDelegate::create<&soundUnderrunCheckHandler>()};
	static void soundUnderrunCheckHandler(BoolMenuItem &item, const Input::Event &e);
	#endif

	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	BoolMenuItem useOSInputMethod {"Skip OS Input Method", BoolMenuItem::SelectDelegate::create<&useOSInputMethodHandler>()};
	static void useOSInputMethodHandler(BoolMenuItem &item, const Input::Event &e);
	#endif

	#ifdef CONFIG_ENV_WEBOS
	BoolMenuItem
	#else
	MultiChoiceSelectMenuItem
	#endif
	touchCtrl {"On-screen Controls"};

	void touchCtrlInit();

	TextMenuItem touchCtrlConfig {"On-screen Config"};

	MultiChoiceSelectMenuItem autoSaveState {"Auto-save State"};

	void autoSaveStateInit();

	MultiChoiceSelectMenuItem statusBar {"Hide Status Bar"};

	void statusBarInit();

	MultiChoiceSelectMenuItem frameSkip {"Frame Skip"};

	void frameSkipInit();

	MultiChoiceSelectMenuItem audioRate {"Sound Rate"};

	void audioRateInit();

	#ifdef CONFIG_BASE_ANDROID
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
			BoolMenuItem directTexture {"Direct Texture"};
		#endif
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
			BoolMenuItem surfaceTexture {"Fast CPU->GPU Copy"};
		#endif
		BoolMenuItem glSyncHack {"GPU Sync Hack"};
	#endif

	BoolMenuItem confirmAutoLoadState {"Confirm Auto-load State"};

	BoolMenuItem pauseUnfocused {Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused"};

	BoolMenuItem largeFonts {"Large Fonts"};

	BoolMenuItem notificationIcon {"Suspended App Icon"};

	BoolMenuItem lowProfileOSNav {"Dim OS Navigation"};

	BoolMenuItem hideOSNav {"Hide OS Navigation"};

	BoolMenuItem idleDisplayPowerSave {"Dim Screen If Idle"};

	BoolMenuItem altGamepadConfirm {"Alt Gamepad Confirm"};

	BoolMenuItem dither {"Dither Image"};

	BoolMenuItem navView {"Title Bar"};

	BoolMenuItem backNav {"Title Back Navigation"};

	BoolMenuItem rememberLastMenu {"Remember Last Menu"};

	BoolMenuItem confirmOverwriteState {"Confirm Overwrite State"};

#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID) || defined (CONFIG_BASE_IOS)
	BoolMenuItem bestColorModeHint {"Use Highest Color Mode"};
	void bestColorModeHintHandler(BoolMenuItem &item, const Input::Event &e);
	void confirmBestColorModeHintAlert(const Input::Event &e);
#endif

	void savePathUpdated(const char *newPath);
	void savePathHandler(TextMenuItem &, const Input::Event &e);
	char savePathStr[256] {0};
	TextMenuItem savePath {""};

	#ifdef CONFIG_BLUETOOTH

	MultiChoiceSelectMenuItem btScanSecs {"Bluetooth Scan"};

	void btScanSecsInit();

	BoolMenuItem keepBtActive {"Background Bluetooth"};

	BoolMenuItem btScanCache {"Bluetooth Scan Cache"};
	#endif

	MultiChoiceSelectMenuItem gameOrientation {"Orientation"};

	void gameOrientationInit();

	MultiChoiceSelectMenuItem menuOrientation {"Orientation"};

	void menuOrientationInit();

	MultiChoiceSelectMenuItem aspectRatio {"Aspect Ratio"};

	void aspectRatioInit();

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	MultiChoiceSelectMenuItem soundBuffers {"Buffer Size In Frames"};

	void soundBuffersInit();
#endif

	MultiChoiceSelectMenuItem zoom {"Zoom"};

	void zoomInit();

	MultiChoiceSelectMenuItem imgFilter {"Image Filter"};

	void imgFilterInit();

	MultiChoiceSelectMenuItem overlayEffect {"Overlay Effect"};

	void overlayEffectInit();

	MultiChoiceSelectMenuItem overlayEffectLevel {"Overlay Effect Level"};

	void overlayEffectLevelInit();

	MultiChoiceSelectMenuItem relativePointerDecel {"Trackball Sensitivity"};

	void relativePointerDecelInit();

#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
	void processPriorityInit();
	MultiChoiceSelectMenuItem processPriority;
#endif

	virtual void loadVideoItems(MenuItem *item[], uint &items);

	virtual void loadAudioItems(MenuItem *item[], uint &items);

	virtual void loadInputItems(MenuItem *item[], uint &items);

	virtual void loadSystemItems(MenuItem *item[], uint &items);

	virtual void loadGUIItems(MenuItem *item[], uint &items);

	MenuItem *item[24] {nullptr};
public:
	constexpr OptionView(): BaseMenuView("Options") { }

	void init(uint idx, bool highlightFirst);
};

class BiosSelectMenu : public BaseMultiChoiceView
{
public:
	constexpr BiosSelectMenu() { }
	constexpr BiosSelectMenu(FsSys::cPath *biosPathStr, int (*fsFilter)(const char *name, int type))
		:biosPathStr(biosPathStr), fsFilter(fsFilter) { }
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2] {nullptr};
	typedef Delegate<void ()> BiosChangeDelegate;
	BiosChangeDelegate biosChangeDel;
	FsSys::cPath *biosPathStr = nullptr;
	int (*fsFilter)(const char *name, int type) = nullptr;

	void onSelectFile(const char* name, const Input::Event &e)
	{
		logMsg("size %d", (int)sizeof(*biosPathStr));
		snprintf(*biosPathStr, sizeof(*biosPathStr), "%s/%s", FsSys::workDir(), name);
		biosChangeDel.invokeSafe();
		View::removeModalView();
		workDirStack.pop();
	}

	void init(FsSys::cPath *biosPathStr, int (*fsFilter)(const char *name, int type), bool highlightFirst)
	{
		var_selfs(biosPathStr);
		var_selfs(fsFilter);
		init(highlightFirst);
	}

	void init(bool highlightFirst)
	{
		assert(biosPathStr);
		choiceEntry[0].init("Select File"); choiceEntryItem[0] = &choiceEntry[0];
		choiceEntry[1].init("Unset"); choiceEntryItem[1] = &choiceEntry[1];
		BaseMenuView::init(choiceEntryItem, sizeofArray(choiceEntry), highlightFirst, C2DO);
	}

	void onSelectElement(const GuiTable1D *, const Input::Event &e, uint i)
	{
		removeModalView();
		if(i == 0)
		{
			workDirStack.push();
			chdirFromFilePath(*biosPathStr);
			fPicker.init(!e.isPointer(), fsFilter);
			fPicker.onSelectFileDelegate().bind<BiosSelectMenu, &BiosSelectMenu::onSelectFile>(this);
			fPicker.onCloseDelegate().bind<&onCloseModalPopWorkDir>();
			fPicker.placeRect(Gfx::viewportRect());
			modalView = &fPicker;
			Base::displayNeedsUpdate();
		}
		else
		{
			strcpy(*biosPathStr, "");
			biosChangeDel.invokeSafe();
		}
	}
};
