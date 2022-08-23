#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/util/math/math.hh>
#include <imagine/base/Window.hh>
#include "MainSystem.hh"
#include <snes9x.h>
#include <memmap.h>
#include <display.h>

namespace EmuEx
{

enum
{
	s9xKeyIdxUp = Controls::systemKeyMapStart,
	s9xKeyIdxRight,
	s9xKeyIdxDown,
	s9xKeyIdxLeft,
	s9xKeyIdxLeftUp,
	s9xKeyIdxRightUp,
	s9xKeyIdxRightDown,
	s9xKeyIdxLeftDown,
	s9xKeyIdxSelect,
	s9xKeyIdxStart,
	s9xKeyIdxA,
	s9xKeyIdxB,
	s9xKeyIdxX,
	s9xKeyIdxY,
	s9xKeyIdxL,
	s9xKeyIdxR,
	s9xKeyIdxATurbo,
	s9xKeyIdxBTurbo,
	s9xKeyIdxXTurbo,
	s9xKeyIdxYTurbo,
	s9xKeyIdxLTurbo,
	s9xKeyIdxRTurbo,
};

const char *EmuSystem::inputFaceBtnName = "A/B/X/Y/L/R";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const int EmuSystem::inputFaceBtns = 6;
const int EmuSystem::inputCenterBtns = 2;
int EmuSystem::inputLTriggerIndex = 5;
int EmuSystem::inputRTriggerIndex = 2;
const int EmuSystem::maxPlayers = 5;
std::array<int, EmuSystem::MAX_FACE_BTNS> EmuSystem::vControllerImageMap{1, 0, 5, 3, 2, 4};

// from controls.cpp
#define SUPERSCOPE_FIRE			0x80
#define SUPERSCOPE_CURSOR		0x40
#define SUPERSCOPE_TURBO		0x20
#define SUPERSCOPE_PAUSE		0x10
#define SUPERSCOPE_OFFSCREEN	0x02

#define JUSTIFIER_TRIGGER		0x80
#define JUSTIFIER_START			0x20
#define JUSTIFIER_SELECT		0x08

constexpr unsigned playerBitShift = 28; // player is encoded in 3 bits, last bit of input code is reserved

VController::Map Snes9xSystem::vControllerMap(int player)
{
	unsigned playerMask = player << playerBitShift;
	VController::Map map{};
	map[VController::F_ELEM] = SNES_B_MASK | playerMask;
	map[VController::F_ELEM+1] = SNES_A_MASK | playerMask;
	map[VController::F_ELEM+2] = SNES_TR_MASK | playerMask;
	map[VController::F_ELEM+3] = SNES_Y_MASK | playerMask;
	map[VController::F_ELEM+4] = SNES_X_MASK | playerMask;
	map[VController::F_ELEM+5] = SNES_TL_MASK | playerMask;

	map[VController::C_ELEM] = SNES_SELECT_MASK | playerMask;
	map[VController::C_ELEM+1] = SNES_START_MASK | playerMask;

	map[VController::D_ELEM] = SNES_UP_MASK | SNES_LEFT_MASK | playerMask;
	map[VController::D_ELEM+1] = SNES_UP_MASK | playerMask;
	map[VController::D_ELEM+2] = SNES_UP_MASK | SNES_RIGHT_MASK | playerMask;
	map[VController::D_ELEM+3] = SNES_LEFT_MASK | playerMask;
	map[VController::D_ELEM+5] = SNES_RIGHT_MASK | playerMask;
	map[VController::D_ELEM+6] = SNES_DOWN_MASK | SNES_LEFT_MASK | playerMask;
	map[VController::D_ELEM+7] = SNES_DOWN_MASK | playerMask;
	map[VController::D_ELEM+8] = SNES_DOWN_MASK | SNES_RIGHT_MASK | playerMask;
	return map;
}

unsigned Snes9xSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	assert(input >= s9xKeyIdxUp);
	unsigned player = (input - s9xKeyIdxUp) / Controls::gamepadKeys;
	unsigned playerMask = player << playerBitShift;
	input -= Controls::gamepadKeys * player;
	switch(input)
	{
		case s9xKeyIdxUp: return SNES_UP_MASK | playerMask;
		case s9xKeyIdxRight: return SNES_RIGHT_MASK | playerMask;
		case s9xKeyIdxDown: return SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxLeft: return SNES_LEFT_MASK | playerMask;
		case s9xKeyIdxLeftUp: return SNES_LEFT_MASK | SNES_UP_MASK | playerMask;
		case s9xKeyIdxRightUp: return SNES_RIGHT_MASK | SNES_UP_MASK | playerMask;
		case s9xKeyIdxRightDown: return SNES_RIGHT_MASK | SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxLeftDown: return SNES_LEFT_MASK | SNES_DOWN_MASK | playerMask;
		case s9xKeyIdxSelect: return SNES_SELECT_MASK | playerMask;
		case s9xKeyIdxStart: return SNES_START_MASK | playerMask;
		case s9xKeyIdxXTurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxX: return SNES_X_MASK | playerMask;
		case s9xKeyIdxYTurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxY: return SNES_Y_MASK | playerMask;
		case s9xKeyIdxATurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxA: return SNES_A_MASK | playerMask;
		case s9xKeyIdxBTurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxB: return SNES_B_MASK | playerMask;
		case s9xKeyIdxLTurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxL: return SNES_TL_MASK | playerMask;
		case s9xKeyIdxRTurbo: turbo = 1; [[fallthrough]];
		case s9xKeyIdxR: return SNES_TR_MASK | playerMask;
		default: bug_unreachable("input == %d", input);
	}
}

#ifdef SNES9X_VERSION_1_4
static uint16 *S9xGetJoypadBits(unsigned idx)
{
	return &gSnes9xSystem().joypadData[idx];
}
#endif

void Snes9xSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.key >> playerBitShift;
	assert(player < maxPlayers);
	auto &padData = *S9xGetJoypadBits(player);
	padData = IG::setOrClearBits(padData, (uint16)(a.key & 0xFFFF), a.state == Input::Action::PUSHED);
}

void Snes9xSystem::clearInputBuffers(EmuInputView &view)
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

WP Snes9xSystem::updateAbsolutePointerPosition(IG::WindowRect gameRect, WP pos)
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
