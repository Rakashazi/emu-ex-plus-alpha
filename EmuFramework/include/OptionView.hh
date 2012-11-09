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
#include <ButtonConfigView.hh>
#include <TouchConfigView.hh>
#include <EmuView.hh>

extern YesNoAlertView ynAlertView;
extern ButtonConfigView bcMenu;
extern KeyConfig<EmuControls::systemTotalKeys> keyConfig;
extern ViewStack viewStack;
extern TouchConfigView tcMenu;
extern EmuView emuView;
void setupStatusBarInMenu();
void setupFont();
void applyOSNavStyle();
ResourceImage *getArrowAsset();

class ButtonConfigCategoryView : public BaseMenuView
{
	struct ProfileItem
	{
		const char *name[10];
		uint names;
		uint val;
		uint devType;

		void init(uint devType, KeyProfileManager *profileMgr)
		{
			names = profileMgr->defaultProfiles + 1;
			assert(names < sizeofArray(name));
			name[0] = "Unbind All";
			iterateTimes(profileMgr->defaultProfiles, i)
			{
				name[i+1] = profileMgr->defaultProfile[i].name;
			}
			var_selfSet(devType);
		}

		void profileHandler(TextMenuItem &, const InputEvent &e)
		{
			multiChoiceView.init(name, names, !e.isPointer());
			multiChoiceView.onSelectDelegate().bind<ProfileItem, &ProfileItem::setProfileFromChoice>(this);
			multiChoiceView.placeRect(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		bool setProfileFromChoice(int val, const InputEvent &e)
		{
			removeModalView();
			this->val = val;
			ynAlertView.init("Really apply new key bindings?", !e.isPointer());
			ynAlertView.onYesDelegate().bind<ProfileItem, &ProfileItem::confirmAlert>(this);
			ynAlertView.placeRect(Gfx::viewportRect());
			modalView = &ynAlertView;
			return 0;
		}

		void confirmAlert(const InputEvent &e)
		{
			keyConfig.loadProfile(devType, val-1);
			keyMapping.build(EmuControls::category);
		}

	} profileItem;
	TextMenuItem profile;

	struct CategoryItem
	{
		KeyCategory *cat;
		uint devType;
		void init(KeyCategory *cat, uint devType)
		{
			var_selfs(cat);
			var_selfs(devType);
		}

		void categoryHandler(TextMenuItem &, const InputEvent &e)
		{
			bcMenu.init(cat, devType, !e.isPointer());
			viewStack.pushAndShow(&bcMenu);
		}
	} catItem[sizeofArrayConst(EmuControls::category)];
	TextMenuItem cat[sizeofArrayConst(EmuControls::category)];

	MenuItem *item[sizeofArrayConst(EmuControls::category) + 1];
public:
	void init(uint devType, bool highlightFirst)
	{
		using namespace EmuControls;
		name_ = InputEvent::devTypeName(devType);
		uint i = 0;
		profile.init("Load Defaults"); item[i++] = &profile;
		profileItem.init(devType, &EmuControls::profileManager(devType));
		profile.selectDelegate().bind<ProfileItem, &ProfileItem::profileHandler>(&profileItem);
		forEachInArray(category, c)
		{
			cat[c_i].init(c->name); item[i++] = &cat[c_i];
			catItem[c_i].init(c, devType);
			cat[c_i].selectDelegate().bind<CategoryItem, &CategoryItem::categoryHandler>(&catItem[c_i]);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

class OptionView : public BaseMenuView
{
protected:
	BoolMenuItem snd {"Sound"};

	#ifdef CONFIG_AUDIO_OPENSL_ES
	BoolMenuItem sndUnderrunCheck {"Strict Underrun Check"};
	#endif

	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	BoolMenuItem useOSInputMethod {"Skip OS Input Method"};
	#endif

	#ifdef CONFIG_ENV_WEBOS
	BoolMenuItem touchCtrl;
	#else
	MultiChoiceSelectMenuItem touchCtrl {"On-screen Controls"};
	#endif

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

	TextMenuItem buttonConfig {"Key Config"};

	#ifdef CONFIG_INPUT_ICADE
	BoolMenuItem iCade {"Use iCade"};
	TextMenuItem iCadeButtonConfig {"iCade Key Config"};
	#endif

	#ifdef CONFIG_BLUETOOTH
	TextMenuItem wiiButtonConfig {"Wiimote Key Config"};
	TextMenuItem iCPButtonConfig {"iControlPad Key Config"};
	TextMenuItem zeemoteButtonConfig {"Zeemote JS1 Key Config"};

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

	MultiChoiceSelectMenuItem dpi {"DPI Override"};

	void dpiInit();

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
