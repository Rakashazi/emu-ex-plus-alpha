#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/util/math/space.hh>
#include <imagine/base/Window.hh>
#include "internal.hh"
#include <snes9x.h>
#include <memmap.h>
#include <display.h>

enum
{
	s9xKeyIdxUp = EmuControls::systemKeyMapStart,
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
	s9xKeyIdxYTurbo
};

void DoGunLatch (int, int);

const char *EmuSystem::inputFaceBtnName = "A/B/X/Y";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const unsigned EmuSystem::inputFaceBtns = 6;
const unsigned EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
const unsigned EmuSystem::maxPlayers = 5;
static int snesPointerX = 0, snesPointerY = 0, snesPointerBtns = 0, snesMouseClick = 0;
static int snesMouseX = 0, snesMouseY = 0;
unsigned doubleClickFrames, rightClickFrames;
static Input::SingleDragTracker dragTracker{};
static bool dragWithButton = false; // true to start next mouse drag with a button held
#ifndef SNES9X_VERSION_1_4
int snesInputPort = SNES_AUTO_INPUT;
int snesActiveInputPort = SNES_JOYPAD;
#else
int snesInputPort = SNES_JOYPAD;
static uint16 joypadData[5]{};
#endif

bool EmuSystem::touchControlsApplicable()
{
	#ifndef SNES9X_VERSION_1_4
	return snesActiveInputPort == SNES_JOYPAD;
	#else
	return IPPU.Controller == SNES_JOYPAD || IPPU.Controller == SNES_MULTIPLAYER5;
	#endif
}

CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons)
{
    if (which == 1)
    	return 0;

    //logMsg("reading mouse %d: %d %d %d, prev %d %d", which1_0_to_1, snesPointerX, snesPointerY, snesPointerBtns, IPPU.PrevMouseX[which1_0_to_1], IPPU.PrevMouseY[which1_0_to_1]);
    x = snesMouseX;
    y = snesMouseY;
    buttons = snesPointerBtns;

    if(snesMouseClick)
    	snesMouseClick--;
    if(snesMouseClick == 1)
    {
    	//logDMsg("ending click");
    	snesPointerBtns = 0;
    }

    return 1;
}

CLINK bool8 S9xReadSuperScopePosition(int &x, int &y, uint32 &buttons)
{
	//logMsg("reading super scope: %d %d %d", snesPointerX, snesPointerY, snesPointerBtns);
	x = snesPointerX;
	y = snesPointerY;
	buttons = snesPointerBtns;
	return 1;
}

#ifdef SNES9X_VERSION_1_4
static uint16 *S9xGetJoypadBits(unsigned idx)
{
	return &joypadData[idx];
}

CLINK uint32 S9xReadJoypad(int which)
{
	assert(which < 5);
	//logMsg("reading joypad %d", which);
	return 0x80000000 | joypadData[which];
}

bool JustifierOffscreen()
{
	return false;
}

void JustifierButtons(uint32& justifiers) { }

static bool usingMouse() { return IPPU.Controller == SNES_MOUSE_SWAPPED; }
static bool usingGun() { return IPPU.Controller == SNES_SUPERSCOPE; }
#endif

void updateVControllerMapping(unsigned player, SysVController::Map &map)
{
	unsigned playerMask = player << 29;
	map[SysVController::F_ELEM] = SNES_A_MASK | playerMask;
	map[SysVController::F_ELEM+1] = SNES_B_MASK | playerMask;
	map[SysVController::F_ELEM+2] = SNES_X_MASK | playerMask;
	map[SysVController::F_ELEM+3] = SNES_Y_MASK | playerMask;
	map[SysVController::F_ELEM+4] = SNES_TL_MASK | playerMask;
	map[SysVController::F_ELEM+5] = SNES_TR_MASK | playerMask;

	map[SysVController::C_ELEM] = SNES_SELECT_MASK | playerMask;
	map[SysVController::C_ELEM+1] = SNES_START_MASK | playerMask;

	map[SysVController::D_ELEM] = SNES_UP_MASK | SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+1] = SNES_UP_MASK | playerMask;
	map[SysVController::D_ELEM+2] = SNES_UP_MASK | SNES_RIGHT_MASK | playerMask;
	map[SysVController::D_ELEM+3] = SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+5] = SNES_RIGHT_MASK | playerMask;
	map[SysVController::D_ELEM+6] = SNES_DOWN_MASK | SNES_LEFT_MASK | playerMask;
	map[SysVController::D_ELEM+7] = SNES_DOWN_MASK | playerMask;
	map[SysVController::D_ELEM+8] = SNES_DOWN_MASK | SNES_RIGHT_MASK | playerMask;
}

unsigned EmuSystem::translateInputAction(unsigned input, bool &turbo)
{
	turbo = 0;
	assert(input >= s9xKeyIdxUp);
	unsigned player = (input - s9xKeyIdxUp) / EmuControls::gamepadKeys;
	unsigned playerMask = player << 29;
	input -= EmuControls::gamepadKeys * player;
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
		case s9xKeyIdxL: return SNES_TL_MASK | playerMask;
		case s9xKeyIdxR: return SNES_TR_MASK | playerMask;
		default: bug_unreachable("input == %d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(EmuApp *, Input::Action action, unsigned emuKey)
{
	auto player = emuKey >> 29; // player is encoded in upper 3 bits of input code
	assert(player < maxPlayers);
	auto &padData = *S9xGetJoypadBits(player);
	padData = IG::setOrClearBits(padData, (uint16)(emuKey & 0xFFFF), action == Input::Action::PUSHED);
}

void EmuSystem::clearInputBuffers(EmuInputView &view)
{
	iterateTimes((unsigned)maxPlayers, p)
	{
		*S9xGetJoypadBits(p) = 0;
	}
	snesPointerBtns = 0;
	doubleClickFrames = 0;
	dragWithButton = false;
	dragTracker.finish();
	dragTracker.setXDragStartDistance(view.window().widthMMInPixels(1.));
	dragTracker.setYDragStartDistance(view.window().heightMMInPixels(1.));
}

void setupSNESInput()
{
	#ifndef SNES9X_VERSION_1_4
	int inputSetup = snesInputPort;
	if(inputSetup == SNES_AUTO_INPUT)
	{
		inputSetup = SNES_JOYPAD;
		if(EmuSystem::gameIsRunning() && !strncmp((const char *) Memory.NSRTHeader + 24, "NSRT", 4))
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
				//S9xSetController(1, CTL_JUSTIFIER,  1, 0, 0, 0);
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
		else
		{
			logMsg("connected joypads");
			IPPU.Controller = SNES_JOYPAD;
		}
	}
	#endif
}

bool EmuSystem::handlePointerInputEvent(Input::Event e, IG::WindowRect gameRect)
{
	switch(snesActiveInputPort)
	{
		case SNES_SUPERSCOPE:
		{
			if(e.released())
			{
				snesPointerBtns = 0;
				#ifndef SNES9X_VERSION_1_4
				*S9xGetSuperscopeBits() = 0;
				#endif
			}
			if(gameRect.overlaps(e.pos()))
			{
				int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
				snesPointerX = IG::scalePointRange((float)xRel, (float)gameRect.xSize(), (float)256.);
				snesPointerY = IG::scalePointRange((float)yRel, (float)gameRect.ySize(), (float)224.);
				//logMsg("mouse moved to @ %d,%d, on SNES %d,%d", e.x, e.y, snesPointerX, snesPointerY);
				if(e.pushed())
				{
					snesPointerBtns = 1;
					#ifndef SNES9X_VERSION_1_4
					*S9xGetSuperscopeBits() = 0x80;
					#endif
				}
			}
			else if(e.pushed())
			{
				snesPointerBtns = 2;
				#ifndef SNES9X_VERSION_1_4
				*S9xGetSuperscopeBits() = 0x40;
				#endif
			}

			#ifndef SNES9X_VERSION_1_4
			S9xGetSuperscopePosBits()[0] = snesPointerX;
			S9xGetSuperscopePosBits()[1] = snesPointerY;
			DoGunLatch(snesPointerX, snesPointerY);
			#endif

			return true;
		}

		case SNES_MOUSE_SWAPPED:
		{
			dragTracker.inputEvent(e,
				[&](Input::DragTrackerState)
				{
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
				},
				[&](Input::DragTrackerState state, Input::DragTrackerState prevState)
				{
					if(!state.isDragging())
						return;
					if(!prevState.isDragging())
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
						auto relPos = state.pos() - prevState.pos();
						snesPointerX += relPos.x;
						snesPointerY += relPos.y;
					}
				},
				[&](Input::DragTrackerState state)
				{
					if(state.isDragging())
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
				});
			snesMouseX = IG::scalePointRange((float)snesPointerX, (float)gameRect.xSize(), (float)256.);
			snesMouseY = IG::scalePointRange((float)snesPointerY, (float)gameRect.ySize(), (float)224.);
			return true;
		}
	}
	return false;
}
