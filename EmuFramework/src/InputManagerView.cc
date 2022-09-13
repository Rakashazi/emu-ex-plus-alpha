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
#include <imagine/util/variant.hh>

namespace EmuEx
{

static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

IdentInputDeviceView::IdentInputDeviceView(ViewAttachParams attach):
	View(attach),
	text{"Push a key on any input device enter its configuration menu", &defaultFace()}
{}

void IdentInputDeviceView::place()
{
	text.compile(renderer(), projP, {.maxLineSize = projP.width() * 0.95f});
}

bool IdentInputDeviceView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::MotionEvent &e)
		{
			if(e.released())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				auto del = onIdentInput;
				dismiss();
				del(e);
				return true;
			}
			return false;
		}
	}, e);
}

void IdentInputDeviceView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::OFF);
	basicEffect.disableTexture(cmds);
	cmds.setColor(.4, .4, .4, 1.);
	GeomRect::draw(cmds, displayRect(), projP);
	cmds.set(ColorName::WHITE);
	basicEffect.enableAlphaTexture(cmds);
	text.draw(cmds, {}, C2DO, projP);
}

static void removeKeyConfFromAllDevices(auto &savedInputDevs, const KeyConfig *conf, IG::ApplicationContext ctx)
{
	logMsg("removing saved key config %s from all devices", conf->name.data());
	for(auto &ePtr : savedInputDevs)
	{
		auto &e = *ePtr;
		if(e.keyConf == conf)
		{
			logMsg("used by saved device config %s,%d", e.name.data(), e.enumId);
			e.keyConf = nullptr;
		}
		auto devIt = IG::find_if(ctx.inputDevices(), [&](auto &devPtr){ return e.matchesDevice(*devPtr); });
		if(devIt != ctx.inputDevices().end())
		{
			inputDevData(*devIt->get()).buildKeyMap(*devIt->get());
		}
	}
}

InputManagerView::InputManagerView(ViewAttachParams attach,
	KeyConfigContainer &customKeyConfigs_,
	InputDeviceSavedConfigContainer &savedInputDevs_):
	TableView{"Key/Gamepad Input Setup", attach, item},
	customKeyConfigsPtr{&customKeyConfigs_},
	savedInputDevsPtr{&savedInputDevs_},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings", &defaultFace(),
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(!savedInputDevs().size())
			{
				app().postMessage("No saved device settings");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, savedInputDevs().size());
			for(auto &ePtr : savedInputDevs())
			{
				multiChoiceView->appendItem(InputDeviceData::makeDisplayName(ePtr->name, ePtr->enumId),
					[this, deleteDeviceConfigPtr = ePtr.get()](const Input::Event &e)
					{
						auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteDeviceSettingsStr);
						ynAlertView->setOnYes(
							[this, deleteDeviceConfigPtr]()
							{
								logMsg("deleting device settings for:%s,%d",
									deleteDeviceConfigPtr->name.data(), deleteDeviceConfigPtr->enumId);
								for(auto &devPtr : appContext().inputDevices())
								{
									auto &inputDevConf = inputDevData(*devPtr).devConf;
									if(inputDevConf.hasSavedConf(*deleteDeviceConfigPtr))
									{
										logMsg("removing from active device");
										inputDevConf.setSavedConf(nullptr);
										break;
									}
								}
								std::erase_if(savedInputDevs(), [&](auto &ptr){ return ptr.get() == deleteDeviceConfigPtr; });
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
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(!customKeyConfigs().size())
			{
				app().postMessage("No saved profiles");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, customKeyConfigs().size());
			for(auto &ePtr : customKeyConfigs())
			{
				multiChoiceView->appendItem(ePtr->name,
					[this, deleteProfilePtr = ePtr.get()](const Input::Event &e)
					{
						auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteProfileStr);
						ynAlertView->setOnYes(
							[this, deleteProfilePtr]()
							{
								logMsg("deleting profile: %s", deleteProfilePtr->name.data());
								removeKeyConfFromAllDevices(savedInputDevs(), deleteProfilePtr, appContext());
								std::erase_if(customKeyConfigs(), [&](auto &confPtr){ return confPtr.get() == deleteProfilePtr; });
								dismissPrevious();
							});
						pushAndShowModal(std::move(ynAlertView), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	rescanOSDevices
	{
		"Re-scan OS Input Devices", &defaultFace(),
		[this](const Input::Event &e)
		{
			appContext().enumInputDevices();
			int devices = 0;
			for(auto &e : appContext().inputDevices())
			{
				if(e->map() == Input::Map::SYSTEM || e->map() == Input::Map::ICADE)
					devices++;
			}
			app().postMessage(2, false, fmt::format("{} OS devices present", devices));
		}
	},
	identDevice
	{
		"Auto-detect Device To Setup", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto identView = makeView<IdentInputDeviceView>();
			identView->onIdentInput =
				[this](const Input::Event &e)
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
		[this](const Input::Event &e)
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
	deleteDeviceConfig.setActive(savedInputDevs_.size());
	deleteProfile.setActive(customKeyConfigs_.size());
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
	doIfUsed(rescanOSDevices, [&](auto &mItem)
	{
		if(appContext().androidSDK() >= 12 && appContext().androidSDK() < 16)
			item.emplace_back(&mItem);
	});
	item.emplace_back(&deviceListHeading);
	inputDevName.clear();
	inputDevName.reserve(appContext().inputDevices().size());
	for(auto &devPtr : appContext().inputDevices())
	{
		auto &devItem = inputDevName.emplace_back(inputDevData(*devPtr).displayName, &defaultFace(),
			[this, &dev = *devPtr](const Input::Event &e)
			{
				pushAndShowDeviceView(dev, e);
			});
		if(devPtr->hasKeys() && !devPtr->isPowerButton())
		{
			item.emplace_back(&devItem);
		}
		else
		{
			logMsg("not adding device:%s to list", devPtr->name().data());
		}
	}
}

void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.setActive(savedInputDevs().size());
	deleteProfile.setActive(customKeyConfigs().size());
}

void InputManagerView::pushAndShowDeviceView(const Input::Device &dev, const Input::Event &e)
{
	pushAndShow(makeViewWithName<InputManagerDeviceView>(inputDevData(dev).displayName, *this, dev,
		customKeyConfigs(), savedInputDevs()), e);
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
	mogaInputSystem
	{
		"MOGA Controller Support", &defaultFace(),
		app().mogaManagerIsActive(),
		[this](BoolMenuItem &item)
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
		(bool)app().notifyInputDeviceChangeOption().val,
		[this](BoolMenuItem &item)
		{
			app().notifyInputDeviceChangeOption() = item.flipBoolValue(*this);
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
		(bool)app().keepBluetoothActiveOption(),
		[this](BoolMenuItem &item)
		{
			app().keepBluetoothActiveOption() = item.flipBoolValue(*this);
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
		BluetoothAdapter::scanCacheUsage(),
		[this](BoolMenuItem &item)
		{
			BluetoothAdapter::setScanCacheUsage(item.flipBoolValue(*this));
		}
	},
	#endif
	altGamepadConfirm
	{
		"Swap Confirm/Cancel Keys", &defaultFace(),
		app().swappedConfirmKeys(),
		[this](BoolMenuItem &item)
		{
			app().setSwappedConfirmKeys(item.flipBoolValue(*this));
		}
	},
	emuInputView{emuInputView_}
{
	if constexpr(MOGA_INPUT)
	{
		item.emplace_back(&mogaInputSystem);
	}
	item.emplace_back(&altGamepadConfirm);
	#if 0
	if(Input::hasTrackball())
	{
		item.emplace_back(&relativePointerDecel);
	}
	#endif
	if constexpr(Config::Input::DEVICE_HOTSWAP)
	{
		if(!app().notifyInputDeviceChangeOption().isConst)
		{
			item.emplace_back(&notifyDeviceChange);
		}
	}
	#ifdef CONFIG_BLUETOOTH
	item.emplace_back(&bluetoothHeading);
	if(used(keepBtActive))
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

	ProfileSelectMenu(ViewAttachParams attach, Input::Device &dev, std::string_view selectedName,
		auto &customKeyConfigList):
		TextTableView
		{
			"Key Profile",
			attach,
			customKeyConfigList.size() + MAX_DEFAULT_KEY_CONFIGS_PER_TYPE
		}
	{
		for(auto &confPtr : customKeyConfigList)
		{
			auto &conf = *confPtr;
			if(conf.map == dev.map())
			{
				if(selectedName == conf.name)
				{
					activeItem = textItem.size();
				}
				textItem.emplace_back(conf.name, &defaultFace(),
					[this, &conf](const Input::Event &e)
					{
						auto del = onProfileChange;
						dismiss();
						del(conf);
					});
			}
		}
		for(const auto &conf : KeyConfig::defaultConfigsForDevice(dev))
		{
			if(selectedName == conf.name)
				activeItem = textItem.size();
			textItem.emplace_back(conf.name, &defaultFace(),
				[this, &conf](const Input::Event &e)
				{
					auto del = onProfileChange;
					dismiss();
					del(conf);
				});
		}
	}
};

static int playerConfToMenuIdx(int player)
{
	if(player == InputDeviceConfig::PLAYER_MULTI)
		return 0;
	else
	{
		assert(player < (int)EmuSystem::maxPlayers);
		return player + 1;
	}
}

static bool customKeyConfigsContainName(auto &customKeyConfigs, std::string_view name)
{
	return IG::find_if(customKeyConfigs, [&](auto &confPtr){ return confPtr->name == name; }) != customKeyConfigs.end();
}

InputManagerDeviceView::InputManagerDeviceView(IG::utf16String name, ViewAttachParams attach,
	InputManagerView &rootIMView_, const Input::Device &dev,
	KeyConfigContainer &customKeyConfigs_,
	InputDeviceSavedConfigContainer &savedInputDevs_):
	TableView{std::move(name), attach, item},
	customKeyConfigsPtr{&customKeyConfigs_},
	savedInputDevsPtr{&savedInputDevs_},
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
		[this](const Input::Event &e)
		{
			auto profileSelectMenu = makeView<ProfileSelectMenu>(devConf->device(),
				devConf->keyConf().name, customKeyConfigs());
			profileSelectMenu->onProfileChange =
				[this](const KeyConfig &profile)
				{
					logMsg("set key profile %s", profile.name.data());
					devConf->setKeyConf(profile, savedInputDevs());
					onShow();
				};
			pushAndShow(std::move(profileSelectMenu), e);
		}
	},
	renameProfile
	{
		"Rename Profile", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!devConf->mutableKeyConf(customKeyConfigs()))
			{
				app().postMessage(2, "Can't rename a built-in profile");
				return;
			}
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", devConf->keyConf().name,
				[this](EmuApp &app, auto str)
				{
					if(customKeyConfigsContainName(customKeyConfigs(), str))
					{
						app.postErrorMessage("Another profile is already using this name");
						postDraw();
						return false;
					}
					assert(devConf->mutableKeyConf(customKeyConfigs()));
					devConf->mutableKeyConf(customKeyConfigs())->name = str;
					onShow();
					postDraw();
					return true;
				});
		}
	},
	newProfile
	{
		"New Profile", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>(
				"Create a new profile? All keys from the current profile will be copied over.");
			ynAlertView->setOnYes(
				[this](const Input::Event &e)
				{
					app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", "",
						[this](EmuApp &app, auto str)
						{
							if(customKeyConfigsContainName(customKeyConfigs(), str))
							{
								app.postErrorMessage("Another profile is already using this name");
								return false;
							}
							devConf->setKeyConfCopiedFromExisting(str, customKeyConfigs(), savedInputDevs());
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
		[this](const Input::Event &e)
		{
			if(!devConf->mutableKeyConf(customKeyConfigs()))
			{
				app().postMessage(2, "Can't delete a built-in profile");
				return;
			}
			auto ynAlertView = makeView<YesNoAlertView>(confirmDeleteProfileStr);
			ynAlertView->setOnYes(
				[this]()
				{
					auto conf = devConf->mutableKeyConf(customKeyConfigs());
					if(!conf)
					{
						bug_unreachable("confirmed deletion of a read-only key config, should never happen");
					}
					logMsg("deleting profile: %s", conf->name.data());
					removeKeyConfFromAllDevices(savedInputDevs(), conf, appContext());
					std::erase_if(customKeyConfigs(), [&](auto &confPtr){ return confPtr.get() == conf; });
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	#if defined CONFIG_INPUT_ICADE
	iCadeMode
	{
		"iCade Mode", &defaultFace(),
		inputDevData(dev).devConf.iCadeMode(),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			if constexpr(Config::envIsIOS)
			{
				confirmICadeMode();
			}
			else
			{
				if(!item.boolValue())
				{
					auto ynAlertView = makeView<YesNoAlertView>(
						"This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", "Enable", "Cancel");
					ynAlertView->setOnYes(
						[this](const Input::Event &e)
						{
							confirmICadeMode();
						});
					pushAndShowModal(std::move(ynAlertView), e);
				}
				else
					confirmICadeMode();
			}
		}
	},
	#endif
	joystickAxis1DPad
	{
		"Joystick X/Y Axis 1 as D-Pad", &defaultFace(),
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_X),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_STICK_1 : 0, Input::Device::AXIS_BITS_STICK_1);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save(savedInputDevs());
		}
	},
	joystickAxis2DPad
	{
		"Joystick X/Y Axis 2 as D-Pad", &defaultFace(),
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_Z),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_STICK_2 : 0, Input::Device::AXIS_BITS_STICK_2);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save(savedInputDevs());
		}
	},
	joystickAxisHatDPad
	{
		"Joystick POV Hat as D-Pad", &defaultFace(),
		bool(inputDevData(dev).devConf.joystickAxisAsDpadBits() & Input::Device::AXIS_BIT_HAT_X),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			bool on = item.flipBoolValue(*this);
			auto bits = devConf->joystickAxisAsDpadBits();
			bits = IG::updateBits(bits, on ? Input::Device::AXIS_BITS_HAT : 0, Input::Device::AXIS_BITS_HAT);
			devConf->setJoystickAxisAsDpadBits(bits);
			devConf->save(savedInputDevs());
		}
	},
	consumeUnboundKeys
	{
		"Handle Unbound Keys", &defaultFace(),
		inputDevData(dev).devConf.shouldConsumeUnboundKeys(),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf->setConsumeUnboundKeys(item.flipBoolValue(*this));
			devConf->save(savedInputDevs());
		}
	},
	devConf{&inputDevData(dev).devConf}
{
	loadProfile.setName(fmt::format("Profile: {}", devConf->keyConf().name));
	renameProfile.setActive(devConf->mutableKeyConf(customKeyConfigs()));
	deleteProfile.setActive(devConf->mutableKeyConf(customKeyConfigs()));
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
			[this, &cat](const Input::Event &e)
			{
				pushAndShow(makeView<ButtonConfigView>(rootIMView, cat, *this->devConf), e);
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
	if constexpr(Config::envIsAndroid)
	{
		item.emplace_back(&consumeUnboundKeys);
	}
}

void InputManagerDeviceView::onShow()
{
	TableView::onShow();
	loadProfile.compile(fmt::format("Profile: {}", devConf->keyConf().name), renderer(), projP);
	bool keyConfIsMutable = devConf->mutableKeyConf(customKeyConfigs());
	renameProfile.setActive(keyConfIsMutable);
	deleteProfile.setActive(keyConfIsMutable);
}

#ifdef CONFIG_INPUT_ICADE
void InputManagerDeviceView::confirmICadeMode()
{
	devConf->setICadeMode(iCadeMode.flipBoolValue(*this), savedInputDevs());
	onShow();
	app().defaultVController().setPhysicalControlsPresent(appContext().keyInputIsPresent());
}
#endif

void InputManagerDeviceView::setPlayer(int playerVal)
{
	bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf->player() != InputDeviceConfig::PLAYER_MULTI) ||
		(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf->player() == InputDeviceConfig::PLAYER_MULTI);
	devConf->setPlayer(playerVal);
	devConf->save(savedInputDevs());
	if(changingMultiplayer)
	{
		loadItems();
		place();
		show();
	}
	else
		onShow();
}

}
