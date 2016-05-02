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

#include <emuframework/InputManagerView.hh>
#include <emuframework/ButtonConfigView.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/base/Base.hh>
static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

void IdentInputDeviceView::init()
{
	text = {"Push a key on any input device enter its configuration menu", View::defaultFace};
	Input::setHandleVolumeKeys(true);
	Input::setKeyRepeat(false);
}

IdentInputDeviceView::~IdentInputDeviceView()
{
	Input::setHandleVolumeKeys(false);
	Input::setKeyRepeat(true);
}

void IdentInputDeviceView::place()
{
	text.maxLineSize = projP.w * 0.95;
	text.compile(projP);
}

void IdentInputDeviceView::inputEvent(Input::Event e)
{
	if(e.isPointer() && e.state == Input::RELEASED)
	{
		dismiss();
	}
	else if(!e.isPointer() && e.state == Input::PUSHED)
	{
		auto del = onIdentInput;
		dismiss();
		del(e);
	}
}

void IdentInputDeviceView::draw()
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(projP.makeTranslate());
	setColor(.4, .4, .4, 1.);
	GeomRect::draw(viewFrame, projP);
	setColor(COLOR_WHITE);
	texAlphaProgram.use();
	text.draw(0, 0, C2DO, projP);
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
	TableView{"Key/Gamepad Input Setup", win, item},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!savedInputDevList.size())
			{
				popup.post("No saved device settings");
				return;
			}
			uint devs = 0;
			for(auto &e : savedInputDevList)
			{
				printfDeviceNameWithNumber(deviceConfigStr[devs++], e.name, e.enumId);
			}
			auto &multiChoiceView = *new TextTableView{item.t.str, window(), devs};
			iterateTimes(devs, i)
			{
				multiChoiceView.appendItem(deviceConfigStr[i],
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						pop();
						int deleteDeviceConfigIdx = i;
						auto &ynAlertView = *new YesNoAlertView{window(), confirmDeleteDeviceSettingsStr};
						ynAlertView.setOnYes(
							[this, deleteDeviceConfigIdx](TextMenuItem &, View &view, Input::Event e)
							{
								view.dismiss();
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
							});
						modalViewController.pushAndShow(ynAlertView, e);
					});
			}
			pushAndShow(multiChoiceView, e);
		}
	},
	deleteProfile
	{
		"Delete Saved Key Profile",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!customKeyConfig.size())
			{
				popup.post("No saved profiles");
				return;
			}
			uint profiles = 0;
			for(auto &e : customKeyConfig)
			{
				profileStr[profiles++] = e.name;
			}
			auto &multiChoiceView = *new TextTableView{item.t.str, window(), profiles};
			iterateTimes(profiles, i)
			{
				multiChoiceView.appendItem(profileStr[i],
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						pop();
						int deleteProfileIdx = i;
						auto &ynAlertView = *new YesNoAlertView{window(), confirmDeleteProfileStr};
						ynAlertView.setOnYes(
							[this, deleteProfileIdx](TextMenuItem &, View &view, Input::Event e)
							{
								view.dismiss();
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
							});
						modalViewController.pushAndShow(ynAlertView, e);
					});
			}
			pushAndShow(multiChoiceView, e);
		}
	},
	#ifdef CONFIG_BASE_ANDROID
	rescanOSDevices
	{
		"Re-scan OS Input Devices",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			using namespace Input;
			Input::devicesChanged();
			uint devices = 0;
			for(auto &e : Input::devList)
			{
				if(e->map() == Event::MAP_SYSTEM || e->map() == Event::MAP_ICADE)
					devices++;
			}
			popup.printf(2, 0, "%d OS devices present", devices);
		}
	},
	#endif
	identDevice
	{
		"Auto-detect Device To Setup",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &identView = *new IdentInputDeviceView{window()};
			identView.init();
			identView.onIdentInput =
				[this](Input::Event e)
				{
					auto dev = e.device;
					if(dev)
					{
						auto &imdMenu = *new InputManagerDeviceView{window(), *this, inputDevConf[dev->idx]};
						imdMenu.setName(inputDevNameStr[dev->idx]);
						pushAndShow(imdMenu, e);
					}
				};
			modalViewController.pushAndShow(identView, e);
		}
	},
	generalOptions
	{
		"General Options",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &optView = *new InputManagerOptionsView{window()};
			pushAndShow(optView, e);
		}
	},
	systemOptions
	{
		"Emulated System Options",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &optView = *EmuSystem::makeView(window(), EmuSystem::ViewID::INPUT_OPTIONS);
			pushAndShow(optView, e);
		}
	},
	deviceListHeading
	{
		"Individual Device Settings"
	}
{
	assert(!onUpdateInputDevices);
	onUpdateInputDevices =
		[this]()
		{
			if(modalViewController.hasView())
				modalViewController.pop();
			viewStack.popTo(*this);
			auto selectedCell = selected;
			loadItems();
			highlightCell(selectedCell);
			place();
			show();
		};
	deleteDeviceConfig.setActive(savedInputDevList.size());
	deleteProfile.setActive(customKeyConfig.size());
	loadItems();
}

void InputManagerView::loadItems()
{
	item.clear();
	item.emplace_back(&identDevice);
	item.emplace_back(&generalOptions);
	if(EmuSystem::hasInputOptions())
	{
		item.emplace_back(&systemOptions);
	}
	item.emplace_back(&deleteDeviceConfig);
	item.emplace_back(&deleteProfile);
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12 && Base::androidSDK() < 16)
	{
		item.emplace_back(&rescanOSDevices);
	}
	#endif
	item.emplace_back(&deviceListHeading);
	int devs = 0;
	for(auto &e : Input::devList)
	{
		printfDeviceNameWithNumber(inputDevNameStr[devs], e->name(), e->enumId());
		inputDevName[devs] = {inputDevNameStr[devs],
			[this, devs](TextMenuItem &, View &, Input::Event e)
			{
				auto &imdMenu = *new InputManagerDeviceView{window(), *this, inputDevConf[devs]};
				imdMenu.setName(inputDevNameStr[devs]);
				pushAndShow(imdMenu, e);
			}};
		item.emplace_back(&inputDevName[devs]);
		devs++;
	}
}


void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.setActive(savedInputDevList.size());
	deleteProfile.setActive(customKeyConfig.size());
}

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
static void setBTScanSecs(int secs)
{
	BluetoothAdapter::scanSecs = secs;
	logMsg("set bluetooth scan time %d", BluetoothAdapter::scanSecs);
}
#endif

InputManagerOptionsView::InputManagerOptionsView(Base::Window &win):
	TableView{"General Input Options", win, item},
	#ifdef CONFIG_BASE_ANDROID
	relativePointerDecelItem
	{
		{
			"Low",
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelLow;
			}
		},
		{
			"Med.",
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelMed;
			}
		},
		{
			"High",
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelHigh;
			}
		}
	},
	relativePointerDecel
	{
		"Trackball Sensitivity",
		[]() -> uint
		{
			if(optionRelPointerDecel == optionRelPointerDecelLow)
				return 0;
			if(optionRelPointerDecel == optionRelPointerDecelMed)
				return 1;
			if(optionRelPointerDecel == optionRelPointerDecelHigh)
				return 2;
			return 0;
		}(),
		relativePointerDecelItem
	},
	#endif
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	mogaInputSystem
	{
		"MOGA Controller Support",
		optionMOGAInputSystem,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			if(!optionMOGAInputSystem && !Base::packageIsInstalled("com.bda.pivot.mogapgp"))
			{
				popup.post("Install the MOGA Pivot app from Google Play to use your MOGA Pocket. "
					"For MOGA Pro or newer, set switch to mode B and pair in the Android Bluetooth settings app instead.", 8);
				return;
			}
			optionMOGAInputSystem = item.flipBoolValue(*this);
			if(optionMOGAInputSystem)
				Input::initMOGA(true);
			else
				Input::deinitMOGA();
		}
	},
	#endif
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	notifyDeviceChange
	{
		"Notify If Devices Change",
		(bool)optionNotifyInputDeviceChange,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionNotifyInputDeviceChange = item.flipBoolValue(*this);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothHeading
	{
		"In-app Bluetooth Options"
	},
	keepBtActive
	{
		"Keep Connections In Background",
		(bool)optionKeepBluetoothActive,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionKeepBluetoothActive = item.flipBoolValue(*this);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecsItem
	{
		{
			"2secs",
			[this]()
			{
				setBTScanSecs(2);
			}
		},
		{
			"4secs",
			[this]()
			{
				setBTScanSecs(4);
			}
		},
		{
			"6secs",
			[this]()
			{
				setBTScanSecs(6);
			}
		},
		{
			"8secs",
			[this]()
			{
				setBTScanSecs(8);
			}
		},
		{
			"10secs",
			[this]()
			{
				setBTScanSecs(10);
			}
		}
	},
	btScanSecs
	{
		"Scan Time",
		[]() -> uint
		{
			switch(BluetoothAdapter::scanSecs)
			{
				default: return 0;
				case 2: return 0;
				case 4: return 1;
				case 6: return 2;
				case 8: return 3;
				case 10: return 4;
			}
		}(),
		btScanSecsItem
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache
	{
		"Cache Scan Results",
		(bool)optionBlueToothScanCache,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionBlueToothScanCache = item.flipBoolValue(*this);
		}
	},
	#endif
	altGamepadConfirm
	{
		"Swap Confirm/Cancel Keys",
		Input::swappedGamepadConfirm,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			Input::swappedGamepadConfirm = item.flipBoolValue(*this);
		}
	}
{
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	item.emplace_back(&mogaInputSystem);
	#endif
	item.emplace_back(&altGamepadConfirm);
	#ifdef CONFIG_BASE_ANDROID
	if(Input::hasTrackball())
	{
		item.emplace_back(&relativePointerDecel);
	}
	#endif
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	if(!optionNotifyInputDeviceChange.isConst)
	{
		item.emplace_back(&notifyDeviceChange);
	}
	#endif
	#ifdef CONFIG_BLUETOOTH
	item.emplace_back(&bluetoothHeading);
	if(!optionKeepBluetoothActive.isConst)
	{
		item.emplace_back(&keepBtActive);
	}
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	item.emplace_back(&btScanSecs);
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	item.emplace_back(&btScanCache);
	#endif
}

class ProfileSelectMenu : public TextTableView
{
public:
	using ProfileChangeDelegate = DelegateFunc<void (const KeyConfig &profile)>;

	ProfileChangeDelegate onProfileChange{};

	ProfileSelectMenu(Base::Window &win, Input::Device &dev, const char *selectedName):
		TextTableView
		{
			"Key Profile",
			win,
			customKeyConfig.size() + MAX_DEFAULT_KEY_CONFIGS_PER_TYPE
		}
	{
		for(auto &conf : customKeyConfig)
		{
			if(conf.map == dev.map())
			{
				if(string_equal(selectedName, conf.name))
				{
					activeItem = textItem.size();
				}
				textItem.emplace_back(conf.name,
					[this, &conf](TextMenuItem &, View &, Input::Event e)
					{
						auto del = onProfileChange;
						dismiss();
						del(conf);
					});
			}
		}
		uint defaultConfs = 0;
		auto defaultConf = KeyConfig::defaultConfigsForDevice(dev, defaultConfs);
		iterateTimes(defaultConfs, c)
		{
			auto &conf = KeyConfig::defaultConfigsForDevice(dev)[c];
			if(string_equal(selectedName, defaultConf[c].name))
				activeItem = textItem.size();
			textItem.emplace_back(defaultConf[c].name,
				[this, &conf](TextMenuItem &, View &, Input::Event e)
				{
					auto del = onProfileChange;
					dismiss();
					del(conf);
				});
		}
	}
};

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

InputManagerDeviceView::InputManagerDeviceView(Base::Window &win, InputManagerView &rootIMView_, InputDeviceConfig &devConfRef):
	TableView{win, item},
	rootIMView{rootIMView_},
	playerItem
	{
		{"Multiple", [this]() { setPlayer(InputDeviceConfig::PLAYER_MULTI); }},
		{"1", [this]() { setPlayer(0); }},
		{"2", [this]() { setPlayer(1); }},
		{"3", [this]() { setPlayer(2); }},
		{"4", [this]() { setPlayer(3); }},
		{"5", [this]() { setPlayer(4); }}
	},
	player
	{
		"Player",
		playerConfToMenuIdx(devConfRef.player),
		[](const MultiChoiceMenuItem &) -> uint
		{
			return EmuSystem::maxPlayers+1;
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return playerItem[idx];
		}
	},
	loadProfile
	{
		profileStr,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &profileSelectMenu = *new ProfileSelectMenu{window(), *devConf->dev, devConf->keyConf().name};
			profileSelectMenu.onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name);
					devConf->setKeyConf(profile);
					onShow();
					keyMapping.buildAll();
				};
			pushAndShow(profileSelectMenu, e);
		}
	},
	renameProfile
	{
		"Rename Profile",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!devConf->mutableKeyConf())
			{
				popup.post("Can't rename a built-in profile", 2, 0);
				return;
			}
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input name", devConf->keyConf().name, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str && strlen(str))
					{
						if(customKeyConfigsContainName(str))
						{
							popup.postError("Another profile is already using this name");
							postDraw();
							return 1;
						}
						assert(devConf->mutableKeyConf());
						string_copy(devConf->mutableKeyConf()->name, str);
						onShow();
						postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	newProfile
	{
		"New Profile",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(customKeyConfig.isFull())
			{
				popup.postError("No space left for new key profiles, please delete one");
				return;
			}
			auto &ynAlertView = *new YesNoAlertView{window(),
				"Create a new profile? All keys from the current profile will be copied over."};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					auto &textInputView = *new CollectTextInputView{window()};
					textInputView.init("Input name", "", getCollectTextCloseAsset());
					textInputView.onText() =
						[this](CollectTextInputView &view, const char *str)
						{
							if(str && strlen(str))
							{
								if(customKeyConfigsContainName(str))
								{
									popup.postError("Another profile is already using this name");
									return 1;
								}
								if(devConf->setKeyConfCopiedFromExisting(str))
								{
									logMsg("created new profile %s", devConf->keyConf().name);
									onShow();
								}
								else
									popup.postError("Too many saved device settings, please delete one");
								postDraw();
							}
							view.dismiss();
							return 0;
						};
					modalViewController.pushAndShow(textInputView, e);
				});
			modalViewController.pushAndShow(ynAlertView, e);
		}
	},
	deleteProfile
	{
		"Delete Profile",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!devConf->mutableKeyConf())
			{
				popup.post("Can't delete a built-in profile", 2, 0);
				return;
			}
			auto &ynAlertView = *new YesNoAlertView{window(), confirmDeleteProfileStr};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
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
				});
			modalViewController.pushAndShow(ynAlertView, e);
		}
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode",
		devConfRef.iCadeMode(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			#ifdef CONFIG_BASE_IOS
			confirmICadeMode(e);
			#else
			if(!item.boolValue())
			{
				auto &ynAlertView = *new YesNoAlertView{window(),
					"This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", "Enable", "Cancel"};
				ynAlertView.setOnYes(
					[this](TextMenuItem &, View &view, Input::Event e)
					{
						confirmICadeMode(e);
					});
				modalViewController.pushAndShow(ynAlertView, e);
			}
			else
				confirmICadeMode(e);
			#endif
		}
	},
	#endif
	joystickAxis1DPad
	{
		"Joystick X/Y Axis 1 as D-Pad",
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_X),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_STICK_1 : 0, Input::Device::AXIS_BITS_STICK_1);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	joystickAxis2DPad
	{
		"Joystick X/Y Axis 2 as D-Pad",
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_Z),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_STICK_2 : 0, Input::Device::AXIS_BITS_STICK_2);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	joystickAxisHatDPad
	{
		"Joystick POV Hat as D-Pad",
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_HAT_X),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_HAT : 0, Input::Device::AXIS_BITS_HAT);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	devConf{&devConfRef}
{
	string_printf(profileStr, "Profile: %s", devConf->keyConf().name);
	renameProfile.setActive(devConf->mutableKeyConf());
	deleteProfile.setActive(devConf->mutableKeyConf());
	loadItems();
}

void InputManagerDeviceView::loadItems()
{
	item.clear();
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&player);
	}
	item.emplace_back(&loadProfile);
	inputCategories = 0;
	iterateTimes(EmuControls::categories, c)
	{
		auto &cat = EmuControls::category[c];
		if(cat.isMultiplayer && devConf->player != InputDeviceConfig::PLAYER_MULTI)
		{
			//logMsg("skipping category %s (%d)", cat.name, (int)c_i);
			continue;
		}
		inputCategory[c] = {cat.name,
			[this, c](TextMenuItem &item, View &, Input::Event e)
			{
				auto &bcMenu = *new ButtonConfigView{window(), rootIMView, &EmuControls::category[c], *this->devConf};
				pushAndShow(bcMenu, e);
			}};
		item.emplace_back(&inputCategory[c]);
		inputCategories++;
	}
	item.emplace_back(&newProfile);
	item.emplace_back(&renameProfile);
	item.emplace_back(&deleteProfile);
	#if defined CONFIG_INPUT_ICADE
	if((devConf->dev->map() == Input::Event::MAP_SYSTEM && devConf->dev->hasKeyboard())
			|| devConf->dev->map() == Input::Event::MAP_ICADE)
	{
		item.emplace_back(&iCadeMode);
	}
	#endif
	if(devConf->dev->joystickAxisBits() & Input::Device::AXIS_BIT_X)
	{
		item.emplace_back(&joystickAxis1DPad);
	}
	if(devConf->dev->joystickAxisBits() & Input::Device::AXIS_BIT_Z)
	{
		item.emplace_back(&joystickAxis2DPad);
	}
	if(devConf->dev->joystickAxisBits() & Input::Device::AXIS_BIT_HAT_X)
	{
		item.emplace_back(&joystickAxisHatDPad);
	}
}

void InputManagerDeviceView::onShow()
{
	TableView::onShow();
	string_printf(profileStr, "Profile: %s", devConf->keyConf().name);
	loadProfile.compile(projP);
	bool keyConfIsMutable = devConf->mutableKeyConf();
	renameProfile.setActive(keyConfIsMutable);
	deleteProfile.setActive(keyConfIsMutable);
}

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode(Input::Event e)
{
	devConf->setICadeMode(iCadeMode.flipBoolValue(*this));
	onShow();
	physicalControlsPresent = Input::keyInputIsPresent();
	EmuControls::updateAutoOnScreenControlVisible();
	keyMapping.buildAll();
}
#endif

void InputManagerDeviceView::setPlayer(int playerVal)
{
	bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player != InputDeviceConfig::PLAYER_MULTI) ||
		(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player == InputDeviceConfig::PLAYER_MULTI);
	devConf->player = playerVal;
	devConf->save();
	if(changingMultiplayer)
	{
		loadItems();
		place();
		show();
	}
	else
		onShow();
	keyMapping.buildAll();
}
