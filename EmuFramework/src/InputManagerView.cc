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

#include <InputManagerView.hh>
#include <ButtonConfigView.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#ifdef CONFIG_BASE_ANDROID
#include <input/android/private.hh>
#endif
extern ViewStack viewStack;
extern MsgPopup popup;
extern InputManagerView *imMenu;
static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

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
		onIdentInput(e);
	}
}

void IdentInputDeviceView::draw(Gfx::FrameTimeBase frameTime)
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
	auto &multiChoiceView = *allocModalView<MultiChoiceView>();
	multiChoiceView.init(deviceConfigStr, devs, !e.isPointer());
	multiChoiceView.onSelect() =
		[this](int i, const Input::Event &e)
		{
			removeModalView();
			int deleteDeviceConfigIdx = i;
			auto &ynAlertView = *allocModalView<YesNoAlertView>();
			ynAlertView.init(confirmDeleteDeviceSettingsStr, !e.isPointer());
			ynAlertView.onYes() =
				[this, deleteDeviceConfigIdx](const Input::Event &e)
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
				};
			View::addModalView(ynAlertView);
			return 0;
		};
	View::addModalView(multiChoiceView);
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
	auto &multiChoiceView = *allocModalView<MultiChoiceView>();
	multiChoiceView.init(profileStr, profiles, !e.isPointer());
	multiChoiceView.onSelect() =
		[this](int i, const Input::Event &e)
		{
			removeModalView();
			int deleteProfileIdx = i;
			auto &ynAlertView = *allocModalView<YesNoAlertView>();
			ynAlertView.init(confirmDeleteProfileStr, !e.isPointer());
			ynAlertView.onYes() =
				[this, deleteProfileIdx](const Input::Event &e)
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
				};
			View::addModalView(ynAlertView);
			return 0;
		};
	View::addModalView(multiChoiceView);
}

void InputManagerView::onShow()
{
	BaseMenuView::onShow();
	deleteDeviceConfig.active = savedInputDevList.size;
	deleteProfile.active = customKeyConfig.size;
}

void InputManagerView::deinit()
{
	BaseMenuView::deinit();
	imMenu = nullptr;
}

void InputManagerView::init(bool highlightFirst)
{
	uint i = 0;
	assert(EmuSystem::maxPlayers <= 5);
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(EmuSystem::maxPlayers > 1)
	{
		pointerInput.init("On-screen Input Player", &pointerInputPlayer); item[i++] = &pointerInput;
	}
	#endif
	deleteDeviceConfig.init((bool)savedInputDevList.size); item[i++] = &deleteDeviceConfig;
	deleteDeviceConfig.onSelect() = [this](TextMenuItem &item, const Input::Event &e){deleteDeviceConfigHandler(item,e);};
	deleteProfile.init((bool)customKeyConfig.size); item[i++] = &deleteProfile;
	deleteProfile.onSelect() = [this](TextMenuItem &item, const Input::Event &e){deleteProfileHandler(item,e);};
	identDevice.init(); item[i++] = &identDevice;
	identDevice.onSelect() = [this](TextMenuItem &item, const Input::Event &e){identDeviceHandler(item,e);};
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	if(!optionNotifyInputDeviceChange.isConst)
	{
		notifyDeviceChange.init(optionNotifyInputDeviceChange); item[i++] = &notifyDeviceChange;
		notifyDeviceChange.onSelect() =
			[this](BoolMenuItem &item, const Input::Event &e)
			{
				item.toggle();
				optionNotifyInputDeviceChange = item.on;
			};
	}
	#endif
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12)
	{
		rescanOSDevices.init(); item[i++] = &rescanOSDevices;
		rescanOSDevices.onSelect() =
			[this](TextMenuItem &item, const Input::Event &e)
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
			};
	}
	#endif
	int devs = 0;
	forEachInDLList(&Input::devList, e)
	{
		printfDeviceNameWithNumber(inputDevNameStr[devs], e.name(), e.devId);
		inputDevName[devs].init(inputDevNameStr[devs]); item[i++] = &inputDevName[devs];
		inputDevName[devs].onSelect() =
			[this, devs](TextMenuItem &, const Input::Event &e)
			{
				auto &imdMenu = *menuAllocator.allocNew<InputManagerDeviceView>();
				imdMenu.init(!e.isPointer(), inputDevConf[devs]);
				imdMenu.name_ = inputDevNameStr[devs];
				viewStack.pushAndShow(&imdMenu, &menuAllocator);
			};
		devs++;
	}
	BaseMenuView::init(item, i, highlightFirst);
}

void InputManagerView::identDeviceHandler(TextMenuItem &, const Input::Event &e)
{
	auto &identView = *allocModalView<IdentInputDeviceView>();
	identView.init();
	identView.onIdentInput =
		[this](const Input::Event &e)
		{
			auto dev = e.device;
			if(dev)
			{
				auto &imdMenu = *menuAllocator.allocNew<InputManagerDeviceView>();
				imdMenu.init(true, inputDevConf[dev->idx]);
				imdMenu.name_ = inputDevNameStr[dev->idx];
				viewStack.pushAndShow(&imdMenu, &menuAllocator);
			}
		};
	View::addModalView(identView);
}

class ProfileSelectMenu : public BaseMultiChoiceView
{
public:
	constexpr ProfileSelectMenu() { }
	TextMenuItem choiceEntry[MAX_DEFAULT_KEY_CONFIGS_PER_TYPE + MAX_CUSTOM_KEY_CONFIGS];
	MenuItem *choiceEntryItem[sizeofArrayConst(choiceEntry)] {nullptr};
	KeyConfig *customConfig[MAX_CUSTOM_KEY_CONFIGS] {nullptr};
	typedef DelegateFunc<void (const KeyConfig &profile)> ProfileChangeDelegate;
	ProfileChangeDelegate onProfileChange;
	Input::Device *dev = nullptr;

	void init(bool highlightFirst, Input::Device &dev)
	{
		this->dev = &dev;
		int customConfigs = 0;
		uint i = 0;
		forEachInDLList(&customKeyConfig, e)
		{
			if(e.map == dev.map())
			{
				choiceEntry[i].init(e.name); choiceEntryItem[i] = &choiceEntry[i];
				choiceEntry[i].onSelect() =
					[this, customConfigs](TextMenuItem &, const Input::Event &e)
					{
						removeModalView();
						onProfileChange(*customConfig[customConfigs]);
					};
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
			choiceEntry[i].onSelect() =
				[this, c](TextMenuItem &, const Input::Event &e)
				{
					removeModalView();
					onProfileChange(KeyConfig::defaultConfigsForDevice(*this->dev)[c]);
				};
			i++;
		}
		BaseMenuView::init(choiceEntryItem, i, highlightFirst, C2DO);
	}
};

void InputManagerDeviceView::loadProfileHandler(TextMenuItem &, const Input::Event &e)
{
	auto &profileSelectMenu = *allocModalView<ProfileSelectMenu>();
	profileSelectMenu.init(!e.isPointer(), *devConf->dev);
	profileSelectMenu.placeRect(Gfx::viewportRect());
	profileSelectMenu.onProfileChange =
		[this](const KeyConfig &profile)
		{
			logMsg("set key profile %s", profile.name);
			devConf->setKeyConf(profile);
			onShow();
			keyMapping.buildAll();
		};
	View::addModalView(profileSelectMenu);
}

/*void InputManagerDeviceView::enabledHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	devConf->enabled = item.on;
	devConf->save();
	onShow();
	keyMapping.buildAll();
}*/

void InputManagerDeviceView::deleteProfileHandler(TextMenuItem &, const Input::Event &e)
{
	if(!devConf->mutableKeyConf())
	{
		popup.post("Can't delete a built-in profile", 2, 0);
		return;
	}
	auto &ynAlertView = *allocModalView<YesNoAlertView>();
	ynAlertView.init(confirmDeleteProfileStr, !e.isPointer());
	ynAlertView.onYes() =
		[this](const Input::Event &e)
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
		};
	View::addModalView(ynAlertView);
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
		auto devConfBackup = devConf;
		viewStack.pop();
		// TODO: refresh in-place
		auto &imdMenu = *menuAllocator.allocNew<InputManagerDeviceView>();
		imdMenu.init(0, *devConfBackup);
		viewStack.pushAndShow(&imdMenu, &menuAllocator);
	}
	else
		onShow();
	keyMapping.buildAll();
}

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode(const Input::Event &e)
{
	iCadeMode.toggle();
	devConf->setICadeMode(iCadeMode.on);
	onShow();
	physicalControlsPresent = EmuControls::keyInputIsPresent();
	EmuControls::updateAutoOnScreenControlVisible();
	keyMapping.buildAll();
}

void InputManagerDeviceView::iCadeModeHandler(BoolMenuItem &item, const Input::Event &e)
{
	#ifdef CONFIG_BASE_IOS
		confirmICadeMode(e);
	#else
		if(!item.on)
		{
			auto &ynAlertView = *allocModalView<YesNoAlertView>();
			ynAlertView.init("This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", !e.isPointer(), "Enable", "Cancel");
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
					confirmICadeMode(e);
				};
			View::addModalView(ynAlertView);
		}
		else
			confirmICadeMode(e);
	#endif
}
#endif

void InputManagerDeviceView::renameProfileHandler(TextMenuItem &item, const Input::Event &e)
{
	if(!devConf->mutableKeyConf())
	{
		popup.post("Can't rename a built-in profile", 2, 0);
		Base::displayNeedsUpdate();
		return;
	}
	auto &textInputView = *allocModalView<CollectTextInputView>();
	textInputView.init("Input name", devConf->keyConf().name);
	textInputView.onText() =
		[this](const char *str)
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
		};
	View::addModalView(textInputView);
}

void InputManagerDeviceView::newProfileHandler(TextMenuItem &item, const Input::Event &e)
{
	if(customKeyConfig.isFull())
	{
		popup.postError("No space left for new key profiles, please delete one");
		Base::displayNeedsUpdate();
		return;
	}
	auto &ynAlertView = *allocModalView<YesNoAlertView>();
	ynAlertView.init("Create a new profile? All keys from the current profile will be copied over.", !e.isPointer());
	ynAlertView.onYes() =
		[this](const Input::Event &e)
		{
			auto &textInputView = *allocModalView<CollectTextInputView>();
			textInputView.init("Input name", "");
			textInputView.onText() =
				[this](const char *str)
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
				};
			View::addModalView(textInputView);
		};
	View::addModalView(ynAlertView);
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
		player.onValue() = [this](MultiChoiceMenuItem &item, int val){playerHandler(item,val);};
	}
	//enabled.init((bool)devConf.enabled); item[i++] = &enabled;
	//enabled.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::enabledHandler>(this);
	/*deleteDeviceConfig.init((bool)devConf.savedConf); item[i++] = &deleteDeviceConfig;
	deleteDeviceConfig.selectDelegate().bind<InputManagerDeviceView, &InputManagerDeviceView::deleteDeviceConfigHandler>(this);*/
	string_printf(profileStr, "Profile: %s", devConf.keyConf().name);
	loadProfile.init(profileStr); item[i++] = &loadProfile;
	loadProfile.onSelect() = [this](TextMenuItem &item, const Input::Event &e){loadProfileHandler(item,e);};
	forEachInArray(EmuControls::category, c)
	{
		if(EmuControls::category[c_i].isMultiplayer && devConf.player != InputDeviceConfig::PLAYER_MULTI)
		{
			//logMsg("skipping category %s (%d)", EmuControls::category[c_i].name, (int)c_i);
			continue;
		}
		inputCategory[c_i].init(c->name); item[i++] = &inputCategory[c_i];
		inputCategory[c_i].onSelect() =
			[this, c_i](TextMenuItem &item, const Input::Event &e)
			{
				auto &bcMenu = *menuAllocator.allocNew<ButtonConfigView>();
				bcMenu.init(&EmuControls::category[c_i], *this->devConf, !e.isPointer());
				viewStack.pushAndShow(&bcMenu, &menuAllocator);
			};
	}
	newProfile.init(); item[i++] = &newProfile;
	newProfile.onSelect() = [this](TextMenuItem &item, const Input::Event &e){newProfileHandler(item,e);};
	renameProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &renameProfile;
	renameProfile.onSelect() = [this](TextMenuItem &item, const Input::Event &e){renameProfileHandler(item,e);};
	deleteProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &deleteProfile;
	deleteProfile.onSelect() = [this](TextMenuItem &item, const Input::Event &e){deleteProfileHandler(item,e);};
	#if defined CONFIG_INPUT_ICADE
	if((devConf.dev->map() == Input::Event::MAP_KEYBOARD && devConf.dev->hasKeyboard())
			|| devConf.dev->map() == Input::Event::MAP_ICADE)
	{
		iCadeMode.init(devConf.iCadeMode()); item[i++] = &iCadeMode;
		iCadeMode.onSelect() = [this](BoolMenuItem &item, const Input::Event &e){iCadeModeHandler(item,e);};
	}
	#endif
	if(devConf.dev->hasJoystick())
	{
		joystickAxis1DPad.init(devConf.mapJoystickAxis1ToDpad()); item[i++] = &joystickAxis1DPad;
		joystickAxis1DPad.onSelect() = [this](BoolMenuItem &item, const Input::Event &e){joystickAxis1DPadHandler(item,e);};
	}
	BaseMenuView::init(item, i, highlightFirst);
}
