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

#include <imagine/data-type/image/sys.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/InputManagerView.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <emuframework/VController.hh>
SysVController vController;
uint pointerInputPlayer = 0;
#endif

struct RelPtr  // for Android trackball
{
	int x, y;
	uint xAction, yAction;
};
static RelPtr relPtr{};

TurboInput turboActions;
uint inputDevConfs = 0;
InputDeviceConfig inputDevConf[Input::MAX_DEVS];
StaticDLList<InputDeviceSavedConfig, MAX_SAVED_INPUT_DEVICES> savedInputDevList;
KeyMapping keyMapping;
StaticDLList<KeyConfig, MAX_CUSTOM_KEY_CONFIGS> customKeyConfig;
bool physicalControlsPresent = false;
bool touchControlsAreOn = false;
VControllerLayoutPosition vControllerLayoutPos[2][7];
bool vControllerLayoutPosChanged = false;
bool fastForwardActive = false;

#ifdef CONFIG_VCONTROLS_GAMEPAD
static Gfx::GC vControllerGCSize()
{
	return vController.xMMSize(int(optionTouchCtrlSize) / 100.);
}

static int vControllerPixelSize()
{
	return IG::makeEvenRoundedUp(vController.xMMSizeToPixel(mainWin.win, int(optionTouchCtrlSize) / 100.));
}
#endif

void initVControls()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.dp.setDeadzone(vController.xMMSizeToPixel(mainWin.win, int(optionTouchDpadDeadzone) / 100.));
	vController.gp.dp.setDiagonalSensitivity(optionTouchDpadDiagonalSensitivity / 1000.);
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
	vController.init((int)optionTouchCtrlAlpha / 255.0, vControllerPixelSize(), View::defaultFace->nominalHeight()*1.75, mainWin.projectionPlane);
	#else
	vController.init((int)optionTouchCtrlAlpha / 255.0, IG::makeEvenRoundedUp(vController.xMMSizeToPixel(mainWin.win, 8.5)), View::defaultFace->nominalHeight()*1.75, mainWin.projectionPlane);
	#endif

	if(!vControllerLayoutPosChanged) // setup default positions if not provided in config file
		resetVControllerPositions();
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if((int)optionTouchCtrl == 2)
		EmuControls::updateAutoOnScreenControlVisible();
	else
		EmuControls::setOnScreenControls(optionTouchCtrl);
	#endif
	vController.updateMapping(0);
}

void resetVControllerPositions()
{
	logMsg("resetting on-screen controls to default positions & states");
	auto &win = mainWin.win;
	uint initFastForwardState = (Config::envIsIOS || (Config::envIsAndroid  && !Base::hasHardwareNavButtons()) || Config::envIsWebOS3)
		? VControllerLayoutPosition::SHOWN : VControllerLayoutPosition::OFF;
	uint initMenuState = ((Config::envIsWebOS && !Config::envIsWebOS3) || (Config::envIsAndroid && Base::hasHardwareNavButtons()))
		? VControllerLayoutPosition::HIDDEN : VControllerLayoutPosition::SHOWN;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	uint initGamepadState = (Config::envIsWebOS || Config::envIsAndroid || Config::envIsIOS || (int)optionTouchCtrl == 1) ? VControllerLayoutPosition::SHOWN : VControllerLayoutPosition::OFF;
	#else
	uint initGamepadState = VControllerLayoutPosition::OFF;
	#endif
	bool isLandscape = true;
	for(auto &e : vControllerLayoutPos)
	{
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		int xOffset = isLandscape ? vController.xMMSizeToPixel(win, 2.) : vController.xMMSizeToPixel(win, .5);
		e[VCTRL_LAYOUT_DPAD_IDX] = {LB2DO, {xOffset + vController.bounds(0).xSize()/2, (int)(-vControllerPixelSize()) - vController.bounds(0).ySize()/2}, initGamepadState};
		e[VCTRL_LAYOUT_CENTER_BTN_IDX] = {CB2DO, {0, 0}, initGamepadState};
		e[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX] = {RB2DO, {-xOffset - vController.bounds(2).xSize()/2, (int)(-vControllerPixelSize()) - vController.bounds(2).ySize()/2}, initGamepadState};
		#endif
		e[VCTRL_LAYOUT_MENU_IDX] = {RT2DO, {0, 0}, initMenuState};
		e[VCTRL_LAYOUT_FF_IDX] = {LT2DO, {0, 0}, initFastForwardState};
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
	vControllerLayoutPosChanged = false;
}

void resetVControllerOptions()
{
	resetVControllerPositions();
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlBtnSpace.reset();
	optionTouchCtrlBtnStagger.reset();
	#endif
}

void resetAllVControllerOptions()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrl.reset();
	pointerInputPlayer = 0;
	optionTouchCtrlSize.reset();
	optionTouchDpadDeadzone.reset();
	optionTouchDpadDiagonalSensitivity.reset();
	optionTouchCtrlExtraXBtnSize.reset();
	optionTouchCtrlExtraYBtnSize.reset();
	optionTouchCtrlExtraYBtnSizeMultiRow.reset();
	optionTouchCtrlTriggerBtnPos.reset();
	optionTouchCtrlBoundingBoxes.reset();
	optionVibrateOnPush.reset();
		#ifdef CONFIG_BASE_ANDROID
		optionTouchCtrlScaledCoordinates.reset();
		#endif
	optionTouchCtrlShowOnTouch.reset();
	#endif
	resetVControllerOptions();
	optionTouchCtrlAlpha.reset();
	EmuControls::updateAutoOnScreenControlVisible();
	vController.updateMapping(pointerInputPlayer);
}

VControllerLayoutPosition vControllerPixelToLayoutPos(IG::Point2D<int> pos, IG::Point2D<int> size)
{
	auto &win = mainWin.win;
	IG::WindowRect bound { pos.x - size.x/2, pos.y - size.y/2, pos.x + size.x/2, pos.y + size.y/2 };

	const auto &viewport = mainWin.viewport();
	const auto &rect = viewport.bounds();
	IG::WindowRect ltQuadrantRect{rect.x, rect.y, rect.xCenter(), rect.yCenter()};
	IG::WindowRect rtQuadrantRect{rect.xCenter(), rect.y, rect.x2, rect.yCenter()};
	IG::WindowRect lbQuadrantRect{rect.x, rect.yCenter(), rect.xCenter(), rect.y2};
	IG::WindowRect rbQuadrantRect{rect.xCenter(), rect.yCenter(), rect.x2, rect.y2};
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

	int x = (origin.xScaler() == 0) ? pos.x - viewport.width()/2 :
		(origin.xScaler() == 1) ? pos.x - viewport.width() : pos.x;
	int y = LT2DO.adjustY(pos.y, (int)viewport.height(), origin);
	return {origin, {x, y}};
}

IG::Point2D<int> vControllerLayoutToPixelPos(VControllerLayoutPosition lPos)
{
	const auto &viewport = mainWin.viewport();
	int x = (lPos.origin.xScaler() == 0) ? lPos.pos.x + viewport.width()/2 :
		(lPos.origin.xScaler() == 1) ? lPos.pos.x + viewport.width() : lPos.pos.x;
	int y = lPos.origin.adjustY(lPos.pos.y, (int)viewport.height(), LT2DO);
	return {x, y};
}

void processRelPtr(const Input::Event &e)
{
	using namespace IG;
	if(relPtr.x != 0 && signOf(relPtr.x) != signOf(e.x))
	{
		//logMsg("reversed trackball X direction");
		relPtr.x = e.x;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	else
		relPtr.x += e.x;

	if(e.x)
	{
		relPtr.xAction = EmuSystem::translateInputAction(e.x > 0 ? EmuControls::systemKeyMapStart+1 : EmuControls::systemKeyMapStart+3);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.xAction);
	}

	if(relPtr.y != 0 && signOf(relPtr.y) != signOf(e.y))
	{
		//logMsg("reversed trackball Y direction");
		relPtr.y = e.y;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
	else
		relPtr.y += e.y;

	if(e.y)
	{
		relPtr.yAction = EmuSystem::translateInputAction(e.y > 0 ? EmuControls::systemKeyMapStart+2 : EmuControls::systemKeyMapStart);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.yAction);
	}

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtr.x, relPtr.y);
}

void commonInitInput()
{
	mem_zero(relPtr);
	mem_zero(turboActions);
	fastForwardActive = false;
}

void commonUpdateInput()
{
	using namespace IG;
	static const uint turboFrames = 4;
	static uint turboClock = 0;

	forEachInArray(turboActions.activeAction, e)
	{
		if(e->action)
		{
			if(turboClock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(Input::PUSHED, e->action);
			}
			else if(turboClock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(Input::RELEASED, e->action);
			}
		}
	}
	turboClock++;
	if(turboClock == turboFrames) turboClock = 0;

#ifdef CONFIG_INPUT_RELATIVE_MOTION_DEVICES
	auto applyRelPointerDecel =
		[](int val)
		{
			return std::max(std::abs(val) - (int)optionRelPointerDecel, 0) * IG::signOf(val);
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

bool isMenuDismissKey(const Input::Event &e)
{
	using namespace Input;
	Key dismissKey = Keycode::MENU;
	Key dismissKey2 = Keycode::GAME_Y;
	if(Config::envIsWebOS1)
		dismissKey = Keycode::RCTRL;
	if(Config::MACHINE_IS_PANDORA && e.device->subtype() == Device::SUBTYPE_PANDORA_HANDHELD)
	{
		if(modalViewController.hasView()) // make sure not performing text input
			return false;
		dismissKey = Keycode::SPACE;
	}
	return e.key() == dismissKey || e.key() == dismissKey2;
}

void updateInputDevices()
{
	using namespace Input;
	int i = 0;
	for(auto &e : devList)
	{
		logMsg("input device %d: name: %s, id: %d, map: %d", i, e->name(), e->enumId(), e->map());
		inputDevConf[i].dev = e;
		inputDevConf[i].reset();
		for(auto &saved : savedInputDevList)
		{
			if(saved.matchesDevice(*e))
			{
				logMsg("has saved config");
				inputDevConf[i].setSavedConf(&saved);
			}
		}
		i++;
	}
	inputDevConfs = i;
	physicalControlsPresent = keyInputIsPresent();
	if(physicalControlsPresent)
	{
		logMsg("Physical controls are present");
	}
	onUpdateInputDevices.callCopySafe();
	keyMapping.buildAll();
}

// KeyConfig

const KeyConfig &KeyConfig::defaultConfigForDevice(const Input::Device &dev)
{
	switch(dev.map())
	{
		case Input::Event::MAP_SYSTEM:
		{
			#if defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_X11
			uint confs = 0;
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
	return defaultConfigsForDevice(dev)[0];
}

const KeyConfig *KeyConfig::defaultConfigsForInputMap(uint map, uint &size)
{
	switch(map)
	{
		case Input::Event::MAP_SYSTEM:
			size = EmuControls::defaultKeyProfiles;
			return EmuControls::defaultKeyProfile;
		#ifdef CONFIG_BLUETOOTH
		case Input::Event::MAP_WIIMOTE:
			size = EmuControls::defaultWiimoteProfiles;
			return EmuControls::defaultWiimoteProfile;
		case Input::Event::MAP_WII_CC:
			size = EmuControls::defaultWiiCCProfiles;
			return EmuControls::defaultWiiCCProfile;
		case Input::Event::MAP_ICONTROLPAD:
			size = EmuControls::defaultIControlPadProfiles;
			return EmuControls::defaultIControlPadProfile;
		case Input::Event::MAP_ZEEMOTE:
			size = EmuControls::defaultZeemoteProfiles;
			return EmuControls::defaultZeemoteProfile;
		#endif
		#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH_SERVER
		case Input::Event::MAP_PS3PAD:
			size = EmuControls::defaultPS3Profiles;
			return EmuControls::defaultPS3Profile;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Event::MAP_ICADE:
			size = EmuControls::defaultICadeProfiles;
			return EmuControls::defaultICadeProfile;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Input::Event::MAP_APPLE_GAME_CONTROLLER:
			size = EmuControls::defaultAppleGCProfiles;
			return EmuControls::defaultAppleGCProfile;
		#endif
	}
	return nullptr;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev, uint &size)
{
	auto conf = defaultConfigsForInputMap(dev.map(), size);
	if(!conf)
	{
		bug_exit("device type %d missing default configs", dev.map());
		return nullptr;
	}
	return conf;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev)
{
	uint size;
	return defaultConfigsForDevice(dev, size);
}

// InputDeviceConfig

void InputDeviceConfig::reset()
{
	assert(dev);
	savedConf = nullptr;
	player = dev->enumId() < EmuSystem::maxPlayers ? dev->enumId() : 0;
	enabled = 1;
}

void InputDeviceConfig::deleteConf()
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name);
		auto removed = savedInputDevList.remove(*savedConf);
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

uint8 InputDeviceConfig::joystickAxisAsDpadBits()
{
	return dev->joystickAxisAsDpadBits();
}

void InputDeviceConfig::setJoystickAxisAsDpadBits(uint8 axisMask)
{
	dev->setJoystickAxisAsDpadBits(axisMask);
}

const KeyConfig &InputDeviceConfig::keyConf()
{
	if(savedConf && savedConf->keyConf)
	{
		//logMsg("has saved config %p", savedConf->keyConf_);
		return *savedConf->keyConf;
	}
	assert(dev);
	return KeyConfig::defaultConfigForDevice(*dev);
}

bool InputDeviceConfig::setKeyConf(const KeyConfig &kConf)
{
	save();
	if(!savedConf)
	{
		logErr("can't set key config");
		return 0;
	}
	savedConf->keyConf = &kConf;
	return 1;
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

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(const char *name)
{
	if(!customKeyConfig.addToEnd())
	{
		bug_exit("should no be called with full key config list");
		return nullptr;
	}
	auto &newConf = customKeyConfig.back();
	newConf = keyConf();
	string_copy(newConf.name, name);
	if(!setKeyConf(newConf))
	{
		// No space left for new device settings
		customKeyConfig.pop_back();
		return nullptr;
	}
	return &newConf;
}

void InputDeviceConfig::save()
{
	if(!savedConf)
	{
		if(!savedInputDevList.addToEnd())
		{
			logWarn("no more space for device configs");
			return;
		}
		logMsg("allocated new device config, %d total", savedInputDevList.size());
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
	var_selfs(savedConf);
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

// KeyMapping

void KeyMapping::buildAll()
{
	assert(inputDevConfs == Input::devList.size());
	// calculate & allocate complete map including all devices
	{
		uint totalKeys = 0;
		for(auto &e : Input::devList)
		{
			totalKeys += Input::Event::mapNumKeys(e->map());
		}
		mem_free(inputDevActionTablePtr[0]);
		inputDevActionTablePtr[0] = nullptr;
		if(unlikely(!totalKeys))
		{
			logMsg("no keys in mapping");
			return;
		}
		logMsg("allocating key mapping with %d keys", totalKeys);
		inputDevActionTablePtr[0] = (ActionGroup*)mem_calloc(totalKeys * sizeof(ActionGroup));
	}
	uint totalKeys = 0;
	int i = 0;
	for(auto &e : Input::devList)
	{
		if(i)
		{
			// [0] is the base pointer to the allocated map, subsequent elements
			// point to an offset within it
			inputDevActionTablePtr[i] = &inputDevActionTablePtr[0][totalKeys];
		}
		auto mapKeys = Input::Event::mapNumKeys(e->map());
		totalKeys += mapKeys;
		auto actionGroup = inputDevActionTablePtr[i];
		if(!inputDevConf[i].enabled)
		{
			i++;
			continue;
		}
		KeyConfig::KeyArray key;
		memcpy(key, inputDevConf[i].keyConf().key(), sizeof(key));
		if(inputDevConf[i].player != InputDeviceConfig::PLAYER_MULTI)
		{
			logMsg("transposing keys for player %d", inputDevConf[i].player+1);
			EmuControls::transposeKeysForPlayer(key, inputDevConf[i].player);
		}
		iterateTimes(MAX_KEY_CONFIG_KEYS, k)
		{
			//logMsg("mapping key %d to %u %s", k, key, Input::buttonName(inputDevConf[i].dev->map, key[k]));
			assert(key[k] < Input::Event::mapNumKeys(e->map()));
			auto slot = IG::mem_findFirstZeroValue(actionGroup[key[k]]);
			if(slot)
				*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
		}

		i++;
	}
}

namespace EmuControls
{

void generic2PlayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory)
{
	if(player == 0)
	{
		// clear P2 joystick keys
		mem_zero(&key[category[startCategory+1].configOffset], category[startCategory+1].keys * sizeof(KeyConfig::Key));
	}
	else
	{
		// transpose joystick keys
		memcpy(&key[category[startCategory+1].configOffset], &key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
		mem_zero(&key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory)
{
	iterateTimes(EmuSystem::maxPlayers, i)
	{
		if(player && i == player)
		{
			//logMsg("moving to player %d map", i);
			memcpy(&key[category[i+startCategory].configOffset], &key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
			mem_zero(&key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
		}
		else if(i)
		{
			//logMsg("clearing player %d map", i);
			mem_zero(&key[category[i+startCategory].configOffset], category[i+startCategory].keys * sizeof(KeyConfig::Key));
		}
	}
}

void setupVolKeysInGame()
{
	#if defined __ANDROID__
	iterateTimes(inputDevConfs, i)
	{
		if(!inputDevConf[i].enabled ||
			inputDevConf[i].dev->map() != Input::Event::MAP_SYSTEM ||
			!inputDevConf[i].savedConf) // no default configs use volume keys
		{
			continue;
		}
		auto key = inputDevConf[i].keyConf().key();
		iterateTimes(MAX_KEY_CONFIG_KEYS, k)
		{
			if(Input::isVolumeKey(key[k]))
			{
				logMsg("key config has volume keys");
				Input::setHandleVolumeKeys(1);
				return;
			}
		}
	}
	#endif
}

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void setupVControllerVars()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	Gfx::GC btnSize = vControllerGCSize();
	int btnSizePixels = vControllerPixelSize();
	logMsg("set on-screen button size: %f, %d pixels", (double)btnSize, btnSizePixels);
	vController.gp.btnSpace = vController.xMMSize(int(optionTouchCtrlBtnSpace) / 100.);
	vController.gp.btnSpacePixels = IG::makeEvenRoundedUp(vController.xMMSizeToPixel(mainWin.win, int(optionTouchCtrlBtnSpace) / 100.));
	vController.gp.btnRowShift = 0;
	vController.gp.btnRowShiftPixels = 0;
	vController.gp.btnExtraXSize = optionTouchCtrlExtraXBtnSize / 1000.;
	vController.gp.btnExtraYSize = optionTouchCtrlExtraYBtnSize / 1000.;
	vController.gp.btnExtraYSizeMultiRow = optionTouchCtrlExtraYBtnSizeMultiRow / 1000.;
	vController.gp.triggersInline = optionTouchCtrlTriggerBtnPos;
	switch((int)optionTouchCtrlBtnStagger)
	{
		bcase 0:
			vController.gp.btnStagger = btnSize * -.75;
			vController.gp.btnStaggerPixels = btnSizePixels * -.75;
		bcase 1:
			vController.gp.btnStagger = btnSize * -.5;
			vController.gp.btnStaggerPixels = btnSizePixels * -.5;
		bcase 2:
			vController.gp.btnStagger = 0;
			vController.gp.btnStaggerPixels = 0;
		bcase 3:
			vController.gp.btnStagger = btnSize * .5;
			vController.gp.btnStaggerPixels = btnSizePixels * .5;
		bcase 4:
			vController.gp.btnStagger = btnSize * .75;
			vController.gp.btnStaggerPixels = btnSizePixels * .75;
		bdefault:
			vController.gp.btnStagger = btnSize + vController.gp.btnSpace;
			vController.gp.btnStaggerPixels = btnSizePixels + vController.gp.btnSpacePixels;
			vController.gp.btnRowShift = -(btnSize + vController.gp.btnSpace);
			vController.gp.btnRowShiftPixels = -(btnSizePixels + vController.gp.btnSpacePixels);
	}
	vController.setBaseBtnSize(vControllerPixelSize(), View::defaultFace->nominalHeight()*1.75, mainWin.projectionPlane);
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
	#else
	vController.init((int)optionTouchCtrlAlpha / 255.0, IG::makeEvenRoundedUp(vController.xMMSizeToPixel(mainWin.win, 8.5)), View::defaultFace->nominalHeight()*1.75, mainWin.projectionPlane);
	#endif

	auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
	iterateTimes(vController.numElements(), i)
	{
		vController.setPos(i, vControllerLayoutToPixelPos(layoutPos[i]));
		vController.setState(i, layoutPos[i].state);
	}
}
#endif

void setOnScreenControls(bool on)
{
	touchControlsAreOn = on;
	placeEmuViews();
}

void updateAutoOnScreenControlVisible()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if((uint)optionTouchCtrl == 2)
	{
		if(touchControlsAreOn && physicalControlsPresent)
		{
			logMsg("auto-turning off on-screen controls");
			setOnScreenControls(0);
		}
		else if(!touchControlsAreOn && !physicalControlsPresent)
		{
			logMsg("auto-turning on on-screen controls");
			setOnScreenControls(1);
		}
	}
	#endif
}

/*static const _2DOrigin allCornersO[] = { RT2DO, RC2DO, RB2DO, CB2DO, LB2DO, LC2DO, LT2DO, CT2DO };
static const _2DOrigin onlyTopBottomO[] = { RT2DO, RB2DO, CB2DO, LB2DO, LT2DO, CT2DO };
template <size_t S, size_t S2>
static _2DOrigin getFreeOnScreenSpace(const _2DOrigin(&occupiedCorner)[S], const _2DOrigin(&wantedCorner)[S2])
{
	forEachInArray(wantedCorner, e)
	{
		if(!equalsAny(*e, occupiedCorner))
			return *e;
	}
	return NULL2DO; // no free corners
}

static bool onScreenObjectCanOverlap(_2DOrigin &a, _2DOrigin &b)
{
	return (&a == &optionTouchCtrlCenterBtnPos.val || &b == &optionTouchCtrlCenterBtnPos.val) // one is the center btn. group, and
		&& (&a == &optionTouchCtrlFaceBtnPos.val || &b == &optionTouchCtrlFaceBtnPos.val
				|| &a == &optionTouchCtrlDpadPos.val || &b == &optionTouchCtrlDpadPos.val); // one is the dpad/face btn. group
}

void resolveOnScreenCollisions(_2DOrigin *movedObj)
{
	_2DOrigin *obj[] = { &optionTouchCtrlFaceBtnPos.val, &optionTouchCtrlDpadPos.val, &optionTouchCtrlCenterBtnPos.val, &optionTouchCtrlMenuPos.val, &optionTouchCtrlFFPos.val };
	iterateTimes(sizeofArray(obj), i)
	{
		if(movedObj == obj[i] || *obj[i] == NULL2DO) // don't move object that was just placed, and ignore objects that are off
		{
			//logMsg("skipped obj %d", (int)i);
			continue;
		}

		iterateTimes(sizeofArray(obj), j)
		{
			if(obj[i] != obj[j] && *obj[j] != NULL2DO && *obj[i] == *obj[j] && !onScreenObjectCanOverlap(*obj[i], *obj[j]))
			{
				_2DOrigin freeO;
				if(obj[i] == &optionTouchCtrlCenterBtnPos.val)
				{
					// Center btns. can only collide with menu/ff hot-spots
					const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val };
					freeO = getFreeOnScreenSpace(occupied, onlyTopBottomO);
				}
				else if(obj[i] == &optionTouchCtrlMenuPos.val || obj[i] == &optionTouchCtrlFFPos.val)
				{
					// Menu/ff hot-spots collide with everything
					const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val, optionTouchCtrlCenterBtnPos.val, };
					freeO = getFreeOnScreenSpace(occupied, allCornersO);
				}
				else
				{
					// Main btns. collide with others of themselves and Menu/ff hot-spots
					const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val };
					freeO = getFreeOnScreenSpace(occupied, allCornersO);
				}
				assert(freeO != NULL2DO);
				logMsg("objs %d & %d collide, moving first to %d,%d", (int)i, (int)j, freeO.x, freeO.y);
				*obj[i] = freeO;
				break;
			}
		}
	}
}*/

void updateVControlImg()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	{
		static Gfx::PixmapTexture overlayImg;
		PngFile png;
		auto filename =	"overlays128.png";
		if(png.loadAsset(filename) != OK)
		{
			bug_exit("couldn't load overlay png");
		}
		overlayImg.init(png);
		vController.setImg(overlayImg);
	}
	#endif
	if(EmuSystem::inputHasKeyboard)
	{
		static Gfx::PixmapTexture kbOverlayImg;
		PngFile png;
		if(png.loadAsset("kbOverlay.png") != OK)
		{
			bug_exit("couldn't load kb overlay png");
		}
		kbOverlayImg.init(png);
		vController.kb.setImg(&kbOverlayImg);
	}
}

}
