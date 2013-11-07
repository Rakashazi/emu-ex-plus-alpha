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
	for(auto &e : savedInputDevList)
	{
		if(e.keyConf == conf)
		{
			logMsg("used by saved device config %s,%d", e.name, e.enumId);
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

InputManagerView::InputManagerView(Base::Window &win):
	BaseMenuView("Key/Gamepad Input Setup", win),
	deleteDeviceConfig
	{
		"Delete Saved Device Settings",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!savedInputDevList.size())
			{
				popup.post("No saved device settings");
				return;
			}
			int devs = 0;
			for(auto &e : savedInputDevList)
			{
				printfDeviceNameWithNumber(deviceConfigStr[devs++], e.name, e.enumId);
			}
			auto &multiChoiceView = *menuAllocator.allocNew<MultiChoiceView>(item.t.str, window());
			multiChoiceView.init(deviceConfigStr, devs, !e.isPointer());
			multiChoiceView.onSelect() =
				[this](int i, const Input::Event &e)
				{
					viewStack.popAndShow();
					int deleteDeviceConfigIdx = i;
					auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
					ynAlertView.init(confirmDeleteDeviceSettingsStr, !e.isPointer());
					ynAlertView.onYes() =
						[this, deleteDeviceConfigIdx](const Input::Event &e)
						{
							auto it = savedInputDevList.begin();
							iterateTimes(deleteDeviceConfigIdx, i)
							{
								++it;
							}
							logMsg("deleting device settings for: %s,%d", it->name, it->enumId);
							iterateTimes(inputDevConfs, i)
							{
								if(inputDevConf[i].savedConf == &(*it))
								{
									logMsg("removing from active device at idx: %d", i);
									inputDevConf[i].savedConf = nullptr;
									break;
								}
							}
							savedInputDevList.erase(it);
							keyMapping.buildAll();
							onShow();
						};
					View::addModalView(ynAlertView);
					return 0;
				};
			viewStack.pushAndShow(&multiChoiceView, &menuAllocator);
		}
	},
	deleteProfile
	{
		"Delete Saved Key Profile",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!customKeyConfig.size())
			{
				popup.post("No saved profiles");
				return;
			}
			int profiles = 0;
			for(auto &e : customKeyConfig)
			{
				profileStr[profiles++] = e.name;
			}
			auto &multiChoiceView = *menuAllocator.allocNew<MultiChoiceView>(item.t.str, window());
			multiChoiceView.init(profileStr, profiles, !e.isPointer());
			multiChoiceView.onSelect() =
				[this](int i, const Input::Event &e)
				{
					viewStack.popAndShow();
					int deleteProfileIdx = i;
					auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
					ynAlertView.init(confirmDeleteProfileStr, !e.isPointer());
					ynAlertView.onYes() =
						[this, deleteProfileIdx](const Input::Event &e)
						{
							auto it = customKeyConfig.begin();
							iterateTimes(deleteProfileIdx, i)
							{
								++it;
							}
							logMsg("deleting profile: %s", it->name);
							removeKeyConfFromAllDevices(&(*it));
							customKeyConfig.erase(it);
							keyMapping.buildAll();
							onShow();
						};
					View::addModalView(ynAlertView);
					return 0;
				};
			viewStack.pushAndShow(&multiChoiceView, &menuAllocator);
		}
	},
	notifyDeviceChange
	{
		"Notify If Devices Change",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			optionNotifyInputDeviceChange = item.on;
		}
	},
	#ifdef CONFIG_BASE_ANDROID
	rescanOSDevices
	{
		"Re-scan OS Input Devices",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			using namespace Input;
			Input::devicesChanged();
			uint devices = 0;
			for(auto &e : Input::devList)
			{
				if(e->map() == Event::MAP_KEYBOARD || e->map() == Event::MAP_ICADE)
					devices++;
			}
			popup.printf(2, 0, "%d OS devices present", devices);
		}
	},
	#endif
	identDevice
	{
		"Auto-detect Device To Setup",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			auto &identView = *allocModalView<IdentInputDeviceView>(window());
			identView.init();
			identView.onIdentInput =
				[this](const Input::Event &e)
				{
					auto dev = e.device;
					if(dev)
					{
						auto &imdMenu = *menuAllocator.allocNew<InputManagerDeviceView>(window());
						imdMenu.init(true, inputDevConf[dev->idx]);
						imdMenu.name_ = inputDevNameStr[dev->idx];
						viewStack.pushAndShow(&imdMenu, &menuAllocator);
					}
				};
			View::addModalView(identView);
		}
	}
{}

void InputManagerView::onShow()
{
	BaseMenuView::onShow();
	deleteDeviceConfig.active = savedInputDevList.size();
	deleteProfile.active = customKeyConfig.size();
}

void InputManagerView::deinit()
{
	BaseMenuView::deinit();
	imMenu = nullptr;
}

void InputManagerView::init(bool highlightFirst)
{
	imMenu = this;
	uint i = 0;
	assert(EmuSystem::maxPlayers <= 5);
	deleteDeviceConfig.init((bool)savedInputDevList.size()); item[i++] = &deleteDeviceConfig;
	deleteProfile.init((bool)customKeyConfig.size()); item[i++] = &deleteProfile;
	identDevice.init(); item[i++] = &identDevice;
	#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
	if(!optionNotifyInputDeviceChange.isConst)
	{
		notifyDeviceChange.init(optionNotifyInputDeviceChange); item[i++] = &notifyDeviceChange;
	}
	#endif
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12 && Base::androidSDK() < 16)
	{
		rescanOSDevices.init(); item[i++] = &rescanOSDevices;
	}
	#endif
	int devs = 0;
	for(auto &e : Input::devList)
	{
		printfDeviceNameWithNumber(inputDevNameStr[devs], e->name(), e->enumId());
		inputDevName[devs].init(inputDevNameStr[devs]); item[i++] = &inputDevName[devs];
		inputDevName[devs].onSelect() =
			[this, devs](TextMenuItem &, const Input::Event &e)
			{
				auto &imdMenu = *menuAllocator.allocNew<InputManagerDeviceView>(window());
				imdMenu.init(!e.isPointer(), inputDevConf[devs]);
				imdMenu.name_ = inputDevNameStr[devs];
				viewStack.pushAndShow(&imdMenu, &menuAllocator);
			};
		devs++;
	}
	BaseMenuView::init(item, i, highlightFirst);
}

class ProfileSelectMenu : public BaseMenuView
{
public:
	constexpr ProfileSelectMenu(Base::Window &win): BaseMenuView("Key Profile", win) {}
	TextMenuItem choiceEntry[MAX_DEFAULT_KEY_CONFIGS_PER_TYPE + MAX_CUSTOM_KEY_CONFIGS];
	MenuItem *choiceEntryItem[sizeofArrayConst(choiceEntry)] {nullptr};
	typedef DelegateFunc<void (const KeyConfig &profile)> ProfileChangeDelegate;
	ProfileChangeDelegate onProfileChange;
	int activeItem = -1;

	void init(bool highlightFirst, Input::Device &dev, const char *selectedName)
	{
		uint i = 0;
		for(auto &conf : customKeyConfig)
		{
			if(conf.map == dev.map())
			{
				if(string_equal(selectedName, conf.name))
				{
					activeItem = i;
				}
				choiceEntry[i].init(conf.name); choiceEntryItem[i] = &choiceEntry[i];
				choiceEntry[i].onSelect() =
					[this, &conf](TextMenuItem &, const Input::Event &e)
					{
						viewStack.popAndShow();
						onProfileChange(conf);
					};
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
			auto &conf = KeyConfig::defaultConfigsForDevice(dev)[c];
			if(string_equal(selectedName, defaultConf[c].name))
				activeItem = i;
			choiceEntry[i].init(defaultConf[c].name); choiceEntryItem[i] = &choiceEntry[i];
			choiceEntry[i].onSelect() =
				[this, &conf](TextMenuItem &, const Input::Event &e)
				{
					viewStack.popAndShow();
					onProfileChange(conf);
				};
			i++;
		}
		BaseMenuView::init(choiceEntryItem, i, highlightFirst);
		if(highlightFirst && activeItem != -1)
		{
			tbl.selected = activeItem;
		}
	}

	void drawElement(const GuiTable1D *table, uint i, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override
	{
		using namespace Gfx;
		if((int)i == activeItem)
			setColor(0., .8, 1.);
		else
			setColor(COLOR_WHITE);
		item[i]->draw(xPos, yPos, xSize, ySize, align);
	}
};

InputManagerDeviceView::InputManagerDeviceView(Base::Window &win):
	BaseMenuView(win),
	player
	{
		"Player",
		[this](MultiChoiceMenuItem &, int val)
		{
			uint playerVal = val == 0 ? InputDeviceConfig::PLAYER_MULTI : val-1;
			bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player != InputDeviceConfig::PLAYER_MULTI) ||
					(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player == InputDeviceConfig::PLAYER_MULTI);
			devConf->player = playerVal;
			devConf->save();
			if(changingMultiplayer)
			{
				auto devConfBackup = devConf;
				deinit();
				init(false, *devConfBackup);
				place();
				show();
			}
			else
				onShow();
			keyMapping.buildAll();
		}
	},
	loadProfile
	{
		[this](TextMenuItem &item, const Input::Event &e)
		{
			auto &profileSelectMenu = *menuAllocator.allocNew<ProfileSelectMenu>(window());
			profileSelectMenu.init(!e.isPointer(), *devConf->dev, devConf->keyConf().name);
			profileSelectMenu.onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name);
					devConf->setKeyConf(profile);
					onShow();
					keyMapping.buildAll();
				};
			viewStack.pushAndShow(&profileSelectMenu, &menuAllocator);
		}
	},
	renameProfile
	{
		"Rename Profile",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!devConf->mutableKeyConf())
			{
				popup.post("Can't rename a built-in profile", 2, 0);
				displayNeedsUpdate();
				return;
			}
			auto &textInputView = *allocModalView<CollectTextInputView>(window());
			textInputView.init("Input name", devConf->keyConf().name);
			textInputView.onText() =
				[this](const char *str)
				{
					if(str && strlen(str))
					{
						if(customKeyConfigsContainName(str))
						{
							popup.postError("Another profile is already using this name");
							displayNeedsUpdate();
							return 1;
						}
						assert(devConf->mutableKeyConf());
						string_copy(devConf->mutableKeyConf()->name, str);
						onShow();
						displayNeedsUpdate();
					}
					removeModalView();
					return 0;
				};
			View::addModalView(textInputView);
		}
	},
	newProfile
	{
		"New Profile",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(customKeyConfig.isFull())
			{
				popup.postError("No space left for new key profiles, please delete one");
				displayNeedsUpdate();
				return;
			}
			auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
			ynAlertView.init("Create a new profile? All keys from the current profile will be copied over.", !e.isPointer());
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
					auto &textInputView = *allocModalView<CollectTextInputView>(window());
					textInputView.init("Input name", "");
					textInputView.onText() =
						[this](const char *str)
						{
							if(str && strlen(str))
							{
								if(customKeyConfigsContainName(str))
								{
									popup.postError("Another profile is already using this name");
									displayNeedsUpdate();
									return 1;
								}
								if(devConf->setKeyConfCopiedFromExisting(str))
								{
									logMsg("created new profile %s", devConf->keyConf().name);
									onShow();
								}
								else
									popup.postError("Too many saved device settings, please delete one");
								displayNeedsUpdate();
							}
							removeModalView();
							return 0;
						};
					View::addModalView(textInputView);
				};
			View::addModalView(ynAlertView);
		}
	},
	deleteProfile
	{
		"Delete Profile",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!devConf->mutableKeyConf())
			{
				popup.post("Can't delete a built-in profile", 2, 0);
				return;
			}
			auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
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
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			#ifdef CONFIG_BASE_IOS
			confirmICadeMode(e);
			#else
			if(!item.on)
			{
				auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
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
	},
	#endif
	joystickAxis1DPad
	{
		"Joystick Axis 1 as D-Pad",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			devConf->setMapJoystickAxis1ToDpad(item.on);
			devConf->save();
		}
	}
{}

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

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode(const Input::Event &e)
{
	iCadeMode.toggle(*this);
	devConf->setICadeMode(iCadeMode.on);
	onShow();
	physicalControlsPresent = EmuControls::keyInputIsPresent();
	EmuControls::updateAutoOnScreenControlVisible();
	keyMapping.buildAll();
}
#endif

void InputManagerDeviceView::init(bool highlightFirst, InputDeviceConfig &devConf)
{
	this->devConf = &devConf;
	uint i = 0;
	if(EmuSystem::maxPlayers > 1)
	{
		static const char *str[] = { "Multiple", "1", "2", "3", "4", "5" };
		player.init(str, playerConfToMenuIdx(devConf.player), EmuSystem::maxPlayers+1); item[i++] = &player;
	}
	string_printf(profileStr, "Profile: %s", devConf.keyConf().name);
	loadProfile.init(profileStr); item[i++] = &loadProfile;
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
				auto &bcMenu = *menuAllocator.allocNew<ButtonConfigView>(window());
				bcMenu.init(&EmuControls::category[c_i], *this->devConf, !e.isPointer());
				viewStack.pushAndShow(&bcMenu, &menuAllocator);
			};
	}
	newProfile.init(); item[i++] = &newProfile;
	renameProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &renameProfile;
	deleteProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &deleteProfile;
	#if defined CONFIG_INPUT_ICADE
	if((devConf.dev->map() == Input::Event::MAP_KEYBOARD && devConf.dev->hasKeyboard())
			|| devConf.dev->map() == Input::Event::MAP_ICADE)
	{
		iCadeMode.init(devConf.iCadeMode()); item[i++] = &iCadeMode;
	}
	#endif
	if(devConf.dev->hasJoystick())
	{
		joystickAxis1DPad.init(devConf.mapJoystickAxis1ToDpad()); item[i++] = &joystickAxis1DPad;
	}
	BaseMenuView::init(item, i, highlightFirst);
}
