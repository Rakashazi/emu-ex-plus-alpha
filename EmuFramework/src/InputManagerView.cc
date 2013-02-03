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

#include <InputManagerView.hh>
#include <ButtonConfigView.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#ifdef CONFIG_BASE_ANDROID
#include <input/android/private.hh>
#endif
extern ViewStack viewStack;
extern InputManagerDeviceView imdMenu;
extern MsgPopup popup;
extern CollectTextInputView textInputView;
static ButtonConfigView bcMenu;
static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";
void updateOnScreenControlVisible();

void IdentInputDeviceView::init()
{
	text.init("Push a key on any input device enter its configuration menu", View::defaultFace);
	Input::setHandleVolumeKeys(1);
}

void IdentInputDeviceView::deinit()
{
	text.deinit();
	Input::setHandleVolumeKeys(0);
}

void IdentInputDeviceView::place()
{
	text.maxLineSize = Gfx::proj.w * 0.95;
	text.compile();
}

void IdentInputDeviceView::inputEvent(const Input::Event &e)
{
	if(e.isPointer() && e.state == Input::RELEASED)
	{
		removeModalView();
	}
	else if(!e.isPointer() && e.state == Input::PUSHED)
	{
		removeModalView();
		onIdentInput.invoke(e);
	}
}

void IdentInputDeviceView::draw()
{
	using namespace Gfx;
	setBlendMode(0);
	resetTransforms();
	setColor(.4, .4, .4, 1.);
	GeomRect::draw(viewFrame);
	setColor(COLOR_WHITE);
	text.draw(0, 0, C2DO, C2DO);
}

static void removeKeyConfFromAllDevices(const KeyConfig *conf)
{
	logMsg("removing saved key config %s from all devices", conf->name);
	forEachInDLList(&savedInputDevList, e)
	{
		if(e.keyConf == conf)
		{
			logMsg("used by saved device config %s,%d", e.name, e.devId);
			e.keyConf = nullptr;
		}
	}
}

template <size_t S>
static void printfDeviceNameWithNumber(char (&buffer)[S], const char *name, uint id)
{
	char idStr[sizeof(" #00")] = "";
	if(id)
		string_printf(idStr, " #%d", id + 1);
	string_printf(buffer, "%s%s", name, idStr);
}

void InputManagerView::confirmDeleteDeviceConfig(const Input::Event &e)
{
	auto it = savedInputDevList.iterator();
	iterateTimes(deleteDeviceConfigIdx, i)
	{
		it.advance();
	}
	logMsg("deleting device settings for: %s,%d", it.obj().name, it.obj().devId);
	iterateTimes(inputDevConfs, i)
	{
		if(inputDevConf[i].savedConf == &it.obj())
		{
			logMsg("removing from active device at idx: %d", i);
			inputDevConf[i].savedConf = nullptr;
			break;
		}
	}
	it.removeElem();
	keyMapping.buildAll();
	onShow();
}

bool InputManagerView::selectDeleteDeviceConfig(int i, const Input::Event &e)
{
	removeModalView();
	deleteDeviceConfigIdx = i;
	ynAlertView.init(confirmDeleteDeviceSettingsStr, !e.isPointer());
	ynAlertView.onYesDelegate().bind<InputManagerView, &InputManagerView::confirmDeleteDeviceConfig>(this);
	ynAlertView.placeRect(Gfx::viewportRect());
	modalView = &ynAlertView;
	Base::displayNeedsUpdate();
	return 0;
}

void InputManagerView::deleteDeviceConfigHandler(TextMenuItem &, const Input::Event &e)
{
	if(!savedInputDevList.size)
	{
		popup.post("No saved device settings");
		return;
	}
	int devs = 0;
	forEachInDLList(&savedInputDevList, e)
	{
		printfDeviceNameWithNumber(deviceConfigStr[devs++], e.name, e.devId);
	}
	multiChoiceView.init(deviceConfigStr, devs, !e.isPointer());
	multiChoiceView.placeRect(Gfx::viewportRect());
	multiChoiceView.onSelectDelegate().bind<InputManagerView, &InputManagerView::selectDeleteDeviceConfig>(this);
	modalView = &multiChoiceView;
	Base::displayNeedsUpdate();
}

void InputManagerView::confirmDeleteProfile(const Input::Event &e)
{
	auto it = customKeyConfig.iterator();
	iterateTimes(deleteProfileIdx, i)
	{
		it.advance();
	}
	logMsg("deleting profile: %s", it.obj().name);
	removeKeyConfFromAllDevices(&it.obj());
	it.removeElem();
	keyMapping.buildAll();
	onShow();
}

bool InputManagerView::selectDeleteProfile(int i, const Input::Event &e)
{
	removeModalView();
	deleteProfileIdx = i;
	ynAlertView.init(confirmDeleteProfileStr, !e.isPointer());
	ynAlertView.onYesDelegate().bind<InputManagerView, &InputManagerView::confirmDeleteProfile>(this);
	ynAlertView.placeRect(Gfx::viewportRect());
	modalView = &ynAlertView;
	Base::displayNeedsUpdate();
	return 0;
}

void InputManagerView::deleteProfileHandler(TextMenuItem &, const Input::Event &e)
{
	if(!customKeyConfig.size)
	{
		popup.post("No saved profiles");
		return;
	}
	int profiles = 0;
	forEachInDLList(&customKeyConfig, e)
	{
		profileStr[profiles++] = e.name;
	}
	multiChoiceView.init(profileStr, profiles, !e.isPointer());
	multiChoiceView.placeRect(Gfx::viewportRect());
	multiChoiceView.onSelectDelegate().bind<InputManagerView, &InputManagerView::selectDeleteProfile>(this);
	modalView = &multiChoiceView;
	Base::displayNeedsUpdate();
}

#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
void InputManagerView::notifyDeviceChangeHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	optionNotifyInputDeviceChange = item.on;
}
#endif

#ifdef CONFIG_BASE_ANDROID
void InputManagerView::rescanOSDevicesHandler(TextMenuItem &, const Input::Event &e)
{
	using namespace Input;
	Input::rescanDevices(0);
	uint devices = 0;
	forEachInDLList(&Input::devList, e)
	{
		if(e.map() == Event::MAP_KEYBOARD || e.map() == Event::MAP_ICADE)
			devices++;
	}
	popup.printf(2, 0, "%d OS devices present", devices);
}
#endif

void InputManagerView::onShow()
{
	BaseMenuView::onShow();
	deleteDeviceConfig.active = savedInputDevList.size;
	deleteProfile.active = customKeyConfig.size;
}

void InputManagerView::init(bool highlightFirst)
{
	uint i = 0;
	assert(EmuSystem::maxPlayers <= 5);
	#ifdef INPUT_SUPPORTS_POINTER
	if(EmuSystem::maxPlayers > 1)
	{
		pointerInput.init("On-screen Input Player", &pointerInputPlayer); item[i++] = &pointerInput;
	}
	#endif
	deleteDeviceConfig.init((bool)savedInputDevList.size); item[i++] = &deleteDeviceConfig;
	deleteDeviceConfig.selectDelegate().bind<InputManagerView, &InputManagerView::deleteDeviceConfigHandler>(this);
	deleteProfile.init((bool)customKeyConfig.size); item[i++] = &deleteProfile;
	deleteProfile.selectDelegate().bind<InputManagerView, &InputManagerView::deleteProfileHandler>(this);
	identDevice.init(); item[i++] = &identDevice;
	identDevice.selectDelegate().bind<InputManagerView, &InputManagerView::identDeviceHandler>(this);
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	if(!optionNotifyInputDeviceChange.isConst)
	{
		notifyDeviceChange.init(optionNotifyInputDeviceChange); item[i++] = &notifyDeviceChange;
		notifyDeviceChange.selectDelegate().bind<InputManagerView, &InputManagerView::notifyDeviceChangeHandler>(this);
	}
	#endif
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12)
	{
		rescanOSDevices.init(); item[i++] = &rescanOSDevices;
		rescanOSDevices.selectDelegate().bind<InputManagerView, &InputManagerView::rescanOSDevicesHandler>(this);
	}
	#endif
	devStart = i;
	forEachInDLList(&Input::devList, e)
	{
		auto idx = i-devStart;
		printfDeviceNameWithNumber(inputDevNameStr[idx], e.name(), e.devId);
		inputDevName[idx].init(inputDevNameStr[idx]); item[i++] = &inputDevName[idx];
	}
	BaseMenuView::init(item, i, highlightFirst);
}

void InputManagerView::onSelectElement(const GuiTable1D *, const Input::Event &e, uint i)
{
	if(i < devStart)
		item[i]->select(this, e);
	else
	{
		imdMenu.init(!e.isPointer(), inputDevConf[i-devStart]);
		imdMenu.name_ = inputDevNameStr[i-devStart];
		viewStack.pushAndShow(&imdMenu);
	}
}

void InputManagerView::onIdentInput(const Input::Event &e)
{
	auto dev = e.device;
	if(dev)
	{
		imdMenu.init(1, inputDevConf[dev->idx]);
		imdMenu.name_ = inputDevNameStr[dev->idx];
		viewStack.pushAndShow(&imdMenu);
	}
}

void InputManagerView::identDeviceHandler(TextMenuItem &, const Input::Event &e)
{
	identView.init();
	identView.onIdentInput.bind<InputManagerView, &InputManagerView::onIdentInput>(this);
	identView.placeRect(Gfx::viewportRect());
	View::modalView = &identView;
}

static class ProfileSelectMenu : public BaseMultiChoiceView
{
public:
	constexpr ProfileSelectMenu() { }
	TextMenuItem choiceEntry[MAX_DEFAULT_KEY_CONFIGS_PER_TYPE + MAX_CUSTOM_KEY_CONFIGS];
	MenuItem *choiceEntryItem[sizeofArrayConst(choiceEntry)] {nullptr};
	uint customConfigs = 0;
	KeyConfig *customConfig[MAX_CUSTOM_KEY_CONFIGS] {nullptr};
	typedef Delegate<void (const KeyConfig &profile)> ProfileChangeDelegate;
	ProfileChangeDelegate profileChangeDel;
	Input::Device *dev = nullptr;

	void init(bool highlightFirst, Input::Device &dev)
	{
		this->dev = &dev;
		customConfigs = 0;
		uint i = 0;
		forEachInDLList(&customKeyConfig, e)
		{
			if(e.map == dev.map())
			{
				choiceEntry[i].init(e.name); choiceEntryItem[i] = &choiceEntry[i];
				customConfig[customConfigs++] = &e;
				i++;
				if(i >= sizeofArray(choiceEntry))
					break;
			}
		}
		uint defaultConfs = 0;
		auto defaultConf = KeyConfig::defaultConfigsForDevice(dev, defaultConfs);
		assert(i + defaultConfs < sizeofArray(choiceEntry));
		iterateTimes(defaultConfs, c)
		{
			choiceEntry[i].init(defaultConf[c].name); choiceEntryItem[i] = &choiceEntry[i];
			i++;
		}
		BaseMenuView::init(choiceEntryItem, i, highlightFirst, C2DO);
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		removeModalView();
		if(i < customConfigs)
		{
			profileChangeDel.invoke(*customConfig[i]);
		}
		else
		{
			i -= customConfigs;
			profileChangeDel.invoke(KeyConfig::defaultConfigsForDevice(*dev)[i]);
		}
	}
} profileSelectMenu;

void InputManagerDeviceView::profileChanged(const KeyConfig &profile)
{
	logMsg("set key profile %s", profile.name);
	devConf->setKeyConf(profile);
	onShow();
	keyMapping.buildAll();
}

void InputManagerDeviceView::loadProfileHandler(TextMenuItem &, const Input::Event &e)
{
	profileSelectMenu.init(!e.isPointer(), *devConf->dev);
	profileSelectMenu.placeRect(Gfx::viewportRect());
	profileSelectMenu.profileChangeDel.bind<InputManagerDeviceView, &InputManagerDeviceView::profileChanged>(this);
	modalView = &profileSelectMenu;
	Base::displayNeedsUpdate();
}

/*void InputManagerDeviceView::enabledHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	devConf->enabled = item.on;
	devConf->save();
	onShow();
	keyMapping.buildAll();
}*/

void InputManagerDeviceView::confirmDeleteProfile(const Input::Event &e)
{
	auto conf = devConf->mutableKeyConf();
	if(!conf)
	{
		bug_exit("confirmed deletion of a read-only key config, should never happen");
		return;
	}
	logMsg("deleting profile: %s", conf->name);
	removeKeyConfFromAllDevices(conf);
	customKeyConfig.remove(*conf);
	onShow();
	keyMapping.buildAll();
}

void InputManagerDeviceView::deleteProfileHandler(TextMenuItem &, const Input::Event &e)
{
	if(!devConf->mutableKeyConf())
	{
		popup.post("Can't delete a built-in profile", 2, 0);
		return;
	}
	ynAlertView.init(confirmDeleteProfileStr, !e.isPointer());
	ynAlertView.onYesDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::confirmDeleteProfile>(this);
	ynAlertView.placeRect(Gfx::viewportRect());
	modalView = &ynAlertView;
	Base::displayNeedsUpdate();
}

void InputManagerDeviceView::onShow()
{
	BaseMenuView::onShow();
	//deleteDeviceConfig.active = devConf->savedConf;
	string_printf(profileStr, "Profile: %s", devConf->keyConf().name);
	loadProfile.compile();
	bool keyConfIsMutable = devConf->mutableKeyConf();
	renameProfile.active = keyConfIsMutable;
	deleteProfile.active = keyConfIsMutable;
}

static uint playerConfToMenuIdx(uint player)
{
	if(player == InputDeviceConfig::PLAYER_MULTI)
		return 0;
	else
	{
		assert(player < EmuSystem::maxPlayers);
		return player + 1;
	}
}

void InputManagerDeviceView::playerHandler(MultiChoiceMenuItem &item, int val)
{
	uint playerVal = val == 0 ? InputDeviceConfig::PLAYER_MULTI : val-1;
	bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player != InputDeviceConfig::PLAYER_MULTI) ||
			(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player == InputDeviceConfig::PLAYER_MULTI);
	devConf->player = playerVal;
	devConf->save();
	if(changingMultiplayer)
	{
		viewStack.pop();
		init(0, *devConf);
		viewStack.pushAndShow(this);
	}
	else
		onShow();
	keyMapping.buildAll();
}

/*void InputManagerDeviceView::confirmDeleteDeviceConfig(const Input::Event &e)
{
	logMsg("deleting device settings for: %s,%d", devConf->dev->name(), devConf->dev->devId);
	devConf->deleteConf();
	devConf->reset();
	viewStack.popAndShow();
	keyMapping.buildAll();
}

void InputManagerDeviceView::deleteDeviceConfigHandler(TextMenuItem &item, const Input::Event &e)
{
	if(item.active)
	{
		ynAlertView.init(confirmDeleteDeviceSettingsStr, !e.isPointer());
		ynAlertView.onYesDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::confirmDeleteDeviceConfig>(this);
		ynAlertView.placeRect(Gfx::viewportRect());
		modalView = &ynAlertView;
		Base::displayNeedsUpdate();
	}
	else
	{
		popup.post("This device has no saved settings yet", 2, 0);
	}
}*/

void InputManagerDeviceView::confirmICadeMode(const Input::Event &e)
{
	iCadeMode.toggle();
	devConf->setICadeMode(iCadeMode.on);
	onShow();
	physicalControlsPresent = EmuControls::keyInputIsPresent();
	updateOnScreenControlVisible();
	keyMapping.buildAll();
}

void InputManagerDeviceView::iCadeModeHandler(BoolMenuItem &item, const Input::Event &e)
{
	#ifdef CONFIG_BASE_IOS
		confirmICadeMode(e);
	#else
		if(!item.on)
		{
			ynAlertView.init("This mode allows input from an iCade-compatible Bluetooth device, don't turn on if this isn't an iCade", !e.isPointer());
			ynAlertView.onYesDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::confirmICadeMode>(this);
			ynAlertView.placeRect(Gfx::viewportRect());
			modalView = &ynAlertView;
			Base::displayNeedsUpdate();
		}
		else
			confirmICadeMode(e);
	#endif
}

uint InputManagerDeviceView::handleRenameProfileFromTextInput(const char *str)
{
	if(str && strlen(str))
	{
		if(customKeyConfigsContainName(str))
		{
			popup.postError("Another profile is already using this name");
			Base::displayNeedsUpdate();
			return 1;
		}
		assert(devConf->mutableKeyConf());
		string_copy(devConf->mutableKeyConf()->name, str);
		onShow();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void InputManagerDeviceView::renameProfileHandler(TextMenuItem &item, const Input::Event &e)
{
	if(!devConf->mutableKeyConf())
	{
		popup.post("Can't rename a built-in profile", 2, 0);
		Base::displayNeedsUpdate();
		return;
	}
	textInputView.init("Input name", devConf->keyConf().name);
	textInputView.onTextDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::handleRenameProfileFromTextInput>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

uint InputManagerDeviceView::handleNewProfileFromTextInput(const char *str)
{
	if(str && strlen(str))
	{
		if(customKeyConfigsContainName(str))
		{
			popup.postError("Another profile is already using this name");
			Base::displayNeedsUpdate();
			return 1;
		}
		if(devConf->setKeyConfCopiedFromExisting(str))
		{
			logMsg("created new profile %s", devConf->keyConf().name);
			onShow();
		}
		else
			popup.postError("Too many saved device settings, please delete one");
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void InputManagerDeviceView::confirmNewProfile(const Input::Event &e)
{
	textInputView.init("Input name", "");
	textInputView.onTextDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::handleNewProfileFromTextInput>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void InputManagerDeviceView::newProfileHandler(TextMenuItem &item, const Input::Event &e)
{
	if(customKeyConfig.isFull())
	{
		popup.postError("No space left for new key profiles, please delete one");
		Base::displayNeedsUpdate();
		return;
	}
	ynAlertView.init("Create a new profile? All keys from the current profile will be copied over.", !e.isPointer());
	ynAlertView.onYesDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::confirmNewProfile>(this);
	ynAlertView.placeRect(Gfx::viewportRect());
	modalView = &ynAlertView;
}

void InputManagerDeviceView::joystickAxis1DPadHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	devConf->setMapJoystickAxis1ToDpad(item.on);
	devConf->save();
}

void InputManagerDeviceView::init(bool highlightFirst, InputDeviceConfig &devConf)
{
	this->devConf = &devConf;
	uint i = 0;
	if(EmuSystem::maxPlayers > 1)
	{
		static const char *str[] = { "Multiple", "1", "2", "3", "4", "5" };
		player.init("Player", str, playerConfToMenuIdx(devConf.player), EmuSystem::maxPlayers+1); item[i++] = &player;
		player.valueDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::playerHandler>(this);
	}
	//enabled.init((bool)devConf.enabled); item[i++] = &enabled;
	//enabled.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::enabledHandler>(this);
	#if defined CONFIG_INPUT_ICADE
	if((devConf.dev->map() == Input::Event::MAP_KEYBOARD && !devConf.dev->hasGamepad())
			|| devConf.dev->map() == Input::Event::MAP_ICADE)
	{
		iCadeMode.init(devConf.iCadeMode()); item[i++] = &iCadeMode;
		iCadeMode.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::iCadeModeHandler>(this);
	}
	#endif
	if(devConf.dev->hasJoystick())
	{
		joystickAxis1DPad.init(devConf.mapJoystickAxis1ToDpad()); item[i++] = &joystickAxis1DPad;
		joystickAxis1DPad.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::joystickAxis1DPadHandler>(this);
	}
	/*deleteDeviceConfig.init((bool)devConf.savedConf); item[i++] = &deleteDeviceConfig;
	deleteDeviceConfig.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::deleteDeviceConfigHandler>(this);*/
	string_printf(profileStr, "Profile: %s", devConf.keyConf().name);
	loadProfile.init(profileStr); item[i++] = &loadProfile;
	loadProfile.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::loadProfileHandler>(this);
	newProfile.init(); item[i++] = &newProfile;
	newProfile.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::newProfileHandler>(this);
	renameProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &renameProfile;
	renameProfile.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::renameProfileHandler>(this);
	deleteProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &deleteProfile;
	deleteProfile.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::deleteProfileHandler>(this);
	categoryStart = i;
	forEachInArray(EmuControls::category, c)
	{
		if(EmuControls::category[c_i].isMultiplayer && devConf.player != InputDeviceConfig::PLAYER_MULTI)
		{
			//logMsg("skipping category %s (%d)", EmuControls::category[c_i].name, (int)c_i);
			continue;
		}
		categoryMap[i-categoryStart] = c_i;
		inputCategory[c_i].init(c->name); item[i++] = &inputCategory[c_i];
	}
	BaseMenuView::init(item, i, highlightFirst);
}

void InputManagerDeviceView::onSelectElement(const GuiTable1D *, const Input::Event &e, uint i)
{
	if(i < categoryStart)
		item[i]->select(this, e);
	else
	{
		bcMenu.init(&EmuControls::category[categoryMap[i-categoryStart]], *devConf, !e.isPointer());
		viewStack.pushAndShow(&bcMenu);
	}
}
