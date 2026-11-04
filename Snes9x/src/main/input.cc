/*  This file is part of Snes9x EX.

	Please see COPYING file in root directory for license information. */

module;
#include <snes9x.h>
#include <memmap.h>
#include <display.h>
#ifndef SNES9X_VERSION_1_4
#include <controls.h>
#ifdef SNES9X_ANDROID_BRIDGE
#include "../bridge/Bridge.hh"
#endif
#endif

module system;

namespace EmuEx
{

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

void Snes9xSystem::handleInputAction(EmuApp*, InputAction a)
{
	auto player = a.flags.deviceId;
	assume(player < AppMeta::maxPlayers);
	auto &padData = *S9xGetJoypadBits(player);
	padData = setOrClearBits(padData, bit(a.code), a.isPushed());
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	SnesBridge::instance().manualInput(player, padData);
#endif
}

void Snes9xSystem::clearInputBuffers()
{
#if defined(SNES9X_ANDROID_BRIDGE) && !defined(SNES9X_VERSION_1_4)
	for(unsigned p=0;p<5;++p) SnesBridge::instance().manualInput(p,0);
#endif
	for(auto p: iotaCount(AppMeta::maxPlayers))
	{
		*S9xGetJoypadBits(p) = 0;
	}
	snesMouseClick = 0;
	snesPointerBtns = 0;
	doubleClickFrames = 0;
	dragWithButton = false;
	mousePointerId = Input::NULL_POINTER_ID;
}

void Snes9xSystem::setupSNESInput(VController& vCtrl)
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
			log.info("using automatic input:{}", inputSetup);
	}

	if(inputSetup == SNES_MOUSE_SWAPPED)
	{
		S9xSetController(0, CTL_MOUSE, 0, 0, 0, 0);
		S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);
		log.info("setting mouse input");
	}
	else if(inputSetup == SNES_SUPERSCOPE)
	{
		S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_SUPERSCOPE, 0, 0, 0, 0);
		log.info("setting superscope input");
	}
	else if(inputSetup == SNES_JUSTIFIER)
	{
		S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_JUSTIFIER, 0, 0, 0, 0);
		log.info("setting justifier input");
	}
	else // Joypad
	{
		if(optionMultitap)
		{
			S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
			S9xSetController(1, CTL_MP5, 1, 2, 3, 4);
			log.info("setting 5-player joypad input");
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
		log.info("connected multitap");
		Settings.MultiPlayer5Master = Settings.MultiPlayer5 = 1;
		Settings.ControllerOption = IPPU.Controller = SNES_MULTIPLAYER5;
	}
	else
	{
		if(snesInputPort == SNES_MOUSE_SWAPPED)
		{
			log.info("connected mouse");
			Settings.MouseMaster = Settings.Mouse = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_MOUSE_SWAPPED;
		}
		else if(snesInputPort == SNES_SUPERSCOPE)
		{
			log.info("connected superscope");
			Settings.SuperScopeMaster = Settings.SuperScope = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_SUPERSCOPE;
		}
		else if(snesInputPort == SNES_JUSTIFIER)
		{
			log.info("connected justifier");
			Settings.Justifier = 1;
			Settings.ControllerOption = IPPU.Controller = SNES_JUSTIFIER;
		}
		else
		{
			log.info("connected joypads");
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
	snesPointerX = remap(xRel, 0, gameRect.xSize(), 0, 256);
	snesPointerY = remap(yRel, 0, gameRect.ySize(), 0, 224);
	//logMsg("updated pointer position:%d,%d (%d,%d in window)", snesPointerX, snesPointerY, pos.x, pos.y);
	return {snesPointerX, snesPointerY};
}

bool Snes9xSystem::onPointerInputStart(const Input::MotionEvent& e, Input::DragTrackerState, WindowRect gameRect)
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

bool Snes9xSystem::onPointerInputUpdate(const Input::MotionEvent& e, Input::DragTrackerState dragState,
	Input::DragTrackerState prevDragState, WindowRect gameRect)
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
						log.info("started drag with right-button");
					}
					else
					{
						snesPointerBtns = 1;
						log.info("started drag with left-button");
					}
				}
				else
				{
					log.info("started drag");
				}
			}
			else
			{
				auto relPos = dragState.pos() - prevDragState.pos();
				snesPointerX += relPos.x;
				snesPointerY += relPos.y;
			}
			snesMouseX = remap(snesPointerX, 0, gameRect.xSize(), 0, 256);
			snesMouseY = remap(snesPointerY, 0, gameRect.ySize(), 0, 224);
			return true;
		}
	}
	return false;
}

bool Snes9xSystem::onPointerInputEnd(const Input::MotionEvent& e, Input::DragTrackerState dragState, WindowRect)
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
				log.info("stopped drag");
				snesPointerBtns = 0;
			}
			else
			{
				if(!rightClickFrames)
				{
					log.info("right clicking mouse");
					snesPointerBtns = 2;
					doubleClickFrames = 15; // allow extra time for a right-click & drag
				}
				else
				{
					log.info("left clicking mouse");
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

#ifdef SNES9X_VERSION_1_4
extern "C"
#else
extern "C++"
#endif
bool8 S9xReadMousePosition(int which, int& x, int& y, uint32& buttons)
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
extern "C" bool8 S9xReadSuperScopePosition(int& x, int& y, uint32& buttons)
{
	//logMsg("reading super scope: %d %d %d", snesPointerX, snesPointerY, snesPointerBtns);
	auto &sys = gSnes9xSystem();
	x = sys.snesPointerX;
	y = sys.snesPointerY;
	buttons = sys.snesPointerBtns;
	return 1;
}

extern "C" uint32 S9xReadJoypad(int which)
{
	assume(which < 5);
	//logMsg("reading joypad %d", which);
	return 0x80000000 | gSnes9xSystem().joypadData[which];
}

extern "C++" bool JustifierOffscreen()
{
	return !gSnes9xSystem().snesMouseClick;
}

extern "C++" void JustifierButtons(uint32& justifiers)
{
	if(gSnes9xSystem().snesPointerBtns)
		justifiers |= 0x00100;
}
#endif
