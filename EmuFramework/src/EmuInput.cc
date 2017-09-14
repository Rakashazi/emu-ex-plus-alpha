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

#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/data-type/image/sys.hh>
#include <imagine/mem/mem.h>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/InputManagerView.hh>
#include "private.hh"
#include "privateInput.hh"

struct RelPtr  // for Android trackball
{
	int x = 0, y = 0;
	uint xAction = 0, yAction = 0;
};
static RelPtr relPtr{};

TurboInput turboActions{};
std::vector<InputDeviceConfig> inputDevConf{};
std::list<InputDeviceSavedConfig> savedInputDevList{};
std::list<KeyConfig> customKeyConfig{};
KeyMapping keyMapping{};
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

void initVControls(Gfx::Renderer &r)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.dp.setDeadzone(r, vController.xMMSizeToPixel(mainWin.win, int(optionTouchDpadDeadzone) / 100.));
	vController.gp.dp.setDiagonalSensitivity(r, optionTouchDpadDiagonalSensitivity / 1000.);
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
	vController.init((int)optionTouchCtrlAlpha / 255.0, vControllerPixelSize(), View::defaultFace.nominalHeight()*1.75, mainWin.projectionPlane);
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

void processRelPtr(Input::Event e)
{
	using namespace IG;
	if(relPtr.x != 0 && sign(relPtr.x) != sign(e.pos().x))
	{
		//logMsg("reversed trackball X direction");
		relPtr.x = e.pos().x;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	else
		relPtr.x += e.pos().x;

	if(e.pos().x)
	{
		relPtr.xAction = EmuSystem::translateInputAction(e.pos().x > 0 ? EmuControls::systemKeyMapStart+1 : EmuControls::systemKeyMapStart+3);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.xAction);
	}

	if(relPtr.y != 0 && sign(relPtr.y) != sign(e.pos().y))
	{
		//logMsg("reversed trackball Y direction");
		relPtr.y = e.pos().y;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
	else
		relPtr.y += e.pos().y;

	if(e.pos().y)
	{
		relPtr.yAction = EmuSystem::translateInputAction(e.pos().y > 0 ? EmuControls::systemKeyMapStart+2 : EmuControls::systemKeyMapStart);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.yAction);
	}

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtr.x, relPtr.y);
}

void commonInitInput()
{
	relPtr = {};
	turboActions = {};
	fastForwardActive = false;
}

void commonUpdateInput()
{
	using namespace IG;
	static const uint turboFrames = 4;
	static uint turboClock = 0;

	for(auto e : turboActions.activeAction)
	{
		if(e.action)
		{
			if(turboClock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(Input::PUSHED, e.action);
			}
			else if(turboClock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(Input::RELEASED, e.action);
			}
		}
	}
	turboClock++;
	if(turboClock == turboFrames) turboClock = 0;

#ifdef CONFIG_INPUT_RELATIVE_MOTION_DEVICES
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

bool isMenuDismissKey(Input::Event e)
{
	using namespace Input;
	Key dismissKey = Keycode::MENU;
	Key dismissKey2 = Keycode::GAME_Y;
	if(Config::envIsWebOS1)
		dismissKey = Keycode::RCTRL;
	if(Config::MACHINE_IS_PANDORA && e.device()->subtype() == Device::SUBTYPE_PANDORA_HANDHELD)
	{
		if(modalViewController.hasView()) // make sure not performing text input
			return false;
		dismissKey = Keycode::SPACE;
	}
	return e.key() == dismissKey || e.key() == dismissKey2;
}

void updateInputDevices()
{
	int i = 0;
	inputDevConf.clear();
	for(auto &e : Input::deviceList())
	{
		logMsg("input device %d: name: %s, id: %d, map: %d", i, e->name(), e->enumId(), e->map());
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
	physicalControlsPresent = Input::keyInputIsPresent();
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
		bug_unreachable("device type %d missing default configs", dev.map());
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

void InputDeviceConfig::deleteConf()
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name);
		auto removed = IG::removeFirst(savedInputDevList, *savedConf);
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

uint InputDeviceConfig::joystickAxisAsDpadBits()
{
	return dev->joystickAxisAsDpadBits();
}

void InputDeviceConfig::setJoystickAxisAsDpadBits(uint axisMask)
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

// KeyMapping

void KeyMapping::buildAll()
{
	assert(inputDevConf.size() == Input::deviceList().size());
	if(inputDevActionTablePtr)
	{
		mem_free(inputDevActionTablePtr[0]);
	}
	inputDevActionTablePtr = std::make_unique<ActionGroup*[]>(Input::deviceList().size());
	// calculate & allocate complete map including all devices
	{
		uint totalKeys = 0;
		for(auto &e : Input::deviceList())
		{
			totalKeys += Input::Event::mapNumKeys(e->map());
		}
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
	for(auto &e : Input::deviceList())
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
			auto slot = IG::findData_if(group, [](Action a){ return a == 0; });
			if(slot != group + IG::size(group))
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
		std::fill_n(&key[category[startCategory+1].configOffset], category[startCategory+1].keys, 0);
	}
	else
	{
		// transpose joystick keys
		std::copy_n(&key[category[startCategory].configOffset], category[startCategory].keys, &key[category[startCategory+1].configOffset]);
		std::fill_n(&key[category[startCategory].configOffset], category[startCategory].keys, 0);
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory)
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
	vController.setBaseBtnSize(vControllerPixelSize(), View::defaultFace.nominalHeight()*1.75, mainWin.projectionPlane);
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

void updateVControlImg()
{
	auto &r = vController.renderer;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	{
		static Gfx::PixmapTexture overlayImg;
		PngFile png;
		auto filename =	"overlays128.png";
		if(auto ec = png.loadAsset(filename);
			ec)
		{
			logErr("couldn't load overlay png");
		}
		overlayImg.init(r, png);
		vController.setImg(overlayImg);
	}
	#endif
	if(EmuSystem::inputHasKeyboard)
	{
		static Gfx::PixmapTexture kbOverlayImg;
		PngFile png;
		if(auto ec = png.loadAsset("kbOverlay.png");
			ec)
		{
			logErr("couldn't load kb overlay png");
		}
		kbOverlayImg.init(r, png);
		vController.kb.setImg(r, &kbOverlayImg);
	}
}

void setActiveFaceButtons(uint btns)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = btns;
	setupVControllerVars();
	vController.place();
	EmuSystem::clearInputBuffers(emuInputView);
	#endif
}

void updateKeyboardMapping()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.updateKeyboardMapping();
	#endif
}

void toggleKeyboard()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.toggleKeyboard();
	#endif
}

}

void TurboInput::addEvent(uint action)
{
	Action *slot = IG::findData_if(activeAction, [](Action a){ return a == 0; });
	if(slot != activeAction.end())
	{
		slot->action = action;
		logMsg("added turbo event action %d", action);
	}
}

void TurboInput::removeEvent(uint action)
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
