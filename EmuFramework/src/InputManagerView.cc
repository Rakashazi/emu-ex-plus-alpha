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
#include <imagine/util/format.hh>

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

static void removeKeyConfFromAllDevices(const KeyConfig *conf, Base::ApplicationContext ctx)
{
	logMsg("removing saved key config %s from all devices", conf->name.data());
	for(auto &e : savedInputDevList)
	{
		if(e.keyConf == conf)
		{
			logMsg("used by saved device config %s,%d", e.name, e.enumId);
			e.keyConf = nullptr;
		}
		auto devIt = IG::find_if(ctx.inputDevices(), [&](auto &devPtr){ return e.matchesDevice(*devPtr); });
		if(devIt != ctx.inputDevices().end())
		{
			inputDevData(*devIt->get()).buildKeyMap(*devIt->get());
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
				multiChoiceView->appendItem(InputDeviceData::makeDisplayName(e.name, e.enumId),
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
								logMsg("deleting device settings for:%s,%d", it->name, it->enumId);
								for(auto &devPtr : appContext().inputDevices())
								{
									auto &inputDevConf = inputDevData(*devPtr).devConf;
									if(inputDevConf.hasSavedConf(*it))
									{
										logMsg("removing from active device");
										inputDevConf.setSavedConf(nullptr);
										break;
									}
								}
								savedInputDevList.erase(it);
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
				multiChoiceView->appendItem(e.name.data(),
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
								logMsg("deleting profile: %s", it->name.data());
								removeKeyConfFromAllDevices(&(*it), appContext());
								customKeyConfig.erase(it);
								dismissPrevious();
							});
						pushAndShowModal(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	#ifdef __ANDROID__
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
			app().postMessage(2, false, fmt::format("{} OS devices present", devices));
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
						pushAndShowDeviceView(*dev, e);
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
	#ifdef __ANDROID__
	if(appContext().androidSDK() >= 12 && appContext().androidSDK() < 16)
	{
		item.emplace_back(&rescanOSDevices);
	}
	#endif
	item.emplace_back(&deviceListHeading);
	inputDevName.clear();
	inputDevName.reserve(appContext().inputDevices().size());
	for(auto &devPtr : appContext().inputDevices())
	{
		auto &devItem = inputDevName.emplace_back(inputDevData(*devPtr).displayName, &defaultFace(),
			[this, &dev = *devPtr](Input::Event e)
			{
				pushAndShowDeviceView(dev, e);
			});
		if(devPtr->hasKeys() && !devPtr->isPowerButton())
		{
			item.emplace_back(&devItem);
		}
		else
		{
			logMsg("not adding device:%s to list", devPtr->name());
		}
	}
}

void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.setActive(savedInputDevList.size());
	deleteProfile.setActive(customKeyConfig.size());
}

void InputManagerView::pushAndShowDeviceView(const Input::Device &dev, Input::Event e)
{
	pushAndShow(makeViewWithName<InputManagerDeviceView>(inputDevData(dev).displayName, *this, dev), e);
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
	mogaInputSystem
	{
		"MOGA Controller Support", &defaultFace(),
		app().mogaManagerIsActive(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			if(!app().mogaManagerIsActive() && !appContext().packageIsInstalled("com.bda.pivot.mogapgp"))
			{
				app().postMessage(8, "Install the MOGA Pivot app from Google Play to use your MOGA Pocket. "
					"For MOGA Pro or newer, set switch to mode B and pair in the Android Bluetooth settings app instead.");
				return;
			}
			app().setMogaManagerActive(item.flipBoolValue(*this), true);
		}
	},
	notifyDeviceChange
	{
		"Notify If Devices Change", &defaultFace(),
		(bool)optionNotifyInputDeviceChange,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionNotifyInputDeviceChange = item.flipBoolValue(*this);
		}
	},
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
	if constexpr(Config::EmuFramework::MOGA_INPUT)
	{
		item.emplace_back(&mogaInputSystem);
	}
	item.emplace_back(&altGamepadConfirm);
	if constexpr(Config::envIsAndroid)
	{
		item.emplace_back(&consumeUnboundGamepadKeys);
	}
	#if 0
	if(Input::hasTrackball())
	{
		item.emplace_back(&relativePointerDecel);
	}
	#endif
	if constexpr(Config::Input::DEVICE_HOTSWAP)
	{
		if(!optionNotifyInputDeviceChange.isConst)
		{
			item.emplace_back(&notifyDeviceChange);
		}
	}
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
				if(string_equal(selectedName, conf.name.data()))
				{
					activeItem = textItem.size();
				}
				textItem.emplace_back(conf.name.data(), &defaultFace(),
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
			if(string_equal(selectedName, defaultConf[c].name.data()))
				activeItem = textItem.size();
			textItem.emplace_back(defaultConf[c].name.data(), &defaultFace(),
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

InputManagerDeviceView::InputManagerDeviceView(IG::utf16String name, ViewAttachParams attach, InputManagerView &rootIMView_, const Input::Device &dev):
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
		(int)playerConfToMenuIdx(inputDevData(dev).devConf.player()),
		[](const MultiChoiceMenuItem &)
		{
			return EmuSystem::maxPlayers+1;
		},
		[this](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return playerItem[idx];
		}
	},
	loadProfile
	{
		{}, &defaultFace(),
		[this](Input::Event e)
		{
			auto profileSelectMenu = makeView<ProfileSelectMenu>(devConf->device(), devConf->keyConf().name.data());
			profileSelectMenu->onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name.data());
					devConf->setKeyConf(profile);
					onShow();
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
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", devConf->keyConf().name.data(),
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
							logMsg("created new profile %s", devConf->keyConf().name.data());
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
					logMsg("deleting profile: %s", conf->name.data());
					removeKeyConfFromAllDevices(conf, appContext());
					customKeyConfig.remove(*conf);
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode", &defaultFace(),
		inputDevData(dev).devConf.iCadeMode(),
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
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_X),
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
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_Z),
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
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_HAT_X),
		[this](BoolMenuItem &item, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_HAT : 0, Input::Device::AXIS_BITS_HAT);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save();
		}
	},
	devConf{&inputDevData(dev).devConf}
{
	loadProfile.setName(fmt::format("Profile: {}", devConf->keyConf().name.data()).data());
	renameProfile.setActive(devConf->mutableKeyConf());
	deleteProfile.setActive(devConf->mutableKeyConf());
	loadItems();
}

void InputManagerDeviceView::loadItems()
{
	item.clear();
	inputCategory.clear();
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&player);
	}
	item.emplace_back(&loadProfile);
	for(auto &cat : app().inputControlCategories())
	{
		if(cat.isMultiplayer && devConf->player() != InputDeviceConfig::PLAYER_MULTI)
		{
			//logMsg("skipping category %s (%d)", cat.name, (int)c_i);
			continue;
		}
		auto &catItem = inputCategory.emplace_back(cat.name, &defaultFace(),
			[this, &cat](Input::Event e)
			{
				pushAndShow(makeView<ButtonConfigView>(rootIMView, &cat, *this->devConf), e);
			});
		item.emplace_back(&catItem);
	}
	item.emplace_back(&newProfile);
	item.emplace_back(&renameProfile);
	item.emplace_back(&deleteProfile);
	#if defined CONFIG_INPUT_ICADE
	auto &dev = devConf->device();
	if((dev.map() == Input::Map::SYSTEM && dev.hasKeyboard())
			|| dev.map() == Input::Map::ICADE)
	{
		item.emplace_back(&iCadeMode);
	}
	#endif
	if(dev.motionAxis(Input::AxisId::X))
	{
		item.emplace_back(&joystickAxis1DPad);
	}
	if(dev.motionAxis(Input::AxisId::Z))
	{
		item.emplace_back(&joystickAxis2DPad);
	}
	if(dev.motionAxis(Input::AxisId::HAT0X))
	{
		item.emplace_back(&joystickAxisHatDPad);
	}
}

void InputManagerDeviceView::onShow()
{
	TableView::onShow();
	loadProfile.compile(fmt::format("Profile: {}", devConf->keyConf().name.data()).data(), renderer(), projP);
	bool keyConfIsMutable = devConf->mutableKeyConf();
	renameProfile.setActive(keyConfIsMutable);
	deleteProfile.setActive(keyConfIsMutable);
}

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode(Input::Event e)
{
	devConf->setICadeMode(iCadeMode.flipBoolValue(*this));
	onShow();
	app().defaultVController().setPhysicalControlsPresent(appContext().keyInputIsPresent());
}
#endif

void InputManagerDeviceView::setPlayer(int playerVal)
{
	bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player() != InputDeviceConfig::PLAYER_MULTI) ||
		(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player() == InputDeviceConfig::PLAYER_MULTI);
	devConf->setPlayer(playerVal);
	devConf->save();
	if(changingMultiplayer)
	{
		loadItems();
		place();
		show();
	}
	else
		onShow();
}
