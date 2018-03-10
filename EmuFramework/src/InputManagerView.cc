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
#include "EmuOptions.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/base/Base.hh>
#include "private.hh"
#include "privateInput.hh"

static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

IdentInputDeviceView::IdentInputDeviceView(ViewAttachParams attach):
	View(attach),
	text{"Push a key on any input device enter its configuration menu", &View::defaultFace}
{
	Input::setKeyRepeat(false);
}

IdentInputDeviceView::~IdentInputDeviceView()
{
	Input::setKeyRepeat(true);
}

void IdentInputDeviceView::place()
{
	text.maxLineSize = projP.w * 0.95;
	text.compile(renderer(), projP);
}

bool IdentInputDeviceView::inputEvent(Input::Event e)
{
	if(e.isPointer() && e.released())
	{
		dismiss();
		return true;
	}
	else if(!e.isPointer() && e.pushed())
	{
		auto del = onIdentInput;
		dismiss();
		del(e);
		return true;
	}
	return false;
}

void IdentInputDeviceView::draw()
{
	using namespace Gfx;
	auto &r = renderer();
	r.setBlendMode(0);
	r.noTexProgram.use(r, projP.makeTranslate());
	r.setColor(.4, .4, .4, 1.);
	GeomRect::draw(r, viewFrame, projP);
	r.setColor(COLOR_WHITE);
	r.texAlphaProgram.use(r);
	text.draw(r, 0, 0, C2DO, projP);
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

static InputManagerView::DeviceNameString makePrintfDeviceNameWithNumber(const char *name, uint id)
{
	char idStr[sizeof(" #00")] = "";
	if(id)
		string_printf(idStr, " #%d", id + 1);
	return string_makePrintf<sizeof(InputManagerView::DeviceNameString)>("%s%s", name, idStr);
}

InputManagerView::InputManagerView(ViewAttachParams attach):
	TableView{"Key/Gamepad Input Setup", attach, item},
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
			uint devs = savedInputDevList.size();
			deviceConfigStr.clear();
			deviceConfigStr.reserve(devs);
			for(auto &e : savedInputDevList)
			{
				deviceConfigStr.emplace_back(makePrintfDeviceNameWithNumber(e.name, e.enumId));
			}
			auto &multiChoiceView = *new TextTableView{item.t.str, attachParams(), devs};
			iterateTimes(devs, i)
			{
				multiChoiceView.appendItem(deviceConfigStr[i].data(),
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						pop();
						int deleteDeviceConfigIdx = i;
						auto &ynAlertView = *new YesNoAlertView{attachParams(), confirmDeleteDeviceSettingsStr};
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
								iterateTimes(inputDevConf.size(), i)
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
						modalViewController.pushAndShow(ynAlertView, e, false);
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
			uint profiles = customKeyConfig.size();
			profileStr.clear();
			profileStr.reserve(profiles);
			for(auto &e : customKeyConfig)
			{
				profileStr.emplace_back(e.name);
			}
			auto &multiChoiceView = *new TextTableView{item.t.str, attachParams(), profiles};
			iterateTimes(profiles, i)
			{
				multiChoiceView.appendItem(profileStr[i],
					[this, i](TextMenuItem &, View &, Input::Event e)
					{
						pop();
						int deleteProfileIdx = i;
						auto &ynAlertView = *new YesNoAlertView{attachParams(), confirmDeleteProfileStr};
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
						modalViewController.pushAndShow(ynAlertView, e, false);
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
			Input::enumDevices();
			uint devices = 0;
			for(auto &e : Input::deviceList())
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
			auto &identView = *new IdentInputDeviceView{attachParams()};
			identView.onIdentInput =
				[this](Input::Event e)
				{
					auto dev = e.device();
					if(dev)
					{
						auto &imdMenu = *new InputManagerDeviceView{attachParams(), *this, inputDevConf[dev->idx]};
						imdMenu.setName(inputDevName[dev->idx].t.str);
						pushAndShow(imdMenu, e);
					}
				};
			modalViewController.pushAndShow(identView, e, false);
		}
	},
	generalOptions
	{
		"General Options",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &optView = *new InputManagerOptionsView{attachParams()};
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
			modalViewController.popAll();
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

InputManagerView::~InputManagerView()
{
	onUpdateInputDevices = {};
}

void InputManagerView::loadItems()
{
	item.clear();
	item.reserve(16);
	item.emplace_back(&identDevice);
	item.emplace_back(&generalOptions);
	item.emplace_back(&deleteDeviceConfig);
	item.emplace_back(&deleteProfile);
	#ifdef CONFIG_BASE_ANDROID
	if(Base::androidSDK() >= 12 && Base::androidSDK() < 16)
	{
		item.emplace_back(&rescanOSDevices);
	}
	#endif
	item.emplace_back(&deviceListHeading);
	inputDevName.clear();
	inputDevNameStr.clear();
	inputDevName.reserve(Input::deviceList().size());
	inputDevNameStr.reserve(Input::deviceList().size());
	for(auto &e : Input::deviceList())
	{
		inputDevNameStr.emplace_back(makePrintfDeviceNameWithNumber(e->name(), e->enumId()));
		inputDevName.emplace_back(inputDevNameStr.back().data(),
			[this, idx = inputDevName.size()](TextMenuItem &, View &, Input::Event e)
			{
				auto &imdMenu = *new InputManagerDeviceView{attachParams(), *this, inputDevConf[idx]};
				imdMenu.setName(inputDevName[idx].t.str);
				pushAndShow(imdMenu, e);
			});
		if(e->hasKeys() && !e->isPowerButton())
		{
			item.emplace_back(&inputDevName.back());
		}
		else
		{
			logMsg("not adding device:%s to list", e->name());
		}
	}
}

void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.setActive(savedInputDevList.size());
	deleteProfile.setActive(customKeyConfig.size());
}

const char *InputManagerView::deviceName(uint idx) const
{
	return inputDevName[idx].t.str;
}

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
static void setBTScanSecs(int secs)
{
	BluetoothAdapter::scanSecs = secs;
	logMsg("set bluetooth scan time %d", BluetoothAdapter::scanSecs);
}
#endif

InputManagerOptionsView::InputManagerOptionsView(ViewAttachParams attach):
	TableView{"General Input Options", attach, item},
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
		[]()
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
		(bool)optionMOGAInputSystem,
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
		[]()
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
		Input::swappedGamepadConfirm(),
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			Input::setSwappedGamepadConfirm(item.flipBoolValue(*this));
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

	ProfileSelectMenu(ViewAttachParams attach, Input::Device &dev, const char *selectedName):
		TextTableView
		{
			"Key Profile",
			attach,
			(uint)customKeyConfig.size() + MAX_DEFAULT_KEY_CONFIGS_PER_TYPE
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

InputManagerDeviceView::InputManagerDeviceView(ViewAttachParams attach, InputManagerView &rootIMView_, InputDeviceConfig &devConfRef):
	TableView{attach, item},
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
		(int)playerConfToMenuIdx(devConfRef.player),
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
			auto &profileSelectMenu = *new ProfileSelectMenu{attachParams(), *devConf->dev, devConf->keyConf().name};
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
			EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e, "Input name", devConf->keyConf().name,
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
				});
		}
	},
	newProfile
	{
		"New Profile",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &ynAlertView = *new YesNoAlertView{attachParams(),
				"Create a new profile? All keys from the current profile will be copied over."};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e, "Input name", "",
						[this](CollectTextInputView &view, const char *str)
						{
							if(str && strlen(str))
							{
								if(customKeyConfigsContainName(str))
								{
									popup.postError("Another profile is already using this name");
									return 1;
								}
								devConf->setKeyConfCopiedFromExisting(str);
								logMsg("created new profile %s", devConf->keyConf().name);
								onShow();
								postDraw();
							}
							view.dismiss();
							return 0;
						});
				});
			modalViewController.pushAndShow(ynAlertView, e, false);
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
			auto &ynAlertView = *new YesNoAlertView{attachParams(), confirmDeleteProfileStr};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					auto conf = devConf->mutableKeyConf();
					if(!conf)
					{
						bug_unreachable("confirmed deletion of a read-only key config, should never happen");
						return;
					}
					logMsg("deleting profile: %s", conf->name);
					removeKeyConfFromAllDevices(conf);
					customKeyConfig.remove(*conf);
					onShow();
					keyMapping.buildAll();
				});
			modalViewController.pushAndShow(ynAlertView, e, false);
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
				auto &ynAlertView = *new YesNoAlertView{attachParams(),
					"This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", "Enable", "Cancel"};
				ynAlertView.setOnYes(
					[this](TextMenuItem &, View &view, Input::Event e)
					{
						view.dismiss();
						confirmICadeMode(e);
					});
				modalViewController.pushAndShow(ynAlertView, e, false);
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
				auto &bcMenu = *new ButtonConfigView{attachParams(), rootIMView, &EmuControls::category[c], *this->devConf};
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
	loadProfile.compile(renderer(), projP);
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
