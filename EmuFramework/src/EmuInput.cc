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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/AppKeyCode.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/Option.hh>
#include "InputDeviceData.hh"
#include "gui/ResetAlertView.hh"
#include <emuframework/EmuOptions.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputManager"};

bool InputManager::handleKeyInput(EmuApp& app, KeyInfo keyInfo, const Input::Event &srcEvent)
{
	if(!keyInfo.flags.appCode)
	{
		handleSystemKeyInput(app, keyInfo, srcEvent.state(), srcEvent.metaKeyBits());
	}
	else
	{
		for(auto c : keyInfo.codes)
		{
			if(handleAppActionKeyInput(app, {c, keyInfo.flags, srcEvent.state(), srcEvent.metaKeyBits()}, srcEvent))
				return true;
		}
	}
	return false;
}

bool InputManager::handleAppActionKeyInput(EmuApp& app, InputAction action, const Input::Event &srcEvent)
{
	bool isPushed = action.state == Input::Action::PUSHED;
	auto& viewController = app.viewController();
	auto& system = app.system();
	assert(action.flags.appCode);
	using enum AppKeyCode;
	switch(AppKeyCode(action.code))
	{
		case fastForward:
		{
			viewController.inputView.setAltSpeedMode(AltSpeedMode::fast, isPushed);
		}
		break;
		case openContent:
		{
			if(!isPushed)
				break;
			log.info("show load game view from key event");
			viewController.popToRoot();
			viewController.pushAndShow(FilePicker::forLoading(app.attachParams(), srcEvent), srcEvent, false);
			return true;
		}
		case closeContent:
		{
			if(!isPushed)
				break;
			viewController.pushAndShowModal(app.makeCloseContentView(), srcEvent, false);
			return true;
		}
		case openSystemActions:
		{
			if(!isPushed)
				break;
			log.info("show system actions view from key event");
			app.showSystemActionsViewFromSystem(app.attachParams(), srcEvent);
			return true;
		}
		case saveState:
		{
			if(!isPushed)
				break;
			static auto doSaveState = [](EmuApp &app, bool notify){	app.saveStateWithSlot(app.system().stateSlot(), notify); };
			if(app.shouldOverwriteExistingState())
			{
				doSaveState(app, app.confirmOverwriteState);
			}
			else
			{
				viewController.pushAndShowModal(std::make_unique<YesNoAlertView>(app.attachParams(), "Really Overwrite State?",
					YesNoAlertView::Delegates
					{
						.onYes = [&app]
						{
							doSaveState(app, false);
							app.showEmulation();
						},
						.onNo = [&app]{ app.showEmulation(); }
					}), srcEvent, false);
			}
			return true;
		}
		case loadState:
		{
			if(!isPushed)
				break;
			app.loadStateWithSlot(system.stateSlot());
			return true;
		}
		case decStateSlot:
		{
			if(!isPushed)
				break;
			auto suspendCtx = app.suspendEmulationThread();
			system.decStateSlot();
			app.postMessage(1, false, std::format("State Slot: {}", system.stateSlotName()));
			return true;
		}
		case incStateSlot:
		{
			if(!isPushed)
				break;
			auto suspendCtx = app.suspendEmulationThread();
			system.incStateSlot();
			app.postMessage(1, false, std::format("State Slot: {}", system.stateSlotName()));
			return true;
		}
		case takeScreenshot:
		{
			if(!isPushed)
				break;
			app.video.takeGameScreenshot();
			return true;
		}
		case toggleFastForward:
		{
			if(!isPushed)
				break;
			viewController.inputView.toggleAltSpeedMode(AltSpeedMode::fast);
		}
		break;
		case openMenu:
		{
			if(!isPushed)
				break;
			log.info("show last view from key event");
			app.showLastViewFromSystem(app.attachParams(), srcEvent);
			return true;
		}
		case turboModifier:
		{
			turboModifierActive = isPushed;
			if(!isPushed)
				turboActions = {};
		}
		break;
		case exitApp:
		{
			if(!isPushed)
				break;
			viewController.pushAndShowModal(std::make_unique<YesNoAlertView>(app.attachParams(), "Really Exit?",
				YesNoAlertView::Delegates{.onYes = [&app]{ app.appContext().exit(); }}), srcEvent, false);
		}
		break;
		case slowMotion:
		{
			viewController.inputView.setAltSpeedMode(AltSpeedMode::slow, isPushed);
		}
		break;
		case toggleSlowMotion:
		{
			if(!isPushed)
				break;
			viewController.inputView.toggleAltSpeedMode(AltSpeedMode::slow);
		}
		break;
		case rewind:
		{
			if(!isPushed)
				break;
			if(app.rewindManager.maxStates)
			{
				app.rewindManager.rewindState(app);
			}
			else
			{
				auto suspendCtx = app.suspendEmulationThread();
				app.postMessage(3, false, "Please set rewind states in Options➔System");
			}
		}
		break;
		case softReset:
		{
			if(!isPushed)
				break;
			auto suspendCtx = app.suspendEmulationThread();
			system.reset(app, EmuSystem::ResetMode::SOFT);
		}
		break;
		case hardReset:
		{
			if(!isPushed)
				break;
			auto suspendCtx = app.suspendEmulationThread();
			system.reset(app, EmuSystem::ResetMode::HARD);
		}
		break;
		case resetMenu:
		{
			if(!isPushed)
				break;
			viewController.pushAndShowModal(resetAlertView(app.attachParams(), app), srcEvent, false);
		}
		break;
	}
	return false;
}

void InputManager::handleSystemKeyInput(EmuApp& app, KeyInfo keyInfo, Input::Action act, uint32_t metaState, SystemKeyInputFlags flags)
{
	if(flags.allowTurboModifier && turboModifierActive && std::ranges::all_of(keyInfo.codes, app.allowsTurboModifier))
		keyInfo.flags.turbo = 1;
	if(keyInfo.flags.toggle)
	{
		toggleInput.updateEvent(app, keyInfo, act);
	}
	else if(keyInfo.flags.turbo)
	{
		turboActions.updateEvent(app, keyInfo, act);
	}
	else
	{
		app.defaultVController().updateSystemKeys(keyInfo, act == Input::Action::PUSHED);
		for(auto code : keyInfo.codes)
		{
			app.system().handleInputAction(&app, {code, keyInfo.flags, act, metaState});
		}
	}
}

void InputManager::updateInputDevices(ApplicationContext ctx)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		log.info("input device:{}, id:{}, map:{}", devPtr->name(), devPtr->enumId(), (int)devPtr->map());
		devPtr->makeAppData<InputDeviceData>(*this, *devPtr);
	}
	vController.setPhysicalControlsPresent(ctx.keyInputIsPresent());
	onUpdateDevices.callCopySafe();
}

KeyConfig* InputManager::customKeyConfig(std::string_view name, const Input::Device &dev) const
{
	return findPtr(customKeyConfigs, [&](auto &ptr){ return ptr->name == name && ptr->desc().map == dev.map(); });
}

KeyConfigDesc InputManager::keyConfig(std::string_view name, const Input::Device &dev) const
{
	auto conf = customKeyConfig(name, dev);
	if(conf)
		return conf->desc();
	for(auto &conf : EmuApp::defaultKeyConfigs())
	{
		if(conf.name == name && conf.map == dev.map())
			return conf;
	}
	return {};
}

static void removeKeyConfFromAllDevices(InputManager &inputManager, std::string_view conf, ApplicationContext ctx)
{
	log.info("removing saved key config {} from all devices", conf);
	for(auto &ePtr : inputManager.savedDevConfigs)
	{
		auto &e = *ePtr;
		if(e.keyConfName == conf)
		{
			log.info("used by saved device config {},{}", e.name, e.enumId);
			e.keyConfName.clear();
		}
		if(auto ptr = findPtr(ctx.inputDevices(), [&](auto &devPtr){ return e.matchesDevice(*devPtr); });
			ptr)
		{
			inputDevData(*ptr).buildKeyMap(inputManager, *ptr);
		}
	}
}

void InputManager::deleteKeyProfile(ApplicationContext ctx, KeyConfig *profile)
{
	removeKeyConfFromAllDevices(*this, profile->name, ctx);
	std::erase_if(customKeyConfigs, [&](auto &confPtr){ return confPtr.get() == profile; });
}

template<class SavedConfig>
static void deleteDeviceSavedConfigImpl(InputManager &mgr, ApplicationContext ctx, const SavedConfig& savedConf, auto& savedConfs)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		auto &inputDevConf = inputDevData(*devPtr).devConf;
		if(inputDevConf.hasSavedConf(savedConf))
		{
			log.info("removing from active device");
			inputDevConf.setSavedConf(mgr, (SavedConfig*){});
			break;
		}
	}
	std::erase_if(savedConfs, [&](auto &ptr){ return ptr.get() == &savedConf; });
}

void InputManager::deleteDeviceSavedConfig(ApplicationContext ctx, const InputDeviceSavedConfig& savedConf)
{
	log.info("deleting device settings for:{},{}", savedConf.name, savedConf.enumId);
	deleteDeviceSavedConfigImpl(*this, ctx, savedConf, savedDevConfigs);
}

void InputManager::deleteDeviceSavedConfig(ApplicationContext ctx, const InputDeviceSavedSessionConfig& savedConf)
{
	log.info("deleting session device settings for:{},{}", savedConf.name, savedConf.enumId);
	deleteDeviceSavedConfigImpl(*this, ctx, savedConf, savedSessionDevConfigs);
}

void InputManager::writeCustomKeyConfigs(FileIO &io) const
{
	for(const auto &c : customKeyConfigs)
	{
		c->writeConfig(io);
	}
}

void InputManager::writeSavedInputDevices(ApplicationContext ctx, FileIO &io) const
{
	if(!savedDevConfigs.size())
		return;
	// compute total size
	ssize_t bytes = 2; // config key size
	bytes += 1; // number of configs
	for(auto &ePtr : savedDevConfigs)
	{
		auto &e = *ePtr;
		bytes += 1; // device id
		bytes += 1; // enabled
		bytes += 1; // player
		bytes += 1; // mapJoystickAxis1ToDpad
		if(hasICadeInput)
			bytes += 1; // iCade mode
		bytes += sizedDataBytes<uint8_t>(e.name);
		bytes += 1; // key config map
		bytes += sizedDataBytes<uint8_t>(e.keyConfName);
	}
	if(bytes > 0xFFFF)
	{
		bug_unreachable("excessive input device config size, should not happen");
	}
	// write to config file
	log.info("saving {} input device configs, {} bytes", savedDevConfigs.size(), bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_DEVICE_CONFIGS));
	io.put(uint8_t(savedDevConfigs.size()));
	for(auto &ePtr : savedDevConfigs)
	{
		auto &e = *ePtr;
		log.info("writing config {}, id {}", e.name, e.enumId);
		uint8_t enumIdWithFlags = e.enumId;
		if(e.handleUnboundEvents)
			enumIdWithFlags |= e.HANDLE_UNBOUND_EVENTS_FLAG;
		io.put(uint8_t(enumIdWithFlags));
		io.put(uint8_t(e.enabled));
		io.put(uint8_t(e.player));
		io.put(std::bit_cast<uint8_t>(e.joystickAxisAsDpadFlags));
		if(hasICadeInput)
			io.put(uint8_t(e.iCadeMode));
		writeSizedData<uint8_t>(io, e.name);
		auto devPtr = ctx.inputDevice(e.name, e.enumId);
		uint8_t keyConfMap = devPtr ? (uint8_t)devPtr->map() : 0;
		io.put(keyConfMap);
		if(e.keyConfName.size())
		{
			log.info("has key conf {}, map {}", e.keyConfName, keyConfMap);
		}
		writeSizedData<uint8_t>(io, e.keyConfName);
	}
}

bool InputManager::readCustomKeyConfig(MapIO &io)
{
	auto conf = KeyConfig::readConfig(io);
	if(conf)
	{
		customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(conf));
	}
	return true;
}

bool InputManager::readSavedInputDevices(MapIO &io)
{
	auto confs = io.get<uint8_t>();
	for([[maybe_unused]] auto confIdx : iotaCount(confs))
	{
		InputDeviceSavedConfig devConf;
		auto enumIdWithFlags = io.get<uint8_t>();
		devConf.handleUnboundEvents = enumIdWithFlags & devConf.HANDLE_UNBOUND_EVENTS_FLAG;
		devConf.enumId = enumIdWithFlags & devConf.ENUM_ID_MASK;
		devConf.enabled = io.get<uint8_t>();
		devConf.player = io.get<uint8_t>();
		if(devConf.player != playerIndexMulti && devConf.player > EmuSystem::maxPlayers)
		{
			log.warn("player {} out of range", devConf.player);
			devConf.player = 0;
		}
		devConf.joystickAxisAsDpadFlags = std::bit_cast<AxisAsDpadFlags>(io.get<uint8_t>());
		if(hasICadeInput)
		{
			devConf.iCadeMode = io.get<uint8_t>();
		}
		readSizedData<uint8_t>(io, devConf.name);
		if(devConf.name.empty())
		{
			log.error("unexpected 0 length device name");
			return false;
		}
		[[maybe_unused]] auto keyConfMap = Input::validateMap(io.get<uint8_t>());
		readSizedData<uint8_t>(io, devConf.keyConfName);
		if(!find(savedDevConfigs, [&](const auto &confPtr){ return *confPtr == devConf;}))
		{
			log.info("read input device config:{}, id:{}", devConf.name, devConf.enumId);
			savedDevConfigs.emplace_back(std::make_unique<InputDeviceSavedConfig>(std::move(devConf)));
		}
		else
		{
			log.warn("ignoring duplicate input device config:{}, id:{}", devConf.name, devConf.enumId);
		}
	}
	return true;
}

void InputManager::resetSessionOptions(ApplicationContext ctx)
{
	for(auto& devPtr: ctx.inputDevices())
	{
		inputDevData(*devPtr).devConf.setSavedConf(*this, (InputDeviceSavedSessionConfig*){});
	}
	savedSessionDevConfigs.clear();
}

bool InputManager::readSessionConfig(ApplicationContext ctx, MapIO& io, uint16_t key)
{
	if(key == CFGKEY_INPUT_DEVICE_CONTENT_CONFIGS)
	{
		readInputDeviceSessionConfigs(ctx, io);
		return true;
	}
	return false;
}

void InputManager::writeSessionConfig(FileIO& io) const
{
	writeInputDeviceSessionConfigs(io);
}

void InputManager::writeInputDeviceSessionConfigs(FileIO &io) const
{
	if(!savedSessionDevConfigs.size())
		return;
	// compute total size
	ssize_t bytes = 2; // config key size
	bytes += 1; // number of configs
	for(auto &ePtr : savedSessionDevConfigs)
	{
		auto &e = *ePtr;
		bytes += 1; // device id
		bytes += 1; // player
		bytes += sizedDataBytes<uint8_t>(e.name);
		bytes += sizedDataBytes<uint8_t>(e.keyConfName);
	}
	if(bytes > 0xFFFF)
	{
		bug_unreachable("excessive input device config size, should not happen");
	}
	// write to config file
	log.info("saving {} input device content configs, {} bytes", savedSessionDevConfigs.size(), bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_DEVICE_CONTENT_CONFIGS));
	io.put(uint8_t(savedSessionDevConfigs.size()));
	for(auto &ePtr : savedSessionDevConfigs)
	{
		auto &e = *ePtr;
		log.info("writing config {}, id {}", e.name, e.enumId);
		io.put(uint8_t(e.enumId));
		io.put(int8_t(e.player));
		writeSizedData<uint8_t>(io, e.name);
		writeSizedData<uint8_t>(io, e.keyConfName);
	}
}

bool InputManager::readInputDeviceSessionConfigs(ApplicationContext ctx, MapIO &io)
{
	savedSessionDevConfigs.clear();
	auto confs = io.get<uint8_t>();
	for([[maybe_unused]] auto confIdx : iotaCount(confs))
	{
		InputDeviceSavedSessionConfig devConf;
		devConf.enumId = io.get<uint8_t>();
		devConf.player = io.get<int8_t>();
		if(devConf.player != playerIndexMulti && devConf.player != playerIndexUnset && devConf.player > EmuSystem::maxPlayers)
		{
			log.warn("player {} out of range", devConf.player);
			devConf.player = playerIndexUnset;
		}
		readSizedData<uint8_t>(io, devConf.name);
		if(devConf.name.empty())
		{
			log.error("unexpected 0 length device name");
			return false;
		}
		readSizedData<uint8_t>(io, devConf.keyConfName);
		if(!find(savedSessionDevConfigs, [&](const auto &conf){ return *conf == devConf;}))
		{
			log.info("read input device content config:{}, id:{}", devConf.name, devConf.enumId);
			auto& confPtr = savedSessionDevConfigs.emplace_back(std::make_unique<InputDeviceSavedSessionConfig>(std::move(devConf)));
			for(auto& devPtr: ctx.inputDevices())
			{
				if(confPtr->matchesDevice(*devPtr))
				{
					log.info("{} has saved session config", devPtr->name());
					inputDevData(*devPtr).devConf.setSavedConf(*this, confPtr.get());
					break;
				}
			}
		}
		else
		{
			log.warn("ignoring duplicate input device content config:{}, id:{}", devConf.name, devConf.enumId);
		}
	}
	return true;
}

KeyConfigDesc InputManager::defaultConfig(const Input::Device &dev) const
{
	KeyConfigDesc firstConfig{}, firstSubtypeConfig{};
	for(const auto &conf : EmuApp::defaultKeyConfigs())
	{
		if(dev.map() == conf.map && !firstConfig)
			firstConfig = conf;
		if(dev.subtype() == conf.devSubtype && !firstSubtypeConfig)
			firstSubtypeConfig = conf;
	}
	return firstSubtypeConfig ?: firstConfig;
}

KeyInfo InputManager::transpose(KeyInfo c, int index) const
{
	if(index != playerIndexMulti)
		c.flags.deviceId = index;
	return c;
}

std::string InputManager::toString(KeyInfo k) const
{
	std::string s{toString(k.codes[0], k.flags)};
	for(const auto &c : k.codes | std::ranges::views::drop(1))
	{
		s += " + ";
		s += toString(c, k.flags);
	}
	if(k.flags.turbo && k.flags.toggle)
		s += " (Turbo Toggle)";
	else if(k.flags.turbo)
		s += " (Turbo)";
	else if(k.flags.toggle)
		s += " (Toggle)";
	return s;
}

std::string InputManager::toString(KeyCode c, KeyFlags flags) const
{
	if(flags.appCode)
		return std::string{EmuEx::toString(AppKeyCode(c))};
	return std::string{EmuApp::systemKeyCodeToString(c)};
}

void InputManager::updateKeyboardMapping()
{
	vController.updateKeyboardMapping();
}

void InputManager::toggleKeyboard()
{
	vController.resetHighlightedKeys();
	vController.toggleKeyboard();
}

void EmuApp::setDisabledInputKeys(std::span<const KeyCode> keys)
{
	auto &vController = inputManager.vController;
	vController.setDisabledInputKeys(keys);
	if(!vController.hasWindow())
		return;
	vController.place();
	system().clearInputBuffers(viewController().inputView);
}

void EmuApp::unsetDisabledInputKeys()
{
	setDisabledInputKeys({});
}

bool InputDeviceSavedConfig::matchesDevice(const Input::Device& dev) const
{
	return dev.enumId() == enumId && dev.name() == name;
}

bool InputDeviceSavedSessionConfig::matchesDevice(const Input::Device& dev) const
{
	return dev.enumId() == enumId && dev.name() == name;
}

const KeyCategory *InputManager::categoryOfKeyCode(KeyInfo key) const
{
	if(key.isAppKey())
		return &appKeyCategory;
	for(const auto &cat : EmuApp::keyCategories())
	{
		if(find(cat.keys, [&](auto &k) { return k.codes == key.codes; }))
			return &cat;
	}
	return {};
}

KeyInfo InputManager::validateSystemKey(KeyInfo key, bool isUIKey) const
{
	if(!categoryOfKeyCode(key))
	{
		log.warn("resetting invalid system key:{}", key.codes[0]);
		if(isUIKey)
			return appKeyCategory.keys[0];
		else
			return EmuApp::keyCategories()[0].keys[0];
	}
	return key;
}

std::string_view toString(AppKeyCode code)
{
	switch(code)
	{
		case AppKeyCode::openContent: return "Open Content";
		case AppKeyCode::closeContent: return "Close Content";
		case AppKeyCode::openSystemActions: return "Open System Actions";
		case AppKeyCode::saveState: return "Save State";
		case AppKeyCode::loadState: return "Load State";
		case AppKeyCode::decStateSlot: return "Decrement State Slot";
		case AppKeyCode::incStateSlot: return "Increment State Slot";
		case AppKeyCode::fastForward: return "Fast-forward";
		case AppKeyCode::takeScreenshot: return "Take Screenshot";
		case AppKeyCode::openMenu: return "Open Menu";
		case AppKeyCode::toggleFastForward: return "Toggle Fast-forward";
		case AppKeyCode::turboModifier: return "Turbo Modifier";
		case AppKeyCode::exitApp: return "Exit App";
		case AppKeyCode::slowMotion: return "Slow-motion";
		case AppKeyCode::toggleSlowMotion: return "Toggle Slow-motion";
		case AppKeyCode::rewind: return "Rewind One State";
		case AppKeyCode::softReset: return "Soft Reset";
		case AppKeyCode::hardReset: return "Hard Reset";
		case AppKeyCode::resetMenu: return "Open Reset Menu";
	};
	return "";
}

}
