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
#include <emuframework/InputManagerView.hh>
#include "privateInput.hh"
#include "WindowData.hh"
#include "EmuOptions.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/gfx/Renderer.hh>
#include <cstdlib>

struct RelPtr  // for Android trackball
{
	int x = 0, y = 0;
	unsigned xAction = 0, yAction = 0;
};
static RelPtr relPtr{};

std::list<InputDeviceSavedConfig> savedInputDevList{};
std::list<KeyConfig> customKeyConfig{};

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS

#ifdef CONFIG_VCONTROLS_GAMEPAD
static Gfx::GC vControllerGCSize(const SysVController &vController)
{
	return vController.xMMSize(int(optionTouchCtrlSize) / 100.);
}

static int vControllerPixelSize(const SysVController &vController, const Base::Window &win)
{
	return IG::makeEvenRoundedUp(vController.xMMSizeToPixel(win, int(optionTouchCtrlSize) / 100.));
}
#endif

void EmuApp::initVControls()
{
	auto &r = renderer;
	auto &app = vController.app();
	auto &winData = vController.windowData();
	auto &win = vController.window();
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	auto &gp = vController.gamePad();
	gp.dPad().setDeadzone(r, vController.xMMSizeToPixel(win, int(optionTouchDpadDeadzone) / 100.), vController.windowData());
	gp.dPad().setDiagonalSensitivity(r, optionTouchDpadDiagonalSensitivity / 1000., vController.windowData());
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
	vController.init((int)optionTouchCtrlAlpha / 255.0, vControllerPixelSize(vController, win), vController.face().nominalHeight()*1.75, winData.projection.plane());
	#else
	vController.init((int)optionTouchCtrlAlpha / 255.0, IG::makeEvenRoundedUp(vController.xMMSizeToPixel(win, 8.5)), vController.face().nominalHeight()*1.75, winData.projection.plane());
	#endif

	if(!vController.layoutPositionChanged()) // setup default positions if not provided in config file
		resetVControllerPositions(vController);
	vController.setInputPlayer(0);
	app.updateVControlImg(vController);
	vController.setMenuImage(app.asset(EmuApp::AssetID::MENU));
	vController.setFastForwardImage(app.asset(EmuApp::AssetID::FAST_FORWARD));
}

void resetVControllerPositions(VController &vController)
{
	auto app = vController.appContext();
	auto &win = vController.window();
	logMsg("resetting on-screen controls to default positions & states");
	unsigned initFastForwardState = (Config::envIsIOS || (Config::envIsAndroid  && !app.hasHardwareNavButtons()))
		? VControllerLayoutPosition::SHOWN : VControllerLayoutPosition::OFF;
	unsigned initMenuState = (Config::envIsAndroid && app.hasHardwareNavButtons())
		? VControllerLayoutPosition::HIDDEN : VControllerLayoutPosition::SHOWN;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	unsigned initGamepadState = (Config::envIsAndroid || Config::envIsIOS || (int)optionTouchCtrl == 1) ? VControllerLayoutPosition::SHOWN : VControllerLayoutPosition::OFF;
	#else
	unsigned initGamepadState = VControllerLayoutPosition::OFF;
	#endif
	bool isLandscape = true;
	for(auto &e : vController.layoutPosition())
	{
		auto defaultSidePadding = vController.xMMSizeToPixel(win, 4.);
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		int xOffset = isLandscape ? vController.xMMSizeToPixel(win, 2.) : vController.xMMSizeToPixel(win, .5);
		e[VCTRL_LAYOUT_DPAD_IDX] = {LB2DO, {xOffset + vController.bounds(0).xSize()/2, (int)(-vControllerPixelSize(vController, win)) - vController.bounds(0).ySize()/2}, initGamepadState};
		e[VCTRL_LAYOUT_CENTER_BTN_IDX] = {CB2DO, {0, 0}, initGamepadState};
		e[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX] = {RB2DO, {-xOffset - vController.bounds(2).xSize()/2, (int)(-vControllerPixelSize(vController, win)) - vController.bounds(2).ySize()/2}, initGamepadState};
		#endif
		e[VCTRL_LAYOUT_MENU_IDX] = {RT2DO, {-defaultSidePadding, 0}, initMenuState};
		e[VCTRL_LAYOUT_FF_IDX] = {LT2DO, {defaultSidePadding, 0}, initFastForwardState};
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		if(EmuSystem::inputHasTriggerBtns)
		{
			int y = std::min(e[0].pos.y - vController.bounds(0).ySize()/2, e[2].pos.y - vController.bounds(2).ySize()/2);
			y -= vController.bounds(5).ySize()/2 + vController.yMMSizeToPixel(win, 1.);
			e[VCTRL_LAYOUT_L_IDX] = {LB2DO, {xOffset + vController.bounds(5).xSize()/2, y}, initGamepadState};
			e[VCTRL_LAYOUT_R_IDX] = {RB2DO, {-xOffset - vController.bounds(5).xSize()/2, y}, initGamepadState};
		}
		#endif
		isLandscape = false;
	};
	vController.setLayoutPositionChanged(false);
}

void resetVControllerOptions(VController &vController)
{
	resetVControllerPositions(vController);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlBtnSpace.reset();
	optionTouchCtrlBtnStagger.reset();
	#endif
}

void resetAllVControllerOptions(VController &vController, EmuViewController &emuViewController)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrl.reset();
	optionTouchCtrlSize.reset();
	optionTouchDpadDeadzone.reset();
	optionTouchDpadDiagonalSensitivity.reset();
	optionTouchCtrlExtraXBtnSize.reset();
	optionTouchCtrlExtraYBtnSize.reset();
	optionTouchCtrlExtraYBtnSizeMultiRow.reset();
	optionTouchCtrlTriggerBtnPos.reset();
	optionTouchCtrlBoundingBoxes.reset();
	optionVibrateOnPush.reset();
	vController.resetUsesScaledCoordinates();
	optionTouchCtrlShowOnTouch.reset();
	#endif
	resetVControllerOptions(vController);
	optionTouchCtrlAlpha.reset();
	emuViewController.updateAutoOnScreenControlVisible();
	vController.setInputPlayer(0);
}

VControllerLayoutPosition vControllerPixelToLayoutPos(IG::Point2D<int> pos, IG::Point2D<int> size, IG::WindowRect viewBounds)
{
	IG::WindowRect bound {pos - size/2, pos + size/2};

	const auto &rect = viewBounds;
	IG::WindowRect ltQuadrantRect{{rect.x, rect.y}, rect.center()};
	IG::WindowRect rtQuadrantRect{{rect.xCenter(), rect.y}, {rect.x2, rect.yCenter()}};
	IG::WindowRect lbQuadrantRect{{rect.x, rect.yCenter()}, {rect.xCenter(), rect.y2}};
	IG::WindowRect rbQuadrantRect{rect.center(), {rect.x2, rect.y2}};
	bool ltQuadrant = bound.overlaps(ltQuadrantRect);
	bool rtQuadrant = bound.overlaps(rtQuadrantRect);
	bool lbQuadrant = bound.overlaps(lbQuadrantRect);
	bool rbQuadrant = bound.overlaps(rbQuadrantRect);
	_2DOrigin origin = C2DO;
	if(ltQuadrant && rtQuadrant && lbQuadrant && rbQuadrant) origin = C2DO;
	else if(ltQuadrant && rtQuadrant) origin = CT2DO;
	else if(ltQuadrant && lbQuadrant) origin = LC2DO;
	else if(rtQuadrant && rbQuadrant) origin = RC2DO;
	else if(lbQuadrant && rbQuadrant) origin = CB2DO;
	else if(ltQuadrant) origin = LT2DO;
	else if(rtQuadrant) origin = RT2DO;
	else if(lbQuadrant) origin = LB2DO;
	else if(rbQuadrant) origin = RB2DO;

	int x = (origin.xScaler() == 0) ? pos.x - rect.xSize()/2 :
		(origin.xScaler() == 1) ? pos.x - rect.xSize() : pos.x;
	int y = LT2DO.adjustY(pos.y, rect.ySize(), origin);
	return {origin, {x, y}};
}

IG::Point2D<int> vControllerLayoutToPixelPos(VControllerLayoutPosition lPos, Gfx::Viewport viewport)
{
	int x = (lPos.origin.xScaler() == 0) ? lPos.pos.x + viewport.width()/2 :
		(lPos.origin.xScaler() == 1) ? lPos.pos.x + viewport.width() : lPos.pos.x;
	int y = lPos.origin.adjustY(lPos.pos.y, (int)viewport.height(), LT2DO);
	return {x, y};
}

#else

void EmuApp::initVControls() {}

#endif // CONFIG_EMUFRAMEWORK_VCONTROLS

static void processRelPtr(EmuApp &app, Input::Event e)
{
	using namespace IG;
	if(relPtr.x != 0 && sign(relPtr.x) != sign(e.pos().x))
	{
		//logMsg("reversed trackball X direction");
		relPtr.x = e.pos().x;
		EmuSystem::handleInputAction(&app, Input::Action::RELEASED, relPtr.xAction);
	}
	else
		relPtr.x += e.pos().x;

	if(e.pos().x)
	{
		relPtr.xAction = EmuSystem::translateInputAction(e.pos().x > 0 ? EmuControls::systemKeyMapStart+1 : EmuControls::systemKeyMapStart+3);
		EmuSystem::handleInputAction(&app, Input::Action::PUSHED, relPtr.xAction);
	}

	if(relPtr.y != 0 && sign(relPtr.y) != sign(e.pos().y))
	{
		//logMsg("reversed trackball Y direction");
		relPtr.y = e.pos().y;
		EmuSystem::handleInputAction(&app, Input::Action::RELEASED, relPtr.yAction);
	}
	else
		relPtr.y += e.pos().y;

	if(e.pos().y)
	{
		relPtr.yAction = EmuSystem::translateInputAction(e.pos().y > 0 ? EmuControls::systemKeyMapStart+2 : EmuControls::systemKeyMapStart);
		EmuSystem::handleInputAction(&app, Input::Action::PUSHED, relPtr.yAction);
	}

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtr.x, relPtr.y);
}

void TurboInput::update(EmuApp *app)
{
	static const unsigned turboFrames = 4;

	for(auto e : activeAction)
	{
		if(e.action)
		{
			if(clock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(app, Input::Action::PUSHED, e.action);
			}
			else if(clock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(app, Input::Action::RELEASED, e.action);
			}
		}
	}
	clock++;
	if(clock == turboFrames) clock = 0;
}

void commonUpdateInput()
{
#if 0
	auto applyRelPointerDecel =
		[](int val)
		{
			return std::max(std::abs(val) - (int)optionRelPointerDecel, 0) * IG::sign(val);
		};

	if(relPtr.x)
	{
		relPtr.x = applyRelPointerDecel(relPtr.x);
		if(!relPtr.x)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	if(relPtr.y)
	{
		relPtr.y = applyRelPointerDecel(relPtr.y);
		if(!relPtr.y)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
#endif
}

void EmuApp::updateInputDevices(Base::ApplicationContext ctx)
{
	int i = 0;
	inputDevConf.clear();
	for(auto &e : ctx.inputDevices())
	{
		logMsg("input device %d: name: %s, id: %d, map: %d", i, e->name(), e->enumId(), (int)e->map());
		inputDevConf.emplace_back(e);
		for(auto &saved : savedInputDevList)
		{
			if(saved.matchesDevice(*e))
			{
				logMsg("has saved config");
				inputDevConf.back().setSavedConf(&saved);
			}
		}
		i++;
	}
	onUpdateInputDevices_.callCopySafe();
	keyMapping.buildAll(inputDevConf, ctx.inputDevices());
}

void EmuApp::setOnUpdateInputDevices(DelegateFunc<void ()> del)
{
	if(del)
	{
		assert(!onUpdateInputDevices_);
	}
	onUpdateInputDevices_ = del;
}

// KeyConfig

const KeyConfig &KeyConfig::defaultConfigForDevice(const Input::Device &dev)
{
	switch(dev.map())
	{
		default: return defaultConfigsForDevice(dev)[0];
		case Input::Map::SYSTEM:
		{
			#if defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_X11
			unsigned confs = 0;
			auto conf = defaultConfigsForDevice(dev, confs);
			iterateTimes(confs, i)
			{
				// Look for the first config to match the device subtype
				if(dev.subtype() == conf[i].devSubtype)
				{
					return conf[i];
				}
			}
			#endif
			return defaultConfigsForDevice(dev)[0];
		}
	}
}

const KeyConfig *KeyConfig::defaultConfigsForInputMap(Input::Map map, unsigned &size)
{
	switch(map)
	{
		default: return nullptr;
		case Input::Map::SYSTEM:
			size = EmuControls::defaultKeyProfiles;
			return EmuControls::defaultKeyProfile;
		#ifdef CONFIG_BLUETOOTH
		case Input::Map::WIIMOTE:
			size = EmuControls::defaultWiimoteProfiles;
			return EmuControls::defaultWiimoteProfile;
		case Input::Map::WII_CC:
			size = EmuControls::defaultWiiCCProfiles;
			return EmuControls::defaultWiiCCProfile;
		case Input::Map::ICONTROLPAD:
			size = EmuControls::defaultIControlPadProfiles;
			return EmuControls::defaultIControlPadProfile;
		case Input::Map::ZEEMOTE:
			size = EmuControls::defaultZeemoteProfiles;
			return EmuControls::defaultZeemoteProfile;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Input::Map::PS3PAD:
			size = EmuControls::defaultPS3Profiles;
			return EmuControls::defaultPS3Profile;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Map::ICADE:
			size = EmuControls::defaultICadeProfiles;
			return EmuControls::defaultICadeProfile;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Input::Map::APPLE_GAME_CONTROLLER:
			size = EmuControls::defaultAppleGCProfiles;
			return EmuControls::defaultAppleGCProfile;
		#endif
	}
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev, unsigned &size)
{
	auto conf = defaultConfigsForInputMap(dev.map(), size);
	if(!conf)
	{
		bug_unreachable("device map:%d missing default configs", (int)dev.map());
		return nullptr;
	}
	return conf;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev)
{
	unsigned size;
	return defaultConfigsForDevice(dev, size);
}

// InputDeviceConfig

template <size_t S>
void uniqueCustomConfigName(char (&name)[S])
{
	iterateTimes(99, i) // Try up to "Custom 99"
	{
		string_printf(name, "Custom %d", i+1);
		// Check if this name is free
		logMsg("checking %s", name);
		bool exists = 0;
		for(auto &e : customKeyConfig)
		{
			logMsg("against %s", e.name);
			if(string_equal(e.name, name))
			{
				exists = 1;
				break;
			}
		}
		if(!exists)
			break;
	}
	logMsg("unique custom key config name: %s", name);
}

void InputDeviceConfig::deleteConf()
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name);
		auto removed = IG::eraseFirst(savedInputDevList, *savedConf);
		assert(removed);
		savedConf = nullptr;
	}
}

#ifdef CONFIG_INPUT_ICADE
bool InputDeviceConfig::setICadeMode(bool on)
{
	// delete device's config since its properties will change with iCade mode switch
	deleteConf();
	dev->setICadeMode(on);
	save();
	if(!savedConf)
	{
		logErr("can't save iCade mode");
		return 0;
	}
	savedConf->iCadeMode = on;
	return 1;
}

bool InputDeviceConfig::iCadeMode()
{
	return dev->iCadeMode();
}
#endif

unsigned InputDeviceConfig::joystickAxisAsDpadBits()
{
	return dev->joystickAxisAsDpadBits();
}

void InputDeviceConfig::setJoystickAxisAsDpadBits(unsigned axisMask)
{
	dev->setJoystickAxisAsDpadBits(axisMask);
}

const KeyConfig &InputDeviceConfig::keyConf() const
{
	if(savedConf && savedConf->keyConf)
	{
		//logMsg("has saved config %p", savedConf->keyConf_);
		return *savedConf->keyConf;
	}
	assert(dev);
	return KeyConfig::defaultConfigForDevice(*dev);
}

void InputDeviceConfig::setKeyConf(const KeyConfig &kConf)
{
	save();
	savedConf->keyConf = &kConf;
}

void InputDeviceConfig::setDefaultKeyConf()
{
	if(savedConf)
	{
		savedConf->keyConf = nullptr;
	}
}

KeyConfig *InputDeviceConfig::mutableKeyConf()
{
	auto currConf = &keyConf();
	//logMsg("curr key config %p", currConf);
	for(auto &e : customKeyConfig)
	{
		//logMsg("checking key config %p", &e);
		if(&e == currConf)
		{
			return &e;
		}
	}
	return nullptr;
}

KeyConfig *InputDeviceConfig::makeMutableKeyConf(EmuApp &app)
{
	auto conf = mutableKeyConf();
	if(!conf)
	{
		logMsg("current config not mutable, creating one");
		char name[96];
		uniqueCustomConfigName(name);
		conf = setKeyConfCopiedFromExisting(name);
		app.printfMessage(3, false, "Automatically created profile: %s", conf->name);
	}
	return conf;
}

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(const char *name)
{
	customKeyConfig.emplace_back();
	auto &newConf = customKeyConfig.back();
	newConf = keyConf();
	string_copy(newConf.name, name);
	setKeyConf(newConf);
	return &newConf;
}

void InputDeviceConfig::save()
{
	if(!savedConf)
	{
		savedInputDevList.emplace_back();
		logMsg("allocated new device config, %d total", (int)savedInputDevList.size());
		savedConf = &savedInputDevList.back();
		*savedConf = InputDeviceSavedConfig();
	}
	savedConf->player = player;
	savedConf->enabled = enabled;
	savedConf->enumId = dev->enumId();
	savedConf->joystickAxisAsDpadBits = dev->joystickAxisAsDpadBits();
	#ifdef CONFIG_INPUT_ICADE
	savedConf->iCadeMode = dev->iCadeMode();
	#endif
	string_copy(savedConf->name, dev->name());
}

void InputDeviceConfig::setSavedConf(InputDeviceSavedConfig *savedConf)
{
	this->savedConf = savedConf;
	if(savedConf)
	{
		player = savedConf->player;
		enabled = savedConf->enabled;
		dev->setJoystickAxisAsDpadBits(savedConf->joystickAxisAsDpadBits);
		#ifdef CONFIG_INPUT_ICADE
		dev->setICadeMode(savedConf->iCadeMode);
		#endif
	}
}

bool InputDeviceConfig::setKey(EmuApp &app, Input::Key mapKey, const KeyCategory &cat, int keyIdx)
{
	auto conf = makeMutableKeyConf(app);
	if(!conf)
		return false;
	auto &keyEntry = conf->key(cat)[keyIdx];
	logMsg("changing key mapping from %s (0x%X) to %s (0x%X)",
			dev->keyName(keyEntry), keyEntry, dev->keyName(mapKey), mapKey);
	keyEntry = mapKey;
	return true;
}

// KeyMapping

void KeyMapping::buildAll(const std::vector<InputDeviceConfig> &inputDevConf, const Base::InputDeviceContainer &inputDevices)
{
	if(!inputDevConf.size())
		return;
	assert(inputDevConf.size() == inputDevices.size());
	inputDevActionTable.resize(0);
	inputDevActionTablePtr = std::make_unique<ActionGroup*[]>(inputDevices.size());
	// calculate & allocate complete map including all devices
	{
		unsigned totalKeys = 0;
		for(auto &e : inputDevices)
		{
			totalKeys += Input::Event::mapNumKeys(e->map());
		}
		if(!totalKeys) [[unlikely]]
		{
			logMsg("no keys in mapping");
			inputDevActionTablePtr[0] = nullptr;
			return;
		}
		logMsg("allocating key mapping with %d keys", totalKeys);
		inputDevActionTable.resize(totalKeys);
	}
	unsigned totalKeys = 0;
	int i = 0;
	for(auto &e : inputDevices)
	{
		// set the offset for this device from the full action table
		inputDevActionTablePtr[i] = &inputDevActionTable[totalKeys];
		auto mapKeys = Input::Event::mapNumKeys(e->map());
		totalKeys += mapKeys;
		auto actionGroup = inputDevActionTablePtr[i];
		if(!inputDevConf[i].enabled)
		{
			i++;
			continue;
		}
		KeyConfig::KeyArray key = inputDevConf[i].keyConf().key();
		if(inputDevConf[i].player != InputDeviceConfig::PLAYER_MULTI)
		{
			logMsg("transposing keys for player %d", inputDevConf[i].player+1);
			EmuControls::transposeKeysForPlayer(key, inputDevConf[i].player);
		}
		iterateTimes(MAX_KEY_CONFIG_KEYS, k)
		{
			//logMsg("mapping key %d to %u %s", k, key, Input::buttonName(inputDevConf[i].dev->map, key[k]));
			assert(key[k] < Input::Event::mapNumKeys(e->map()));
			auto &group = actionGroup[key[k]];
			auto slot = IG::find_if(group, [](Action a){ return a == 0; });
			if(slot != group.end())
				*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
		}

		i++;
	}
}

void KeyMapping::free()
{
	inputDevActionTable.resize(0);
	inputDevActionTablePtr.reset();
}

KeyMapping::operator bool() const
{
	return (bool)inputDevActionTablePtr;
}

namespace EmuControls
{

void generic2PlayerTranspose(KeyConfig::KeyArray &key, unsigned player, unsigned startCategory)
{
	if(player == 0)
	{
		// clear P2 joystick keys
		std::fill_n(&key[category[startCategory+1].configOffset], category[startCategory+1].keys, 0);
	}
	else
	{
		// transpose joystick keys
		std::copy_n(&key[category[startCategory].configOffset], category[startCategory].keys, &key[category[startCategory+1].configOffset]);
		std::fill_n(&key[category[startCategory].configOffset], category[startCategory].keys, 0);
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, unsigned player, unsigned startCategory)
{
	iterateTimes(EmuSystem::maxPlayers, i)
	{
		if(player && i == player)
		{
			//logMsg("moving to player %d map", i);
			std::copy_n(&key[category[startCategory].configOffset], category[startCategory].keys, &key[category[i+startCategory].configOffset]);
			std::fill_n(&key[category[startCategory].configOffset], category[startCategory].keys, 0);
		}
		else if(i)
		{
			//logMsg("clearing player %d map", i);
			std::fill_n(&key[category[i+startCategory].configOffset], category[i+startCategory].keys, 0);
		}
	}
}

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void setupVControllerVars(VController &vController)
{
	auto &winData = vController.windowData();
	auto &win = vController.window();
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	Gfx::GC btnSize = vControllerGCSize(vController);
	int btnSizePixels = vControllerPixelSize(vController, win);
	auto &gp = vController.gamePad();
	logMsg("set on-screen button size: %f, %d pixels", (double)btnSize, btnSizePixels);
	gp.setSpacing(vController.xMMSize(int(optionTouchCtrlBtnSpace) / 100.));
	gp.setSpacingPixels(IG::makeEvenRoundedUp(vController.xMMSizeToPixel(win, int(optionTouchCtrlBtnSpace) / 100.)));
	gp.setRowShift(0);
	gp.setRowShiftPixels(0);
	gp.setExtraXSize(optionTouchCtrlExtraXBtnSize / 1000.);
	gp.setExtraYSize(optionTouchCtrlExtraYBtnSize / 1000.);
	gp.setExtraYSizeMultiRow(optionTouchCtrlExtraYBtnSizeMultiRow / 1000.);
	gp.setTriggersInline(optionTouchCtrlTriggerBtnPos);
	switch((int)optionTouchCtrlBtnStagger)
	{
		bcase 0:
			gp.setStagger(btnSize * -.75);
			gp.setStaggerPixels(btnSizePixels * -.75);
		bcase 1:
			gp.setStagger(btnSize * -.5);
			gp.setStaggerPixels(btnSizePixels * -.5);
		bcase 2:
			gp.setStagger(0);
			gp.setStaggerPixels(0);
		bcase 3:
			gp.setStagger(btnSize * .5);
			gp.setStaggerPixels(btnSizePixels * .5);
		bcase 4:
			gp.setStagger(btnSize * .75);
			gp.setStaggerPixels(btnSizePixels * .75);
		bdefault:
			gp.setStagger(btnSize + gp.spacing());
			gp.setStaggerPixels(btnSizePixels + gp.spacingPixels());
			gp.setRowShift(-(btnSize + gp.spacing()));
			gp.setRowShiftPixels(-(btnSizePixels + gp.spacingPixels()));
	}
	vController.setBaseBtnSize(vControllerPixelSize(vController, win), vController.face().nominalHeight()*1.75, winData.projection.plane());
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
	#else
	vController.init((int)optionTouchCtrlAlpha / 255.0, IG::makeEvenRoundedUp(vController.xMMSizeToPixel(win, 8.5)), vController.face().nominalHeight()*1.75, winData.projection.plane());
	#endif

	auto &layoutPos = vController.layoutPosition()[winData.viewport().isPortrait() ? 1 : 0];
	iterateTimes(vController.numElements(), i)
	{
		vController.setPos(i, vControllerLayoutToPixelPos(layoutPos[i], winData.viewport()));
		vController.setState(i, layoutPos[i].state);
	}
}
#endif

}

void EmuApp::updateVControlImg(VController &vController)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	{
		vController.setImg(asset(AssetID::GAMEPAD_OVERLAY));
	}
	#endif
	if(EmuSystem::inputHasKeyboard)
	{
		vController.setKeyboardImage(asset(AssetID::KEYBOARD_OVERLAY));
	}
}

void EmuApp::setActiveFaceButtons(unsigned btns)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	auto &vController = defaultVController();
	vController.gamePad().setActiveFaceButtons(btns);
	if(!vController.hasWindow())
		return;
	EmuControls::setupVControllerVars(vController);
	vController.place();
	EmuSystem::clearInputBuffers(emuViewController->inputView());
	#endif
}

void EmuApp::updateKeyboardMapping()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	defaultVController().updateKeyboardMapping();
	#endif
}

void EmuApp::toggleKeyboard()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	defaultVController().toggleKeyboard();
	#endif
}

void EmuApp::updateVControllerMapping()
{
	defaultVController().updateMapping();
}

void TurboInput::addEvent(unsigned action)
{
	Action *slot = IG::find_if(activeAction, [](Action a){ return a == 0; });
	if(slot != activeAction.end())
	{
		slot->action = action;
		logMsg("added turbo event action %d", action);
	}
}

void TurboInput::removeEvent(unsigned action)
{
	for(auto &e : activeAction)
	{
		if(e.action == action)
		{
			e.action = 0;
			logMsg("removed turbo event action %d", action);
		}
	}
}

bool KeyConfig::operator ==(KeyConfig const& rhs) const
{
	return string_equal(name, rhs.name);
}

KeyConfig::Key *KeyConfig::key(const KeyCategory &category)
{
	assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

const KeyConfig::Key *KeyConfig::key(const KeyCategory &category) const
{
	assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

void KeyConfig::unbindCategory(const KeyCategory &category)
{
	std::fill_n(key(category), category.keys, 0);
}
