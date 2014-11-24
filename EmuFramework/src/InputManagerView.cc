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
#include <emuframework/TextEntry.hh>
#include <imagine/base/Base.hh>
static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

void IdentInputDeviceView::init()
{
	text.init("Push a key on any input device enter its configuration menu", View::defaultFace);
	Input::setHandleVolumeKeys(true);
	Input::setKeyRepeat(false);
}

void IdentInputDeviceView::deinit()
{
	text.deinit();
	Input::setHandleVolumeKeys(false);
	Input::setKeyRepeat(true);
}

void IdentInputDeviceView::place()
{
	text.maxLineSize = projP.w * 0.95;
	text.compile(projP);
}

void IdentInputDeviceView::inputEvent(const Input::Event &e)
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
	TableView{"Key/Gamepad Input Setup", win},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings",
		[this](TextMenuItem &item, View &, const Input::Event &e)
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
			auto &multiChoiceView = *new MultiChoiceView{item.t.str, window()};
			multiChoiceView.init(devs, !e.isPointer());
			iterateTimes(devs, i)
			{
				multiChoiceView.setItem(i, deviceConfigStr[i],
					[this, i](TextMenuItem &, View &, const Input::Event &e)
					{
						pop();
						int deleteDeviceConfigIdx = i;
						auto &ynAlertView = *new YesNoAlertView{window()};
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
						modalViewController.pushAndShow(ynAlertView);
					});
			}
			pushAndShow(multiChoiceView);
		}
	},
	deleteProfile
	{
		"Delete Saved Key Profile",
		[this](TextMenuItem &item, View &, const Input::Event &e)
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
			auto &multiChoiceView = *new MultiChoiceView{item.t.str, window()};
			multiChoiceView.init(profiles, !e.isPointer());
			iterateTimes(profiles, i)
			{
				multiChoiceView.setItem(i, profileStr[i],
					[this, i](TextMenuItem &, View &, const Input::Event &e)
					{
						pop();
						int deleteProfileIdx = i;
						auto &ynAlertView = *new YesNoAlertView{window()};
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
						modalViewController.pushAndShow(ynAlertView);
					});
			}
			pushAndShow(multiChoiceView);
		}
	},
	#ifdef CONFIG_BASE_ANDROID
	rescanOSDevices
	{
		"Re-scan OS Input Devices",
		[this](TextMenuItem &item, View &, const Input::Event &e)
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
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &identView = *new IdentInputDeviceView{window()};
			identView.init();
			identView.onIdentInput =
				[this](const Input::Event &e)
				{
					auto dev = e.device;
					if(dev)
					{
						auto &imdMenu = *new InputManagerDeviceView{window(), *this};
						imdMenu.init(true, inputDevConf[dev->idx]);
						imdMenu.name_ = inputDevNameStr[dev->idx];
						pushAndShow(imdMenu);
					}
				};
			modalViewController.pushAndShow(identView);
		}
	},
	generalOptions
	{
		"General Options",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &optView = *new InputManagerOptionsView{window()};
			optView.init(!e.isPointer());
			pushAndShow(optView);
		}
	},
	systemOptions
	{
		"Emulated System Options",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &optView = *makeOptionCategoryMenu(window(), e, 2);
			pushAndShow(optView);
		}
	},
	deviceListHeading
	{
		"Individual Device Settings"
	}
{}

void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.active = savedInputDevList.size();
	deleteProfile.active = customKeyConfig.size();
}

void InputManagerView::deinit()
{
	TableView::deinit();
	assert(onUpdateInputDevices);
	onUpdateInputDevices = {};
}

void InputManagerView::init(bool highlightFirst)
{
	assert(!onUpdateInputDevices);
	onUpdateInputDevices =
		[this]()
		{
			if(modalViewController.hasView())
				modalViewController.pop();
			viewStack.popTo(*this);
			deinit();
			init(false);
			place();
			show();
		};
	uint i = 0;
	assert(EmuSystem::maxPlayers <= 5);
	identDevice.init(); item[i++] = &identDevice;
	generalOptions.init(); item[i++] = &generalOptions;
	if(EmuSystem::hasInputOptions())
	{
		systemOptions.init(); item[i++] = &systemOptions;
	}
	deleteDeviceConfig.init((bool)savedInputDevList.size()); item[i++] = &deleteDeviceConfig;
	deleteProfile.init((bool)customKeyConfig.size()); item[i++] = &deleteProfile;
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12 && Base::androidSDK() < 16)
	{
		rescanOSDevices.init(); item[i++] = &rescanOSDevices;
	}
	#endif
	deviceListHeading.init(); item[i++] = &deviceListHeading;
	int devs = 0;
	for(auto &e : Input::devList)
	{
		printfDeviceNameWithNumber(inputDevNameStr[devs], e->name(), e->enumId());
		inputDevName[devs].init(inputDevNameStr[devs]); item[i++] = &inputDevName[devs];
		inputDevName[devs].onSelect() =
			[this, devs](TextMenuItem &, View &, const Input::Event &e)
			{
				auto &imdMenu = *new InputManagerDeviceView{window(), *this};
				imdMenu.init(!e.isPointer(), inputDevConf[devs]);
				imdMenu.name_ = inputDevNameStr[devs];
				pushAndShow(imdMenu);
			};
		devs++;
	}
	TableView::init(item, i, highlightFirst);
}

#ifdef CONFIG_BASE_ANDROID
void InputManagerOptionsView::relativePointerDecelInit()
{
	static const char *str[] = { "Low", "Med.", "High" };
	int init = 0;
	if(optionRelPointerDecel == optionRelPointerDecelLow)
		init = 0;
	if(optionRelPointerDecel == optionRelPointerDecelMed)
		init = 1;
	if(optionRelPointerDecel == optionRelPointerDecelHigh)
		init = 2;
	relativePointerDecel.init(str, init, sizeofArray(str));
}
#endif

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
void InputManagerOptionsView::btScanSecsInit()
{
	static const char *str[] =
	{
		"2secs", "4secs", "6secs", "8secs", "10secs"
	};

	int val = 1;
	switch(BluetoothAdapter::scanSecs)
	{
		bcase 2: val = 0;
		bcase 4: val = 1;
		bcase 6: val = 2;
		bcase 8: val = 3;
		bcase 10: val = 4;
	}
	btScanSecs.init(str, val, sizeofArray(str));
}
#endif

InputManagerOptionsView::InputManagerOptionsView(Base::Window &win):
	TableView{"General Input Options", win},
	#ifdef CONFIG_BASE_ANDROID
	relativePointerDecel
	{
		"Trackball Sensitivity",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			#if defined(CONFIG_BASE_ANDROID)
			if(val == 0)
				optionRelPointerDecel.val = optionRelPointerDecelLow;
			else if(val == 1)
				optionRelPointerDecel.val = optionRelPointerDecelMed;
			else if(val == 2)
				optionRelPointerDecel.val = optionRelPointerDecelHigh;
			#endif
		}
	},
	#endif
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	mogaInputSystem
	{
		"MOGA Controller Support",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			if(!optionMOGAInputSystem && !Base::packageIsInstalled("com.bda.pivot.mogapgp"))
			{
				popup.post("Install the MOGA Pivot app from Google Play to use your MOGA Pocket. "
					"For MOGA Pro or newer, set switch to mode B and pair in the Android Bluetooth settings app instead.", 8);
				return;
			}
			item.toggle(*this);
			optionMOGAInputSystem = item.on;
			if(item.on)
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
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionNotifyInputDeviceChange = item.on;
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
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionKeepBluetoothActive = item.on;
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecs
	{
		"Scan Time",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			switch(val)
			{
				bcase 0: BluetoothAdapter::scanSecs = 2;
				bcase 1: BluetoothAdapter::scanSecs = 4;
				bcase 2: BluetoothAdapter::scanSecs = 6;
				bcase 3: BluetoothAdapter::scanSecs = 8;
				bcase 4: BluetoothAdapter::scanSecs = 10;
			}
			logMsg("set bluetooth scan time %d", BluetoothAdapter::scanSecs);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache
	{
		"Cache Scan Results",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionBlueToothScanCache = item.on;
		}
	},
	#endif
	altGamepadConfirm
	{
		"Swap Confirm/Cancel Keys",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			Input::swappedGamepadConfirm = item.on;
		}
	}
	{}

void InputManagerOptionsView::init(bool highlightFirst)
{
	uint i = 0;
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	mogaInputSystem.init(optionMOGAInputSystem); item[i++] = &mogaInputSystem;
	#endif
	altGamepadConfirm.init(Input::swappedGamepadConfirm); item[i++] = &altGamepadConfirm;
	#ifdef CONFIG_BASE_ANDROID
	if(Input::hasTrackball())
	{
		relativePointerDecelInit(); item[i++] = &relativePointerDecel;
	}
	#endif
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	if(!optionNotifyInputDeviceChange.isConst)
	{
		notifyDeviceChange.init(optionNotifyInputDeviceChange); item[i++] = &notifyDeviceChange;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothHeading.init(); item[i++] = &bluetoothHeading;
	if(!optionKeepBluetoothActive.isConst)
	{
		keepBtActive.init(optionKeepBluetoothActive); item[i++] = &keepBtActive;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecsInit(); item[i++] = &btScanSecs;
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache.init(optionBlueToothScanCache); item[i++] = &btScanCache;
	#endif
	TableView::init(item, i, highlightFirst);
}

class ProfileSelectMenu : public TableView
{
public:
	ProfileSelectMenu(Base::Window &win): TableView{"Key Profile", win} {}
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
					[this, &conf](TextMenuItem &, View &, const Input::Event &e)
					{
						auto del = onProfileChange;
						dismiss();
						del(conf);
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
				[this, &conf](TextMenuItem &, View &, const Input::Event &e)
				{
					auto del = onProfileChange;
					dismiss();
					del(conf);
				};
			i++;
		}
		TableView::init(choiceEntryItem, i, highlightFirst);
		if(highlightFirst && activeItem != -1)
		{
			selected = activeItem;
		}
	}

	void drawElement(uint i, Gfx::GCRect rect) const override
	{
		using namespace Gfx;
		if((int)i == activeItem)
			setColor(0., .8, 1.);
		else
			setColor(COLOR_WHITE);
		item[i]->draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), LC2DO, projP);
	}
};

InputManagerDeviceView::InputManagerDeviceView(Base::Window &win, InputManagerView &rootIMView):
	TableView{win},
	rootIMView{rootIMView},
	player
	{
		"Player",
		[this](MultiChoiceMenuItem &, View &, int val)
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
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &profileSelectMenu = *new ProfileSelectMenu{window()};
			profileSelectMenu.init(!e.isPointer(), *devConf->dev, devConf->keyConf().name);
			profileSelectMenu.onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name);
					devConf->setKeyConf(profile);
					onShow();
					keyMapping.buildAll();
				};
			pushAndShow(profileSelectMenu);
		}
	},
	renameProfile
	{
		"Rename Profile",
		[this](TextMenuItem &item, View &, const Input::Event &e)
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
			modalViewController.pushAndShow(textInputView);
		}
	},
	newProfile
	{
		"New Profile",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(customKeyConfig.isFull())
			{
				popup.postError("No space left for new key profiles, please delete one");
				return;
			}
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init("Create a new profile? All keys from the current profile will be copied over.", !e.isPointer());
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
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
					modalViewController.pushAndShow(textInputView);
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	},
	deleteProfile
	{
		"Delete Profile",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(!devConf->mutableKeyConf())
			{
				popup.post("Can't delete a built-in profile", 2, 0);
				return;
			}
			auto &ynAlertView = *new YesNoAlertView{window()};
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
			modalViewController.pushAndShow(ynAlertView);
		}
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			#ifdef CONFIG_BASE_IOS
			confirmICadeMode(e);
			#else
			if(!item.on)
			{
				auto &ynAlertView = *new YesNoAlertView{window()};
				ynAlertView.init("This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", !e.isPointer(), "Enable", "Cancel");
				ynAlertView.onYes() =
					[this](const Input::Event &e)
					{
						confirmICadeMode(e);
					};
				modalViewController.pushAndShow(ynAlertView);
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
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			updateBits(bits, item.on ? Input::Device::AXIS_BITS_STICK_1 : 0, Input::Device::AXIS_BITS_STICK_1);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	joystickAxis2DPad
	{
		"Joystick X/Y Axis 2 as D-Pad",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			updateBits(bits, item.on ? Input::Device::AXIS_BITS_STICK_2 : 0, Input::Device::AXIS_BITS_STICK_2);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	joystickAxisHatDPad
	{
		"Joystick POV Hat as D-Pad",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			updateBits(bits, item.on ? Input::Device::AXIS_BITS_HAT : 0, Input::Device::AXIS_BITS_HAT);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	}
{}

void InputManagerDeviceView::onShow()
{
	TableView::onShow();
	string_printf(profileStr, "Profile: %s", devConf->keyConf().name);
	loadProfile.compile(projP);
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
	physicalControlsPresent = Input::keyInputIsPresent();
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
	iterateTimes(EmuControls::categories, c)
	{
		auto &cat = EmuControls::category[c];
		if(cat.isMultiplayer && devConf.player != InputDeviceConfig::PLAYER_MULTI)
		{
			//logMsg("skipping category %s (%d)", cat.name, (int)c_i);
			continue;
		}
		inputCategory[c].init(cat.name); item[i++] = &inputCategory[c];
		inputCategory[c].onSelect() =
			[this, c](TextMenuItem &item, View &, const Input::Event &e)
			{
				auto &bcMenu = *new ButtonConfigView{window(), rootIMView};
				bcMenu.init(&EmuControls::category[c], *this->devConf, !e.isPointer());
				pushAndShow(bcMenu);
			};
	}
	newProfile.init(); item[i++] = &newProfile;
	renameProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &renameProfile;
	deleteProfile.init((bool)devConf.mutableKeyConf()); item[i++] = &deleteProfile;
	#if defined CONFIG_INPUT_ICADE
	if((devConf.dev->map() == Input::Event::MAP_SYSTEM && devConf.dev->hasKeyboard())
			|| devConf.dev->map() == Input::Event::MAP_ICADE)
	{
		iCadeMode.init(devConf.iCadeMode()); item[i++] = &iCadeMode;
	}
	#endif
	if(devConf.dev->joystickAxisBits() & Input::Device::AXIS_BIT_X)
	{
		joystickAxis1DPad.init(devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_X); item[i++] = &joystickAxis1DPad;
	}
	if(devConf.dev->joystickAxisBits() & Input::Device::AXIS_BIT_Z)
	{
		joystickAxis2DPad.init(devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_Z); item[i++] = &joystickAxis2DPad;
	}
	if(devConf.dev->joystickAxisBits() & Input::Device::AXIS_BIT_HAT_X)
	{
		joystickAxisHatDPad.init(devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_HAT_X); item[i++] = &joystickAxisHatDPad;
	}
	TableView::init(item, i, highlightFirst);
}
