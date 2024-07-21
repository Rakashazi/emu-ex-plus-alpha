#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/util/math.hh>
#include <imagine/base/Window.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <memmap.h>
#include <display.h>
#include <imagine/logger/logger.h>

namespace EmuEx
{

const int EmuSystem::maxPlayers = 5;

enum class SnesKey : KeyCode
{
	Up = 11,
	Right = 8,
	Down = 10,
	Left = 9,
	Select = 13,
	Start = 12,
	A = 7,
	B = 15,
	X = 6,
	Y = 14,
	L = 5,
	R = 4,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	SnesKey::Up,
	SnesKey::Right,
	SnesKey::Down,
	SnesKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	SnesKey::Select,
	SnesKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	SnesKey::B,
	SnesKey::A,
	SnesKey::Y,
	SnesKey::X
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto faceLRKeyInfo = makeArray<KeyInfo>
(
	SnesKey::B,
	SnesKey::A,
	SnesKey::R,
	SnesKey::Y,
	SnesKey::X,
	SnesKey::L
);

constexpr auto lKeyInfo = makeArray<KeyInfo>(SnesKey::L);
constexpr auto rKeyInfo = makeArray<KeyInfo>(SnesKey::R);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceLRKeyInfo, turboFaceKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);
constexpr auto gp3KeyInfo = transpose(gpKeyInfo, 2);
constexpr auto gp4KeyInfo = transpose(gpKeyInfo, 3);
constexpr auto gp5KeyInfo = transpose(gpKeyInfo, 4);

std::span<const KeyCategory> Snes9xApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
		KeyCategory{"Gamepad 3", gp3KeyInfo, 2},
		KeyCategory{"Gamepad 4", gp4KeyInfo, 3},
		KeyCategory{"Gamepad 5", gp5KeyInfo, 4},
	};
	return categories;
}

std::string_view Snes9xApp::systemKeyCodeToString(KeyCode c)
{
	switch(SnesKey(c))
	{
		case SnesKey::Up: return "Up";
		case SnesKey::Right: return "Right";
		case SnesKey::Down: return "Down";
		case SnesKey::Left: return "Left";
		case SnesKey::Select: return "Select";
		case SnesKey::Start: return "Start";
		case SnesKey::A: return "A";
		case SnesKey::B: return "B";
		case SnesKey::X: return "X";
		case SnesKey::Y: return "Y";
		case SnesKey::L: return "L";
		case SnesKey::R: return "R";
		default: return "";
	}
}

std::span<const KeyConfigDesc> Snes9xApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{SnesKey::Up, Keycode::UP},
		KeyMapping{SnesKey::Right, Keycode::RIGHT},
		KeyMapping{SnesKey::Down, Keycode::DOWN},
		KeyMapping{SnesKey::Left, Keycode::LEFT},
		KeyMapping{SnesKey::Select, Keycode::SPACE},
		KeyMapping{SnesKey::Start, Keycode::ENTER},
		KeyMapping{SnesKey::B, Keycode::Z},
		KeyMapping{SnesKey::A, Keycode::X},
		KeyMapping{SnesKey::Y, Keycode::A},
		KeyMapping{SnesKey::X, Keycode::S},
		KeyMapping{SnesKey::L, Keycode::Q},
		KeyMapping{SnesKey::R, Keycode::W},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{SnesKey::Up, Keycode::UP},
		KeyMapping{SnesKey::Right, Keycode::RIGHT},
		KeyMapping{SnesKey::Down, Keycode::DOWN},
		KeyMapping{SnesKey::Left, Keycode::LEFT},
		KeyMapping{SnesKey::Select, Keycode::GAME_SELECT},
		KeyMapping{SnesKey::Start, Keycode::GAME_START},
		KeyMapping{SnesKey::B, Keycode::GAME_A},
		KeyMapping{SnesKey::A, Keycode::GAME_B},
		KeyMapping{SnesKey::Y, Keycode::GAME_X},
		KeyMapping{SnesKey::X, Keycode::GAME_Y},
		KeyMapping{SnesKey::L, Keycode::GAME_L1},
		KeyMapping{SnesKey::R, Keycode::GAME_R1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{SnesKey::Up, WiimoteKey::UP},
		KeyMapping{SnesKey::Right, WiimoteKey::RIGHT},
		KeyMapping{SnesKey::Down, WiimoteKey::DOWN},
		KeyMapping{SnesKey::Left, WiimoteKey::LEFT},
		KeyMapping{SnesKey::B, WiimoteKey::_1},
		KeyMapping{SnesKey::A, WiimoteKey::_2},
		KeyMapping{SnesKey::Y, WiimoteKey::A},
		KeyMapping{SnesKey::X, WiimoteKey::B},
		KeyMapping{SnesKey::Select, WiimoteKey::MINUS},
		KeyMapping{SnesKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool Snes9xApp::allowsTurboModifier(KeyCode c)
{
	switch(SnesKey(c))
	{
		case SnesKey::R ... SnesKey::A:
		case SnesKey::Y ... SnesKey::B:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc Snes9xApp::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		x{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 2}, {2, 2}})},
		y{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 2}, {2, 2}})},
		l{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 4}, {2, 2}})},
		r{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 4}, {2, 2}})},
		select{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		start{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(SnesKey(key[0]))
	{
		case SnesKey::A: return virtualControllerAssets.a;
		case SnesKey::B: return virtualControllerAssets.b;
		case SnesKey::X: return virtualControllerAssets.x;
		case SnesKey::Y: return virtualControllerAssets.y;
		case SnesKey::L: return virtualControllerAssets.l;
		case SnesKey::R: return virtualControllerAssets.r;
		case SnesKey::Select: return virtualControllerAssets.select;
		case SnesKey::Start: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

// from controls.cpp
#define SUPERSCOPE_FIRE			0x80
#define SUPERSCOPE_CURSOR		0x40
#define SUPERSCOPE_TURBO		0x20
#define SUPERSCOPE_PAUSE		0x10
#define SUPERSCOPE_OFFSCREEN	0x02

#define JUSTIFIER_TRIGGER		0x80
#define JUSTIFIER_START			0x20
#define JUSTIFIER_SELECT		0x08

#ifdef SNES9X_VERSION_1_4
static uint16 *S9xGetJoypadBits(unsigned idx)
{
	return &gSnes9xSystem().joypadData[idx];
}
#endif

void Snes9xSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	assert(player < maxPlayers);
	auto &padData = *S9xGetJoypadBits(player);
	padData = setOrClearBits(padData, bit(a.code), a.isPushed());
}

void Snes9xSystem::clearInputBuffers(EmuInputView&)
{
	for(auto p : iotaCount(maxPlayers))
	{
		*S9xGetJoypadBits(p) = 0;
	}
	snesMouseClick = 0;
	snesPointerBtns = 0;
	doubleClickFrames = 0;
	dragWithButton = false;
	mousePointerId = Input::NULL_POINTER_ID;
}

void Snes9xSystem::setupSNESInput(VController &vCtrl)
{
	#ifndef SNES9X_VERSION_1_4
	int inputSetup = snesInputPort;
	if(inputSetup == SNES_AUTO_INPUT)
	{
		inputSetup = SNES_JOYPAD;
		if(hasContent() && !strncmp((const char *) Memory.NSRTHeader + 24, "NSRT", 4))
		{
			switch (Memory.NSRTHeader[29])
			{
				case 0x00:	// Everything goes
				break;

				case 0x10:	// Mouse in Port 0
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x01:	// Mouse in Port 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x03:	// Super Scope in Port 1
				inputSetup = SNES_SUPERSCOPE;
				break;

				case 0x06:	// Multitap in Port 1
				//S9xSetController(1, CTL_MP5,        1, 2, 3, 4);
				break;

				case 0x66:	// Multitap in Ports 0 and 1
				//S9xSetController(0, CTL_MP5,        0, 1, 2, 3);
				//S9xSetController(1, CTL_MP5,        4, 5, 6, 7);
				break;

				case 0x08:	// Multitap in Port 1, Mouse in new Port 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x04:	// Pad or Super Scope in Port 1
				inputSetup = SNES_SUPERSCOPE;
				break;

				case 0x05:	// Justifier - Must ask user...
				inputSetup = SNES_JUSTIFIER;
				break;

				case 0x20:	// Pad or Mouse in Port 0
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x22:	// Pad or Mouse in Port 0 & 1
				inputSetup = SNES_MOUSE_SWAPPED;
				break;

				case 0x24:	// Pad or Mouse in Port 0, Pad or Super Scope in Port 1
				// There should be a toggles here for what to put in, I'm leaving it at gamepad for now
				break;

				case 0x27:	// Pad or Mouse in Port 0, Pad or Mouse or Super Scope in Port 1
				// There should be a toggles here for what to put in, I'm leaving it at gamepad for now
				break;

				// Not Supported yet
				case 0x99:	// Lasabirdie
				break;

				case 0x0A:	// Barcode Battler
				break;
			}
		}
		if(inputSetup != SNES_JOYPAD)
			logMsg("using automatic input %d", inputSetup);
	}

	if(inputSetup == SNES_MOUSE_SWAPPED)
	{
		S9xSetController(0, CTL_MOUSE, 0, 0, 0, 0);
		S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
		logMsg("setting mouse input");
	}
	else if(inputSetup == SNES_SUPERSCOPE)
	{
		S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_SUPERSCOPE, 0, 0, 0, 0);
		logMsg("setting superscope input");
	}
	else if(inputSetup == SNES_JUSTIFIER)
	{
		S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_JUSTIFIER, 0, 0, 0, 0);
		logMsg("setting justifier input");
	}
	else // Joypad
	{
		if(optionMultitap)
		{
			S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
			S9xSetController(1, CTL_MP5, 1, 2, 3, 4);
			logMsg("setting 5-player joypad input");
		}
		else
		{
			S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
			S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
		}
	}
	snesActiveInputPort = inputSetup;
	vCtrl.setGamepadIsEnabled(inputSetup == SNES_JOYPAD || inputSetup == SNES_JUSTIFIER);
	#else
	Settings.MultiPlayer5Master = Settings.MultiPlayer5 = 0;
	Settings.MouseMaster = Settings.Mouse = 0;
	Settings.SuperScopeMaster = Settings.SuperScope = 0;
	Settings.Justifier = Settings.SecondJustifier = 0;
	if(snesInputPort == SNES_JOYPAD && optionMultitap)
	{
		logMsg("connected multitap");
		Settings.MultiPlayer5Master = Settings.MultiPlayer5 = 1;
		Settings.ControllerOption = IPPU.Controller = SNES_MULTIPLAYER5;
	}
	else
	{
		if(snesInputPort == SNES_MOUSE_SWAPPED)
		{
			logMsg("connected mouse");
			Settings.MouseMaster = Settings.Mouse = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_MOUSE_SWAPPED;
		}
		else if(snesInputPort == SNES_SUPERSCOPE)
		{
			logMsg("connected superscope");
			Settings.SuperScopeMaster = Settings.SuperScope = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_SUPERSCOPE;
		}
		else if(snesInputPort == SNES_JUSTIFIER)
		{
			logMsg("connected justifier");
			Settings.Justifier = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_JUSTIFIER;
		}
		else
		{
			logMsg("connected joypads");
			IPPU.Controller = SNES_JOYPAD;
		}
	}
	vCtrl.setGamepadIsEnabled(IPPU.Controller == SNES_JOYPAD || IPPU.Controller == SNES_MULTIPLAYER5
		|| IPPU.Controller == SNES_JUSTIFIER);
	#endif
}

WPt Snes9xSystem::updateAbsolutePointerPosition(WRect gameRect, WPt pos)
{
	int xRel = pos.x - gameRect.x, yRel = pos.y - gameRect.y;
	snesPointerX = IG::remap(xRel, 0, gameRect.xSize(), 0, 256);
	snesPointerY = IG::remap(yRel, 0, gameRect.ySize(), 0, 224);
	//logMsg("updated pointer position:%d,%d (%d,%d in window)", snesPointerX, snesPointerY, pos.x, pos.y);
	return {snesPointerX, snesPointerY};
}

bool Snes9xSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect gameRect)
{
	switch(snesActiveInputPort)
	{
		case SNES_SUPERSCOPE:
		{
			snesMouseClick = 1;
			if(gameRect.overlaps(e.pos()))
			{
				updateAbsolutePointerPosition(gameRect, e.pos());
				if(e.pushed())
				{
					#ifndef SNES9X_VERSION_1_4
					*S9xGetSuperscopeBits() = SUPERSCOPE_FIRE;
					#else
					snesPointerBtns = 1;
					#endif
				}
			}
			else
			{
				#ifndef SNES9X_VERSION_1_4
				*S9xGetSuperscopeBits() = SUPERSCOPE_CURSOR;
				#else
				snesPointerBtns = 2;
				#endif
			}
			return true;
		}
		case SNES_JUSTIFIER:
		{
			if(gameRect.overlaps(e.pos()))
			{
				snesMouseClick = 1;
				updateAbsolutePointerPosition(gameRect, e.pos());
				if(e.pushed())
				{
					#ifndef SNES9X_VERSION_1_4
					*S9xGetJustifierBits() = JUSTIFIER_TRIGGER;
					#else
					snesPointerBtns = 1;
					#endif
				}
			}
			else
			{
				#ifndef SNES9X_VERSION_1_4
				*S9xGetJustifierBits() = JUSTIFIER_TRIGGER;
				#else
				snesPointerBtns = 1;
				#endif
			}
			return true;
		}
		case SNES_MOUSE_SWAPPED:
		{
			if(mousePointerId != Input::NULL_POINTER_ID)
				return false;
			mousePointerId = e.pointerId();
			rightClickFrames = 15;
			if(doubleClickFrames) // check if in double-click time window
			{
				dragWithButton = 1;
			}
			else
			{
				dragWithButton = 0;
				doubleClickFrames = 15;
			}
			return true;
		}
	}
	return false;
}

bool Snes9xSystem::onPointerInputUpdate(const Input::MotionEvent &e, Input::DragTrackerState dragState,
	Input::DragTrackerState prevDragState, IG::WindowRect gameRect)
{
	switch(snesActiveInputPort)
	{
		case SNES_MOUSE_SWAPPED:
		{
			if(e.pointerId() != mousePointerId || !dragState.isDragging())
				return false;
			if(!prevDragState.isDragging())
			{
				if(dragWithButton)
				{
					snesMouseClick = 0;
					if(!rightClickFrames)
					{
						// in right-click time window
						snesPointerBtns = 2;
						logMsg("started drag with right-button");
					}
					else
					{
						snesPointerBtns = 1;
						logMsg("started drag with left-button");
					}
				}
				else
				{
					logMsg("started drag");
				}
			}
			else
			{
				auto relPos = dragState.pos() - prevDragState.pos();
				snesPointerX += relPos.x;
				snesPointerY += relPos.y;
			}
			snesMouseX = IG::remap(snesPointerX, 0, gameRect.xSize(), 0, 256);
			snesMouseY = IG::remap(snesPointerY, 0, gameRect.ySize(), 0, 224);
			return true;
		}
	}
	return false;
}

bool Snes9xSystem::onPointerInputEnd(const Input::MotionEvent &e, Input::DragTrackerState dragState, IG::WindowRect)
{
	switch(snesActiveInputPort)
	{
		case SNES_SUPERSCOPE:
		{
			snesMouseClick = 0;
			#ifndef SNES9X_VERSION_1_4
			*S9xGetSuperscopeBits() = SUPERSCOPE_OFFSCREEN;
			#else
			snesPointerBtns = 0;
			#endif
			return true;
		}
		case SNES_JUSTIFIER:
		{
			snesMouseClick = 0;
			#ifndef SNES9X_VERSION_1_4
			*S9xGetJustifierBits() = 0;
			#else
			snesPointerBtns = 0;
			#endif
			return true;
		}
		case SNES_MOUSE_SWAPPED:
		{
			if(e.pointerId() != mousePointerId)
				return false;
			mousePointerId = Input::NULL_POINTER_ID;
			if(dragState.isDragging())
			{
				logMsg("stopped drag");
				snesPointerBtns = 0;
			}
			else
			{
				if(!rightClickFrames)
				{
					logMsg("right clicking mouse");
					snesPointerBtns = 2;
					doubleClickFrames = 15; // allow extra time for a right-click & drag
				}
				else
				{
					logMsg("left clicking mouse");
					snesPointerBtns = 1;
				}
				snesMouseClick = 3;
			}
			return true;
		}
	}
	return false;
}

SystemInputDeviceDesc Snes9xSystem::inputDeviceDesc(int) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO, {.staggeredLayout = true}},
		InputComponentDesc{"Face Buttons + Inline L/R", faceLRKeyInfo, InputComponent::button, RB2DO, {.altConfig = true, .staggeredLayout = true}},
		InputComponentDesc{"L", lKeyInfo, InputComponent::trigger, LB2DO},
		InputComponentDesc{"R", rKeyInfo, InputComponent::trigger, RB2DO},
		InputComponentDesc{"Select", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Start", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Select/Start", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

}

using namespace EmuEx;

CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons)
{
    if (which == 1)
    	return 0;
    auto &sys = gSnes9xSystem();
    //logMsg("reading mouse %d: %d %d %d, prev %d %d", which1_0_to_1, snesPointerX, snesPointerY, snesPointerBtns, IPPU.PrevMouseX[which1_0_to_1], IPPU.PrevMouseY[which1_0_to_1]);
    x = sys.snesMouseX;
    y = sys.snesMouseY;
    buttons = sys.snesPointerBtns;

    if(sys.snesMouseClick)
    	sys.snesMouseClick--;
    if(sys.snesMouseClick == 1)
    {
    	//logDMsg("ending click");
    	sys.snesPointerBtns = 0;
    }

    return 1;
}

#ifdef SNES9X_VERSION_1_4
CLINK bool8 S9xReadSuperScopePosition(int &x, int &y, uint32 &buttons)
{
	//logMsg("reading super scope: %d %d %d", snesPointerX, snesPointerY, snesPointerBtns);
	auto &sys = gSnes9xSystem();
	x = sys.snesPointerX;
	y = sys.snesPointerY;
	buttons = sys.snesPointerBtns;
	return 1;
}

CLINK uint32 S9xReadJoypad(int which)
{
	assert(which < 5);
	//logMsg("reading joypad %d", which);
	return 0x80000000 | gSnes9xSystem().joypadData[which];
}

bool JustifierOffscreen()
{
	return !gSnes9xSystem().snesMouseClick;
}

void JustifierButtons(uint32& justifiers)
{
	if(gSnes9xSystem().snesPointerBtns)
		justifiers |= 0x00100;
}
#endif
