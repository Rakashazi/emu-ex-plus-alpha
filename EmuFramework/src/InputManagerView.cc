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
#include "privateInput.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/ScopeGuard.hh>

static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

IdentInputDeviceView::IdentInputDeviceView(ViewAttachParams attach):
	View(attach),
	text{"Push a key on any input device enter its configuration menu", &defaultFace()}
{}

void IdentInputDeviceView::place()
{
	text.setMaxLineSize(projP.width() * 0.95);
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

void IdentInputDeviceView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	cmds.setBlendMode(0);
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	cmds.setColor(.4, .4, .4, 1.);
	GeomRect::draw(cmds, viewRect(), projP);
	cmds.set(ColorName::WHITE);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	text.draw(cmds, 0, 0, C2DO, projP);
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

InputManagerView::InputManagerView(ViewAttachParams attach):
	TableView{"Key/Gamepad Input Setup", attach, item},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!savedInputDevList.size())
			{
				app().postMessage("No saved device settings");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, savedInputDevList.size());
			for(unsigned i = 0; auto &e : savedInputDevList)
			{
				auto incIdx = IG::scopeGuard([&](){ i++; });
				multiChoiceView->appendItem(makeDeviceName(e.name, e.enumId).data(),
					[this, i](Input::Event e)
					{
						int deleteDeviceConfigIdx = i;
						auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteDeviceSettingsStr);
						ynAlertView->setOnYes(
							[this, deleteDeviceConfigIdx]()
							{
								auto it = savedInputDevList.begin();
								iterateTimes(deleteDeviceConfigIdx, i)
								{
									++it;
								}
								logMsg("deleting device settings for: %s,%d", it->name, it->enumId);
								auto &inputDevConf = app().inputDeviceConfigs();
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
								app().buildKeyInputMapping();
								dismissPrevious();
							});
						pushAndShowModal(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	deleteProfile
	{
		"Delete Saved Key Profile", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(!customKeyConfig.size())
			{
				app().postMessage("No saved profiles");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, customKeyConfig.size());
			for(unsigned i = 0; auto &e : customKeyConfig)
			{
				auto incIdx = IG::scopeGuard([&](){ i++; });
				multiChoiceView->appendItem(e.name,
					[this, i](Input::Event e)
					{
						int deleteProfileIdx = i;
						auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteProfileStr);
						ynAlertView->setOnYes(
							[this, deleteProfileIdx]()
							{
								auto it = customKeyConfig.begin();
								iterateTimes(deleteProfileIdx, i)
								{
									++it;
								}
								logMsg("deleting profile: %s", it->name);
								removeKeyConfFromAllDevices(&(*it));
								customKeyConfig.erase(it);
								app().buildKeyInputMapping();
								dismissPrevious();
							});
						pushAndShowModal(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	#ifdef CONFIG_BASE_ANDROID
	rescanOSDevices
	{
		"Re-scan OS Input Devices", &defaultFace(),
		[this](Input::Event e)
		{
			appContext().enumInputDevices();
			unsigned devices = 0;
			for(auto &e : appContext().inputDevices())
			{
				if(e->map() == Input::Map::SYSTEM || e->map() == Input::Map::ICADE)
					devices++;
			}
			app().printfMessage(2, false, "%d OS devices present", devices);
		}
	},
	#endif
	identDevice
	{
		"Auto-detect Device To Setup", &defaultFace(),
		[this](Input::Event e)
		{
			auto identView = makeView<IdentInputDeviceView>();
			identView->onIdentInput =
				[this](Input::Event e)
				{
					auto dev = e.device();
					if(dev)
					{
						auto imdMenu = makeViewWithName<InputManagerDeviceView>(inputDevName[dev->idx], *this, app().inputDeviceConfigs()[dev->idx]);
						pushAndShow(std::move(imdMenu), e);
					}
				};
			pushAndShowModal(std::move(identView), e);
		}
	},
	generalOptions
	{
		"General Options", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<InputManagerOptionsView>(&app().viewController().inputView()), e);
		}
	},
	deviceListHeading
	{
		"Individual Device Settings", &defaultBoldFace(),
	}
{
	app().setOnUpdateInputDevices(
		[this]()
		{
			popTo(*this);
			auto selectedCell = selected;
			waitForDrawFinished();
			loadItems();
			highlightCell(selectedCell);
			place();
			show();
		});
	deleteDeviceConfig.setActive(savedInputDevList.size());
	deleteProfile.setActive(customKeyConfig.size());
	loadItems();
}

InputManagerView::~InputManagerView()
{
	app().setOnUpdateInputDevices(nullptr);
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
	if(appContext().androidSDK() >= 12 && appContext().androidSDK() < 16)
	{
		item.emplace_back(&rescanOSDevices);
	}
	#endif
	item.emplace_back(&deviceListHeading);
	inputDevName.clear();
	inputDevName.reserve(appContext().inputDevices().size());
	for(auto &e : appContext().inputDevices())
	{
		inputDevName.emplace_back(makeDeviceName(e->name(), e->enumId()).data(), &defaultFace(),
			[this, idx = inputDevName.size()](Input::Event e)
			{
				pushAndShowDeviceView(idx, e);
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

void InputManagerView::pushAndShowDeviceView(unsigned idx, Input::Event e)
{
	assumeExpr(idx < inputDevName.size());
	pushAndShow(makeViewWithName<InputManagerDeviceView>(inputDevName[idx], *this, app().inputDeviceConfigs()[idx]), e);
}

InputManagerView::DeviceNameString InputManagerView::makeDeviceName(const char *name, unsigned id)
{
	char idStr[sizeof(" #00")] = "";
	if(id)
		string_printf(idStr, " #%u", id + 1);
	return string_makePrintf<sizeof(InputManagerView::DeviceNameString)>("%s%s", name, idStr);
}

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
static void setBTScanSecs(int secs)
{
	BluetoothAdapter::scanSecs = secs;
	logMsg("set bluetooth scan time %d", BluetoothAdapter::scanSecs);
}
#endif

InputManagerOptionsView::InputManagerOptionsView(ViewAttachParams attach, EmuInputView *emuInputView_):
	TableView{"General Input Options", attach, item},
	#if 0
	relativePointerDecelItem
	{
		{
			"Low", &defaultFace(),
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelLow;
			}
		},
		{
			"Med.", &defaultFace(),
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelMed;
			}
		},
		{
			"High", &defaultFace(),
			[this]()
			{
				optionRelPointerDecel.val = optionRelPointerDecelHigh;
			}
		}
	},
	relativePointerDecel
	{
		"Trackball Sensitivity", &defaultFace(),
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
		"MOGA Controller Support", &defaultFace(),
		(bool)optionMOGAInputSystem,
		[this](BoolMenuItem &item, Input::Event e)
		{
			if(!optionMOGAInputSystem && !appContext().packageIsInstalled("com.bda.pivot.mogapgp"))
			{
				app().postMessage(8, "Install the MOGA Pivot app from Google Play to use your MOGA Pocket. "
					"For MOGA Pro or newer, set switch to mode B and pair in the Android Bluetooth settings app instead.");
				return;
			}
			optionMOGAInputSystem = item.flipBoolValue(*this);
			if(optionMOGAInputSystem)
				appContext().initMogaInputSystem(true);
			else
				appContext().deinitMogaInputSystem();
		}
	},
	#endif
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	notifyDeviceChange
	{
		"Notify If Devices Change", &defaultFace(),
		(bool)optionNotifyInputDeviceChange,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionNotifyInputDeviceChange = item.flipBoolValue(*this);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothHeading
	{
		"In-app Bluetooth Options", &defaultBoldFace(),
	},
	keepBtActive
	{
		"Keep Connections In Background", &defaultFace(),
		(bool)optionKeepBluetoothActive,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionKeepBluetoothActive = item.flipBoolValue(*this);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecsItem
	{
		{
			"2secs", &defaultFace(),
			[this]()
			{
				setBTScanSecs(2);
			}
		},
		{
			"4secs", &defaultFace(),
			[this]()
			{
				setBTScanSecs(4);
			}
		},
		{
			"6secs", &defaultFace(),
			[this]()
			{
				setBTScanSecs(6);
			}
		},
		{
			"8secs", &defaultFace(),
			[this]()
			{
				setBTScanSecs(8);
			}
		},
		{
			"10secs", &defaultFace(),
			[this]()
			{
				setBTScanSecs(10);
			}
		}
	},
	btScanSecs
	{
		"Scan Time", &defaultFace(),
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
		"Cache Scan Results", &defaultFace(),
		(bool)optionBlueToothScanCache,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionBlueToothScanCache = item.flipBoolValue(*this);
		}
	},
	#endif
	altGamepadConfirm
	{
		"Swap Confirm/Cancel Keys", &defaultFace(),
		app().swappedConfirmKeys(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			app().setSwappedConfirmKeys(item.flipBoolValue(*this));
		}
	},
	consumeUnboundGamepadKeys
	{
		"Handle Unbound Gamepad Keys", &defaultFace(),
		(bool)optionConsumeUnboundGamepadKeys,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConsumeUnboundGamepadKeys = item.flipBoolValue(*this);
			if(emuInputView)
				emuInputView->setConsumeUnboundGamepadKeys(optionConsumeUnboundGamepadKeys);
		}
	},
	emuInputView{emuInputView_}
{
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	item.emplace_back(&mogaInputSystem);
	#endif
	item.emplace_back(&altGamepadConfirm);
	[this](auto &consumeUnboundGamepadKeys)
	{
		if constexpr(Config::envIsAndroid)
		{
			item.emplace_back(&consumeUnboundGamepadKeys);
		}
	}(consumeUnboundGamepadKeys);
	#if 0
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
			(unsigned)customKeyConfig.size() + MAX_DEFAULT_KEY_CONFIGS_PER_TYPE
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
				textItem.emplace_back(conf.name, &defaultFace(),
					[this, &conf](Input::Event e)
					{
						auto del = onProfileChange;
						dismiss();
						del(conf);
					});
			}
		}
		unsigned defaultConfs = 0;
		auto defaultConf = KeyConfig::defaultConfigsForDevice(dev, defaultConfs);
		iterateTimes(defaultConfs, c)
		{
			auto &conf = KeyConfig::defaultConfigsForDevice(dev)[c];
			if(string_equal(selectedName, defaultConf[c].name))
				activeItem = textItem.size();
			textItem.emplace_back(defaultConf[c].name, &defaultFace(),
				[this, &conf](Input::Event e)
				{
					auto del = onProfileChange;
					dismiss();
					del(conf);
				});
		}
	}
};

static unsigned playerConfToMenuIdx(unsigned player)
{
	if(player == InputDeviceConfig::PLAYER_MULTI)
		return 0;
	else
	{
		assert(player < EmuSystem::maxPlayers);
		return player + 1;
	}
}

InputManagerDeviceView::InputManagerDeviceView(NameString name, ViewAttachParams attach, InputManagerView &rootIMView_, InputDeviceConfig &devConfRef):
	TableView{std::move(name), attach, item},
	rootIMView{rootIMView_},
	playerItem
	{
		{"Multiple", &defaultFace(), [this]() { setPlayer(InputDeviceConfig::PLAYER_MULTI); }},
		{"1", &defaultFace(), [this]() { setPlayer(0); }},
		{"2", &defaultFace(), [this]() { setPlayer(1); }},
		{"3", &defaultFace(), [this]() { setPlayer(2); }},
		{"4", &defaultFace(), [this]() { setPlayer(3); }},
		{"5", &defaultFace(), [this]() { setPlayer(4); }}
	},
	player
	{
		"Player", &defaultFace(),
		(int)playerConfToMenuIdx(devConfRef.player),
		[](const MultiChoiceMenuItem &) -> unsigned
		{
			return EmuSystem::maxPlayers+1;
		},
		[this](const MultiChoiceMenuItem &, unsigned idx) -> TextMenuItem&
		{
			return playerItem[idx];
		}
	},
	loadProfile
	{
		nullptr, &defaultFace(),
		[this](Input::Event e)
		{
			auto profileSelectMenu = makeView<ProfileSelectMenu>(*devConf->dev, devConf->keyConf().name);
			profileSelectMenu->onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name);
					devConf->setKeyConf(profile);
					onShow();
					app().buildKeyInputMapping();
				};
			pushAndShow(std::move(profileSelectMenu), e);
		}
	},
	renameProfile
	{
		"Rename Profile", &defaultFace(),
		[this](Input::Event e)
		{
			if(!devConf->mutableKeyConf())
			{
				app().postMessage(2, "Can't rename a built-in profile");
				return;
			}
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", devConf->keyConf().name,
				[this](EmuApp &app, auto str)
				{
					if(customKeyConfigsContainName(str))
					{
						app.postErrorMessage("Another profile is already using this name");
						postDraw();
						return false;
					}
					assert(devConf->mutableKeyConf());
					string_copy(devConf->mutableKeyConf()->name, str);
					onShow();
					postDraw();
					return true;
				});
		}
	},
	newProfile
	{
		"New Profile", &defaultFace(),
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>(
				"Create a new profile? All keys from the current profile will be copied over.");
			ynAlertView->setOnYes(
				[this](Input::Event e)
				{
					app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", "",
						[this](EmuApp &app, auto str)
						{
							if(customKeyConfigsContainName(str))
							{
								app.postErrorMessage("Another profile is already using this name");
								return false;
							}
							devConf->setKeyConfCopiedFromExisting(str);
							logMsg("created new profile %s", devConf->keyConf().name);
							onShow();
							postDraw();
							return true;
						});
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	deleteProfile
	{
		"Delete Profile", &defaultFace(),
		[this](Input::Event e)
		{
			if(!devConf->mutableKeyConf())
			{
				app().postMessage(2, "Can't delete a built-in profile");
				return;
			}
			auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteProfileStr);
			ynAlertView->setOnYes(
				[this]()
				{
					auto conf = devConf->mutableKeyConf();
					if(!conf)
					{
						bug_unreachable("confirmed deletion of a read-only key config, should never happen");
						return;
					}
					logMsg("deleting profile: %s", conf->name);
					removeKeyConfFromAllDevices(conf);
					customKeyConfig.remove(*conf);
					app().buildKeyInputMapping();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode", &defaultFace(),
		devConfRef.iCadeMode(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			#ifdef CONFIG_BASE_IOS
			confirmICadeMode(e);
			#else
			if(!item.boolValue())
			{
				auto ynAlertView = makeView<YesNoAlertView>(
					"This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", "Enable", "Cancel");
				ynAlertView->setOnYes(
					[this](Input::Event e)
					{
						confirmICadeMode(e);
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
			else
				confirmICadeMode(e);
			#endif
		}
	},
	#endif
	joystickAxis1DPad
	{
		"Joystick X/Y Axis 1 as D-Pad", &defaultFace(),
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_X),
		[this](BoolMenuItem &item, Input::Event e)
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
		"Joystick X/Y Axis 2 as D-Pad", &defaultFace(),
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_Z),
		[this](BoolMenuItem &item, Input::Event e)
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
		"Joystick POV Hat as D-Pad", &defaultFace(),
		bool(devConfRef.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_HAT_X),
		[this](BoolMenuItem &item, Input::Event e)
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
	loadProfile.setName(string_makePrintf<128>("Profile: %s", devConf->keyConf().name).data());
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
		inputCategory[c] = {cat.name, &defaultFace(),
			[this, c](Input::Event e)
			{
				pushAndShow(makeView<ButtonConfigView>(rootIMView, &EmuControls::category[c], *this->devConf), e);
			}};
		item.emplace_back(&inputCategory[c]);
		inputCategories++;
	}
	item.emplace_back(&newProfile);
	item.emplace_back(&renameProfile);
	item.emplace_back(&deleteProfile);
	#if defined CONFIG_INPUT_ICADE
	if((devConf->dev->map() == Input::Map::SYSTEM && devConf->dev->hasKeyboard())
			|| devConf->dev->map() == Input::Map::ICADE)
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
	loadProfile.compile(string_makePrintf<128>("Profile: %s", devConf->keyConf().name).data(), renderer(), projP);
	bool keyConfIsMutable = devConf->mutableKeyConf();
	renameProfile.setActive(keyConfIsMutable);
	deleteProfile.setActive(keyConfIsMutable);
}

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode(Input::Event e)
{
	devConf->setICadeMode(iCadeMode.flipBoolValue(*this));
	onShow();
	app().viewController().setPhysicalControlsPresent(appContext().keyInputIsPresent());
	app().viewController().updateAutoOnScreenControlVisible();
	app().buildKeyInputMapping();
}
#endif

void InputManagerDeviceView::setPlayer(int playerVal)
{
	bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player != InputDeviceConfig::PLAYER_MULTI) ||
		(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player == InputDeviceConfig::PLAYER_MULTI);
	devConf->player = playerVal;
	devConf->save();
	{
		waitForDrawFinished();
		if(changingMultiplayer)
		{
			loadItems();
			place();
			show();
		}
		else
			onShow();
	}
	app().buildKeyInputMapping();
}
