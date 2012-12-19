/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <gui/View.hh>
#include <gui/AlertView.hh>
#include "EmuSystem.hh"
#include "Recent.hh"
#include "Screenshot.hh"
#include <util/gui/ViewStack.hh>
#include <VideoImageOverlay.hh>
#include "EmuOptions.hh"
#include <EmuInput.hh>
#include "MsgPopup.hh"
#include "MultiChoiceView.hh"
#include "ConfigFile.hh"
#include "FilePicker.hh"
#include <EmuView.hh>

#include <meta.h>

bool isMenuDismissKey(const InputEvent &e);
void startGameFromMenu();
bool touchControlsApplicable();
void loadGameCompleteFromFilePicker(uint result, const InputEvent &e);
EmuFilePicker fPicker;
CreditsView credits(creditsViewStr);
YesNoAlertView ynAlertView;
Gfx::Sprite menuIcon;
MultiChoiceView multiChoiceView;
ViewStack viewStack;
extern KeyConfig<EmuControls::systemTotalKeys> keyConfig;

void onLeftNavBtn(const InputEvent &e)
{
	viewStack.popAndShow();
}

void onRightNavBtn(const InputEvent &e)
{
	if(EmuSystem::gameIsRunning())
	{
		startGameFromMenu();
	}
}

BasicNavView viewNav(NavView::OnInputDelegate::create<&onLeftNavBtn>(), NavView::OnInputDelegate::create<&onRightNavBtn>());
static bool menuViewIsActive = 1;

static Rect2<int> emuMenuB, emuFFB;
MsgPopup popup;

EmuView emuView;

namespace Gfx
{
void onViewChange(Gfx::GfxViewState * = 0);
}

#if !defined(CONFIG_AUDIO_ALSA) && !defined(CONFIG_AUDIO_SDL) && !defined(CONFIG_AUDIO_PS3)
	// use WIP direct buffer write API
	#define USE_NEW_AUDIO
#endif

// used on iOS to allow saves on incorrectly root-owned files/dirs
void fixFilePermissions(const char *path)
{
	if(FsSys::hasWriteAccess(path) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	if(!Base::setUIDEffective())
	{
		logErr("failed to set effective uid");
		return;
	}
	FsSys::chown(path, Base::realUID, Base::realUID);
	Base::setUIDReal();
}

#ifdef CONFIG_BASE_IOS_SETUID
namespace CATS
{
	static uchar mainScreenTurnOn = 0;
	char warWasBeginning[] =
	{
		0x58, 0x36, 0x7, 0x7, 0x1B, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x4, 0x58, 0x32, 0x3A,
		0x22, 0x27, 0x16, 0x3, 0x14, 0x1F, 0x12, 0x5, 0x59, 0x16, 0x7, 0x7, 0x58, 0x32, 0x3A, 0x22,
		0x27, 0x16, 0x3, 0x14, 0x1F, 0x12, 0x5, 0x77,
	};
	static char ad2101[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x7, 0x14, 0x12, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2102[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x19, 0x12, 0x4, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2103[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x10, 0x15, 0x14, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char ad2104[] = { 0x14, 0x18, 0x1A, 0x59, 0x12, 0xF, 0x7, 0x1B, 0x2, 0x4, 0x16, 0x1B, 0x7, 0x1F, 0x16, 0x59,
			0x1A, 0x13, 0x12, 0x1A, 0x2, 0x59, 0x1B, 0x1E, 0x4, 0x3, 0x77, };
	static char *in[] = { ad2101, ad2102, ad2103, ad2104 };
	static char makeYourTime[] =
	{
		0x58, 0x1, 0x16, 0x5, 0x58, 0x1B, 0x1E, 0x15, 0x58, 0x13, 0x7, 0x1C, 0x10, 0x58, 0x1E, 0x19,
		0x11, 0x18, 0x77,
	};
	static char theBomb[] =
	{
			0x54, 0x57, 0x24, 0x18, 0x1A, 0x12, 0x15, 0x18, 0x13, 0xE, 0x57, 0x4, 0x12, 0x3, 0x57, 0x2,
			0x4, 0x57, 0x2, 0x7, 0x57, 0x3, 0x1F, 0x12, 0x57, 0x15, 0x18, 0x1A, 0x15, 0x7D, 0x46, 0x45,
			0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x16, 0x13,
			0x4, 0x59, 0x10, 0x59, 0x13, 0x18, 0x2, 0x15, 0x1B, 0x12, 0x14, 0x1B, 0x1E, 0x14, 0x1C, 0x59,
			0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x16, 0x13,
			0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x59,
			0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x15, 0x2,
			0x3, 0x3, 0x18, 0x19, 0x4, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13,
			0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x11, 0x12, 0x12, 0x13, 0x16, 0x13, 0x4, 0x59, 0x10, 0x18,
			0x18, 0x10, 0x1B, 0x12, 0x16, 0x13, 0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x14,
			0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x10, 0x18, 0x18,
			0x10, 0x1B, 0x12, 0x16, 0x13, 0x4, 0x12, 0x5, 0x1, 0x1E, 0x14, 0x12, 0x4, 0x59, 0x14, 0x18,
			0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12,
			0x16, 0x13, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16,
			0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47,
			0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16, 0x13, 0x46, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B,
			0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A,
			0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16,
			0x13, 0x45, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16,
			0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47,
			0x59, 0x46, 0x57, 0x7, 0x16, 0x10, 0x12, 0x16, 0x13, 0x44, 0x59, 0x10, 0x18, 0x18, 0x10, 0x1B,
			0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19, 0x59, 0x14, 0x18, 0x1A,
			0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x3, 0x7, 0x14, 0x59, 0x10,
			0x18, 0x18, 0x10, 0x1B, 0x12, 0x4, 0xE, 0x19, 0x13, 0x1E, 0x14, 0x16, 0x3, 0x1E, 0x18, 0x19,
			0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5,
			0x12, 0x4, 0x18, 0x2, 0x5, 0x14, 0x12, 0x4, 0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19,
			0x1C, 0x4, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46,
			0x57, 0x5, 0x3, 0x45, 0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19, 0x1C, 0x4, 0x59, 0x14,
			0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5, 0x3, 0x44,
			0x59, 0x1E, 0x19, 0x11, 0x18, 0x1B, 0x1E, 0x19, 0x1C, 0x4, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46,
			0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A, 0x59, 0x16, 0x13, 0x1A, 0x18,
			0x15, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57,
			0x5, 0x59, 0x16, 0x13, 0x1A, 0x18, 0x15, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A, 0x1, 0x59, 0x16, 0x13, 0x1A, 0x18, 0x15, 0x59,
			0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1A, 0x1A,
			0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16, 0x59, 0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40,
			0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x5, 0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16,
			0x59, 0x19, 0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x4,
			0x14, 0x5, 0x1E, 0x7, 0x3, 0x4, 0x59, 0x14, 0x1F, 0x1E, 0x3, 0x1E, 0x1C, 0x16, 0x59, 0x19,
			0x12, 0x3, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1B, 0x13, 0x46,
			0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59,
			0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x1B, 0x13, 0x45, 0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18,
			0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45, 0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x14,
			0x16, 0x4, 0x59, 0x14, 0x5, 0x1E, 0x3, 0x12, 0x18, 0x59, 0x14, 0x18, 0x1A, 0x7D, 0x46, 0x45,
			0x40, 0x59, 0x47, 0x59, 0x47, 0x59, 0x46, 0x57, 0x16, 0x13, 0x4, 0x59, 0x16, 0x13, 0x13, 0x1E,
			0x14, 0x3, 0x14, 0x1F, 0x16, 0x19, 0x19, 0x12, 0x1B, 0x59, 0x14, 0x18, 0x1A, 0x77,
	};
	static char greatJustice[] =
	{
		0x58, 0x12, 0x3, 0x14, 0x58, 0x1F, 0x18, 0x4, 0x3, 0x4, 0x77,
	};

	template <size_t S>
	static void weGetSignal(char (&str)[S])
	{
		iterateTimes(S, c)
		{
			str[c] ^= 0x77;
		}
	}

	static bool whatHappen()
	{
		FsSys f;
		if(f.openDir(makeYourTime, Fs::OPEN_UNSORT) == OK)
		{
			iterateTimes(f.numEntries(), i)
			{
				iterateTimes(sizeofArray(in), s)
				{
					if(strstr(f.entryFilename(i), &in[s][16]))
					{
						if(!string_equal(f.entryFilename(i), in[s]))
						{
							f.closeDir();
							return 1;
						}
					}
				}
			}
			f.closeDir();
		}

		return 0;
	}

	static void youKnowWhatYouDoing()
	{
		if(Base::effectiveUID != 0 || !Base::setUIDEffective())
		{
			for(;;) { };
		}
	}

	static void whatYouSay()
	{
		weGetSignal(ad2101);
		weGetSignal(ad2102);
		weGetSignal(ad2103);
		weGetSignal(ad2104);
		weGetSignal(makeYourTime);
		weGetSignal(greatJustice);
		weGetSignal(theBomb);
		weGetSignal(warWasBeginning);
	}

	static void howAreYouGentlemen()
	{
		bool takeOffEveryZIG = whatHappen() || FsSys::fileExists(warWasBeginning);
		Io *ZIG = IoSys::open(greatJustice);
		const uchar *d = ZIG->mmapConst();
		if(takeOffEveryZIG && mem_locate(d, ZIG->size(), theBomb))
		{
			takeOffEveryZIG = 0;
		}
		off_t truncOffset = 0;
		if(takeOffEveryZIG)
		{
			truncOffset = mem_locateRelPos(d, ZIG->size(), theBomb, 20);
		}
		delete ZIG;

		if(takeOffEveryZIG)
		{
			youKnowWhatYouDoing();

			Io *ZIG = IoSys::open(greatJustice, IO_OPEN_WRITE);

			if(truncOffset > 234 && truncOffset < (off_t)ZIG->size())
			{
				ZIG->truncate(truncOffset);
			}
			ZIG->fseek(0, SEEK_END);

			if(takeOffEveryZIG)
			{
				ZIG->fwrite(theBomb, strlen(theBomb), 1);
				ZIG->fwrite("\n", 1, 1);
			}

			delete ZIG;

			Base::setUIDReal();
		}
	}
}
#endif

//static int soundRateDelta = 0;
bool ffGuiKeyPush = 0, ffGuiTouch = 0;



/*static void updateActivePlayerKeyInputs()
{
	mem_zero(playerActiveKeyInput);
	#ifdef INPUT_SUPPORTS_POINTER
	playerActiveKeyInput[pointerInputPlayer] = 1;
	//logMsg("pointer -> player %d", pointerInputPlayer);
	#endif
	#ifdef INPUT_SUPPORTS_KEYBOARD
	playerActiveKeyInput[keyboardInputPlayer] = 1;
	//logMsg("keyboard -> player %d", keyboardInputPlayer);
	#endif
	#ifdef CONFIG_BASE_PS3
	iterateTimes((int)maxPlayers, i)
	{
		playerActiveKeyInput[gamepadInputPlayer[i]] = 1;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH
	iterateTimes(Bluetooth::wiimotes(), i)
	{
		playerActiveKeyInput[wiimoteInputPlayer[i]] = 1;
		//logMsg("wiimote %d -> player %d", i, wiimoteInputPlayer[i]);
	}
	#ifdef CONFIG_BLUETOOTH_ICP
	iterateTimes(Bluetooth::iCPs(), i)
	{
		playerActiveKeyInput[iControlPadInputPlayer[i]] = 1;
		//logMsg("iCP -> player %d", iControlPadInputPlayer);
	}
	#endif
	iterateTimes(Bluetooth::zeemotes(), i)
	{
		playerActiveKeyInput[zeemoteInputPlayer[i]] = 1;
		//logMsg("zeemote -> player %d", zeemoteInputPlayer);
	}
	#endif
	#ifdef CONFIG_INPUT_ICADE
	playerActiveKeyInput[iCadeInputPlayer] = 1;
	#endif
}*/



static GC fontMM =
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || CONFIG_ENV_WEBOS_OS >= 3
	3.5
#elif defined(CONFIG_ENV_WEBOS)
	3.2
#elif defined(CONFIG_BASE_PS3)
	10
#else
	8
#endif
;

static GC largeFontMM =
#if defined(CONFIG_BASE_ANDROID) || defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)
	5
#elif defined(CONFIG_BASE_PS3)
	14
#else
	14
#endif
;

#ifndef CONFIG_BASE_PS3

	#include "VController.hh"
	SysVController vController;

	void refreshTouchConfigMenu();
	void setupVControllerPosition()
	{
		vController.gp.dp.origin = optionTouchCtrlDpadPos;
		vController.gp.btnO = optionTouchCtrlFaceBtnPos;
		vController.gp.cenBtnO = optionTouchCtrlCenterBtnPos;
		vController.gp.triggerPos = optionTouchCtrlTriggerBtnPos;
	}

	static const _2DOrigin allCornersO[] = { RT2DO, RC2DO, RB2DO, CB2DO, LB2DO, LC2DO, LT2DO, CT2DO };
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

	void resolveOnScreenCollisions(_2DOrigin *movedObj = 0)
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
	}

	void updateVControlImg()
	{
		vController.setImg(ResourceImagePng::loadAsset((optionTouchCtrlImgRes == 128U) ? "overlays128.png" : "overlays64.png"));
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		vController.kb.setImg(ResourceImagePng::loadAsset("kbOverlay.png"));
		#endif
	}

#endif

void setupStatusBarInMenu()
{
	if(!optionHideStatusBar.isConst)
		Base::setStatusBarHidden(optionHideStatusBar > 1);
}

static void setupStatusBarInGame()
{
	if(!optionHideStatusBar.isConst)
		Base::setStatusBarHidden(optionHideStatusBar);
}

void onRemoveModalView()
{
	if(!menuViewIsActive)
	{
		startGameFromMenu();
	}
}

bool touchControlsAreOn = 0;



#ifdef INPUT_SUPPORTS_POINTER
void setupVControllerVars()
{
	vController.gp.btnSize = Gfx::xMMSize(int(optionTouchCtrlSize) / 100.);
	logMsg("set on-screen button size: %f, %d pixels", (double)vController.gp.btnSize, Gfx::toIXSize(vController.gp.btnSize));
	vController.gp.dp.deadzone = Gfx::xMMSizeToPixel(int(optionTouchDpadDeadzone) / 100.);
	vController.gp.dp.diagonalSensitivity = optionTouchDpadDiagonalSensitivity / 1000.;
	vController.gp.btnSpace = Gfx::xMMSize(int(optionTouchCtrlBtnSpace) / 100.);
	vController.gp.btnRowShift = 0;
	vController.gp.btnExtraXSize = optionTouchCtrlExtraXBtnSize / 1000.;
	vController.gp.btnExtraYSize = optionTouchCtrlExtraYBtnSize / 1000.;
	vController.gp.btnExtraYSizeMultiRow = optionTouchCtrlExtraYBtnSizeMultiRow / 1000.;
	switch((int)optionTouchCtrlBtnStagger)
	{
		case 0: vController.gp.btnStagger = vController.gp.btnSize * -.75; break;
		case 1: vController.gp.btnStagger = vController.gp.btnSize * -.5; break;
		case 2: vController.gp.btnStagger = 0; break;
		case 3: vController.gp.btnStagger = vController.gp.btnSize * .5; break;
		case 4: vController.gp.btnStagger = vController.gp.btnSize * .75; break;
		default:
			vController.gp.btnStagger = vController.gp.btnSize + vController.gp.btnSpace;
			vController.gp.btnRowShift = -(vController.gp.btnSize + vController.gp.btnSpace);
		break;
	}
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
}
#endif



static void setupVolKeysInGame()
{
	using namespace EmuControls;
	#if defined(INPUT_SUPPORTS_KEYBOARD)
	uint *key = keyConfig.key(InputEvent::DEV_KEYBOARD);
	iterateTimes(keyConfig.totalKeys, k)
	{
		if(Input::isVolumeKey(key[k]))
		{
			Input::setHandleVolumeKeys(1);
			return;
		}
	}
	#endif
}

static bool trackFPS = 0;
static TimeSys prevFrameTime;
static uint frameCount = 0;

void applyOSNavStyle()
{
	uint flags = 0;
	if(optionLowProfileOSNav) flags|= Base::OS_NAV_STYLE_DIM;
	if(optionHideOSNav) flags|= Base::OS_NAV_STYLE_HIDDEN;
	Base::setOSNavigationStyle(flags);
}

void startGameFromMenu()
{
	applyOSNavStyle();
	Base::setIdleDisplayPowerSave(0);
	setupStatusBarInGame();
	if(!optionFrameSkip.isConst && (uint)optionFrameSkip != EmuSystem::optionFrameSkipAuto)
		Gfx::setVideoInterval((int)optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	switch(optionTouchCtrl)
	{
		bcase 0: touchControlsAreOn = 0;
		bcase 1: touchControlsAreOn = 1;
		bcase 2: touchControlsAreOn = !Input::keyInputIsPresent();
		bdefault: bug_branch("%d", (int)optionTouchCtrl);
	}
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef INPUT_SUPPORTS_POINTER
	vController.resetInput();
	#endif
	// TODO: simplify this
	if(!Gfx::setValidOrientations(optionGameOrientation, 1))
		Gfx::onViewChange();
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	Gfx::onViewChange();
	#endif
	commonInitInput();
	ffGuiKeyPush = ffGuiTouch = 0;

	popup.clear();
	Input::setKeyRepeat(0);
	setupVolKeysInGame();
	/*if(optionFrameSkip == -1)
	{
		gfx_updateFrameTime();
	}*/
	/*if(optionFrameSkip != 0 && soundRateDelta != 0)
	{
		logMsg("reset sound rate delta");
		soundRateDelta = 0;
		audio_setPcmRate(audio_pPCM.rate);
	}*/

	#ifdef CONFIG_BASE_IOS_SETUID
	CATS::mainScreenTurnOn++;
	#endif

	EmuSystem::start();
	Base::displayNeedsUpdate();

	if(trackFPS)
	{
		frameCount = 0;
		prevFrameTime.setTimeNow();
	}
}

static void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	Base::setIdleDisplayPowerSave(
	#ifdef CONFIG_BLUETOOTH
		Bluetooth::devsConnected() ? 0 :
	#endif
		(int)optionIdleDisplayPowerSave);
	//Base::setLowProfileNavigation(0);
	setupStatusBarInMenu();
	EmuSystem::pause();
	if(!optionFrameSkip.isConst)
		Gfx::setVideoInterval(1);
	//logMsg("setting valid orientations");
	if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
		Gfx::onViewChange();
	Input::setKeyRepeat(1);
	Input::setHandleVolumeKeys(0);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::displayNeedsUpdate();
	viewStack.show();
}





void confirmExitAppAlert(const InputEvent &e)
{
	Base::exit();
}

namespace Base
{

void onExit(bool backgrounded)
{
	EmuSystem::pause();
	if(backgrounded)
	{
		EmuSystem::saveAutoState();
		EmuSystem::saveBackupMem();
		if(optionNotificationIcon)
		{
			char title[48];
			snprintf(title, sizeof(title), "%s was suspended", CONFIG_APP_NAME);
			Base::addNotification(title, title, EmuSystem::gameName);
		}
	}
	else
	{
		EmuSystem::closeGame();
	}

	saveConfigFile();

	#ifdef CONFIG_BLUETOOTH
		if(!backgrounded || (backgrounded && !optionKeepBluetoothActive))
			Bluetooth::closeBT();
	#endif

	#ifdef CONFIG_BASE_IOS_SETUID
	if(CATS::mainScreenTurnOn >= 2)
	{
		CATS::howAreYouGentlemen();
	}
	#endif
}

void onFocusChange(uint in)
{
	if(optionPauseUnfocused && !menuViewIsActive)
	{
		if(in)
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
		}
		else
		{
			EmuSystem::pause();
		}
		Base::displayNeedsUpdate();
	}
}

static void handleOpenFileCommand(const char *filename)
{
	auto type = FsSys::fileType(filename);
	if(type == Fs::TYPE_DIR)
	{
		logMsg("changing to dir %s from external command", filename);
		restoreMenuFromGame();
		FsSys::chdir(filename);
		viewStack.popToRoot();
		fPicker.init(Input::keyInputIsPresent());
		viewStack.useNavView = 0;
		viewStack.pushAndShow(&fPicker);
		return;
	}
	if(type != Fs::TYPE_FILE)
		return;
	if(!EmuFilePicker::defaultFsFilter(filename, type))
		return;
	FsSys::cPath dir, file;
	dirName(filename, dir);
	baseName(filename, file);
	FsSys::chdir(dir);
	logMsg("opening file %s in dir %s from external command", file, dir);
	restoreMenuFromGame();
	GameFilePicker::onSelectFile(file, InputEvent{});
}

void onDragDrop(const char *filename)
{
	logMsg("got DnD: %s", filename);
	handleOpenFileCommand(filename);
}

void onInterProcessMessage(const char *filename)
{
	logMsg("got IPC: %s", filename);
	handleOpenFileCommand(filename);
}

void onResume(bool focused)
{
	if(optionPauseUnfocused)
		onFocusChange(focused); // let focus handler deal with resuming emulation
	else
	{
		if(!menuViewIsActive) // resume emulation
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
			Base::displayNeedsUpdate();
		}
	}
}

void onInputDevChange(const Base::InputDevChange &change)
{
	logMsg("got input dev change");
	if((uint)optionTouchCtrl == 2)
	{
		if(change.added() || change.shown())
		{
			logMsg("turning off on-screen controls");
			touchControlsAreOn = 0;
			emuView.placeEmu();
		}
		#ifdef CONFIG_BLUETOOTH
		else if(!Bluetooth::devsConnected())
		{
			logMsg("turning on on-screen controls");
			touchControlsAreOn = 1;
			emuView.placeEmu();
		}
		#endif
	}

	if(Base::appIsRunning() && (change.added() || change.removed()))
	{
		popup.printf(2, 0, "%s %d %s", InputEvent::devTypeName(change.devType), change.devId + 1, change.added() ? "connected" : "disconnected");
		Base::displayNeedsUpdate();
	}

	if(menuViewIsActive)
		Base::setIdleDisplayPowerSave(
		#ifdef CONFIG_BLUETOOTH
			Bluetooth::devsConnected() ? 0 :
		#endif
			(int)optionIdleDisplayPowerSave);

	#ifdef CONFIG_BLUETOOTH
		if(viewStack.size == 1) // update bluetooth items
			viewStack.top()->onShow();
	#endif
}

}

void takeGameScreenshot()
{
	FsSys::cPath path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(emuView.vidPix, path))
		{
			popup.printf(2, 1, "Error writing screenshot #%d", screenshotNum);
		}
		else
		{
			popup.printf(2, 0, "Wrote screenshot #%d", screenshotNum);
		}
	}
}

void EmuView::place()
{
	placeEmu();
	//if(emuActive)
	{
		#ifndef CONFIG_BASE_PS3
			//if(touchControlsAreOn())
			{
				setupVControllerVars();
				vController.place();
			}
			if(optionTouchCtrlMenuPos != NULL2DO)
			{
				emuMenuB = Gfx::relRectFromViewport(0, 0, Gfx::xMMSizeToPixel(9), optionTouchCtrlMenuPos, optionTouchCtrlMenuPos);
			}
			if(optionTouchCtrlFFPos != NULL2DO)
			{
				emuFFB = Gfx::relRectFromViewport(0, 0, Gfx::xMMSizeToPixel(9), optionTouchCtrlFFPos, optionTouchCtrlFFPos);
			}
			using namespace Gfx;
			menuIcon.setPos(gXPos(emuMenuB, LB2DO) + gXSize(emuMenuB) / 4.0, gYPos(emuMenuB, LB2DO) + gYSize(emuMenuB) / 3.0,
					gXPos(emuMenuB, RT2DO) - gXSize(emuMenuB) / 4.0, gYPos(emuMenuB, RT2DO) - gYSize(emuMenuB) / 3.0);
		#endif
	}
}



void EmuView::inputEvent(const InputEvent &e)
{
	#ifdef INPUT_SUPPORTS_POINTER
	if(e.isPointer())
	{
		if(e.state == INPUT_PUSHED && optionTouchCtrlMenuPos != NULL2DO && emuMenuB.overlaps(e.x, e.y))
		{
			viewStack.top()->clearSelection();
			restoreMenuFromGame();
			return;
		}
		else if(e.state == INPUT_PUSHED && optionTouchCtrlFFPos != NULL2DO && emuFFB.overlaps(e.x, e.y))
		{
			toggle(ffGuiTouch);
		}
		else if((touchControlsAreOn && touchControlsApplicable())
			#ifdef CONFIG_VCONTROLLER_KEYBOARD
			|| vController.kbMode
			#endif
			)
		{
			vController.applyInput(e);
		}
	}
	else if(e.isRelativePointer())
	{
		processRelPtr(e);
	}
	else
	#endif
	{
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		if(e.state == INPUT_PUSHED && e.button == Input::Key::ESCAPE)
		{
			restoreMenuFromGame();
			return;
		}
		#endif
		KeyMapping::Action (*actionMap)[KeyMapping::maxKeyActions];
		keyMapping.mapFromDevType(e.devType, actionMap);
		bool handledSystemControl = 0;
		uint player = mapInputToPlayer(e);//e.devId;
		//logMsg("player %d input %s", player, Input::buttonName(e.devType, e.button));
		iterateTimes(KeyMapping::maxKeyActions, i)
		{
			KeyMapping::Action action = actionMap[e.button][i];
			if(action != 0)
			{
				using namespace EmuControls;
				action--;

				switch(action)
				{
					bcase guiKeyIdxFastForward:
					{
						ffGuiKeyPush = e.state == INPUT_PUSHED;
						logMsg("fast-forward key state: %d", ffGuiKeyPush);
					}

					bcase guiKeyIdxLoadGame:
					if(e.state == INPUT_PUSHED)
					{
						logMsg("open load game menu from key event");
						restoreMenuFromGame();
						viewStack.popToRoot();
						fPicker.init(Input::keyInputIsPresent());
						viewStack.useNavView = 0;
						viewStack.pushAndShow(&fPicker);
						return;
					}

					bcase guiKeyIdxMenu:
					if(e.state == INPUT_PUSHED)
					{
						logMsg("open menu from key event");
						restoreMenuFromGame();
						return;
					}

					bcase guiKeyIdxSaveState:
					if(e.state == INPUT_PUSHED)
					{
						int ret = EmuSystem::saveState();
						if(ret != STATE_RESULT_OK)
						{
							popup.postError(stateResultToStr(ret));
						}
						else
							popup.post("State Saved");
						return;
					}

					bcase guiKeyIdxLoadState:
					if(e.state == INPUT_PUSHED)
					{
						int ret = EmuSystem::loadState();
						if(ret != STATE_RESULT_OK && ret != STATE_RESULT_OTHER_ERROR)
						{
							popup.postError(stateResultToStr(ret));
						}
						return;
					}

					bcase guiKeyIdxGameScreenshot:
					if(e.state == INPUT_PUSHED)
					{
						takeGameScreenshot();
						return;
					}

					bcase guiKeyIdxExit:
					if(e.state == INPUT_PUSHED)
					{
						logMsg("request exit from key event");
						ynAlertView.init("Really Exit?", Input::keyInputIsPresent());
						ynAlertView.onYesDelegate().bind<&confirmExitAppAlert>();
						ynAlertView.placeRect(Gfx::viewportRect());
						modalView = &ynAlertView;
						restoreMenuFromGame();
						return;
					}

					bdefault:
					{
						//logMsg("action %d, %d", emuKey, state);
						bool turbo;
						uint sysAction = EmuSystem::translateInputAction(action, turbo);
						//logMsg("action %d -> %d, pushed %d", action, sysAction, e.state == INPUT_PUSHED);
						if(turbo)
						{
							if(e.state == INPUT_PUSHED)
							{
								turboActions.addEvent(player, sysAction);
							}
							else
							{
								turboActions.removeEvent(player, sysAction);
							}
						}
						EmuSystem::handleInputAction(player, e.state, sysAction);
						handledSystemControl = 1;
					}
				}
			}
			else
				break;
		}
	}
}

namespace Gfx
{
void onDraw()
{
	emuView.draw();
	if(likely(EmuSystem::isActive()))
	{
		if(trackFPS)
		{
			if(frameCount == 119)
			{
				TimeSys now;
				now.setTimeNow();
				float total = now - prevFrameTime;
				prevFrameTime = now;
				logMsg("%f fps", double(120./total));
				frameCount = 0;
			}
			else
				frameCount++;
		}
		return;
	}

	if(View::modalView)
		View::modalView->draw();
	else if(menuViewIsActive)
		viewStack.draw();
	popup.draw();
}
}

static void handleInputEvent(const InputEvent &e)
{
	/*if(e.isPointer())
	{
		logMsg("Pointer %s @ %d,%d", Input::eventActionToStr(e.state), e.x, e.y);
	}
	else
	{
		logMsg("%s %s from %s", Input::buttonName(e.devType, e.button), Input::eventActionToStr(e.state), e.devTypeName(e.devType));
	}*/
	if(likely(EmuSystem::isActive()))
	{
		emuView.inputEvent(e);
	}
	else if(View::modalView)
		View::modalView->inputEvent(e);
	else if(menuViewIsActive)
	{
		if(e.state == INPUT_PUSHED && e.isDefaultCancelButton())
		{
			if(viewStack.size == 1)
			{
				//logMsg("cancel button at view stack root");
				if(EmuSystem::gameIsRunning())
				{
					startGameFromMenu();
				}
				else if(e.devType == InputEvent::DEV_KEYBOARD && (Config::envIsAndroid || Config::envIsLinux))
					Base::exit();
			}
			else viewStack.popAndShow();
		}
		if(e.state == INPUT_PUSHED && isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				startGameFromMenu();
			}
		}
		else viewStack.inputEvent(e);
	}
}

void setupFont()
{
	logMsg("setting up font, large: %d", (int)optionLargeFonts);
	View::defaultFace->applySettings(FontSettings(Gfx::yMMSizeToPixel(optionLargeFonts ? largeFontMM : fontMM)));
	//View::defaultFace->applySettings(FontSettings(33));
}

namespace Gfx
{
void onViewChange(GfxViewState *)
{
	logMsg("view change");
	GuiTable1D::setDefaultXIndent();
	popup.place();
	emuView.place();
	viewStack.place(Gfx::viewportRect());
	if(View::modalView)
		View::modalView->placeRect(Gfx::viewportRect());
	logMsg("done view change");
}
}

ResourceImage *getArrowAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("padButton.png");
		res->ref();
	}
	return res;
}

ResourceImage *getXAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("xButton.png");
		res->ref();
	}
	return res;
}

static void mainInitCommon()
{
	initOptions();
	EmuSystem::initOptions();
	#ifdef CONFIG_BASE_ANDROID
		if(Base::runningDeviceType() == Base::DEV_TYPE_XPERIA_PLAY)
			EmuControls::profileManager(InputEvent::DEV_KEYBOARD).baseProfile = 1; // Index 1 is always Play profile
	#endif
	resetBaseKeyProfile();

	#ifdef CONFIG_BLUETOOTH
	assert(EmuSystem::maxPlayers <= 5);
	Bluetooth::maxGamepadsPerType = EmuSystem::maxPlayers;
	#endif

	loadConfigFile();

	#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
		Base::setWindowPixelBestColorHint(optionBestColorModeHint);
	#endif

	#ifdef CONFIG_BASE_IOS_SETUID
		CATS::whatYouSay();

		const char *emuPatch = "/Applications/EMUPatcher.app/EMUPatcher";
		if(FsSys::fileExists(emuPatch))
		{
			Base::setUIDEffective();
			FsSys::remove(emuPatch);
			Base::setUIDReal();
		}
	#endif
}

#ifndef CONFIG_BASE_PS3
#include "TouchConfigView.hh"
TouchConfigView tcMenu(touchConfigFaceBtnName, touchConfigCenterBtnName);
#endif

#include "CommonViewControl.hh"

#include "ButtonConfigView.hh"

#include <MenuView.hh>

#include <main/EmuMenuViews.hh>
static SystemOptionView oCategoryMenu;
static SystemMenuView mMenu;

template <size_t NAV_GRAD_SIZE>
static void mainInitWindowCommon(const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE])
{
	Gfx::setClear(1);
	if(!optionDitherImage.isConst)
	{
		Gfx::setDither(optionDitherImage);
	}

	#ifdef CONFIG_BASE_ANDROID
		if(!optionTouchCtrlImgRes.isConst)
			optionTouchCtrlImgRes.initDefault((Gfx::viewPixelWidth() * Gfx::viewPixelHeight() > 380000) ? 128 : 64);
	#endif

	View::defaultFace = ResourceFace::loadSystem();
	assert(View::defaultFace);

	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
		if((int8)optionProcessPriority != 0)
			Base::setProcessPriority(optionProcessPriority);
	#endif
	EmuSystem::configAudioRate();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle();
	setupStatusBarInMenu();

	emuView.gameView.init();
	emuView.disp.init();
	emuView.vidImgOverlay.setEffect(optionOverlayEffect);
	emuView.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;
	keyMapping.buildAll();

	if(optionDPI != 0U)
		Base::setDPI(optionDPI);
	setupFont();
	popup.init();
	#ifndef CONFIG_BASE_PS3
	vController.init((int)optionTouchCtrlAlpha / 255.0, Gfx::xMMSize(int(optionTouchCtrlSize) / 100.));
	updateVControlImg();
	resolveOnScreenCollisions();
	setupVControllerPosition();
	#endif

	menuIcon.init(getArrowAsset());

	View::removeModalViewDelegate().bind<&onRemoveModalView>();
	//logMsg("setting up view stack");
	viewNav.init(View::defaultFace, View::needsBackControl ? getArrowAsset() : nullptr,
			!Config::envIsPS3 ? getArrowAsset() : nullptr, navViewGrad, sizeofArray(navViewGrad));
	viewNav.setRightBtnActive(0);
	viewStack.init();
	if(optionTitleBar)
	{
		//logMsg("title bar on");
		viewStack.setNavView(&viewNav);
	}

	//logMsg("setting menu orientation");
	// set orientation last since it can trigger onViewChange()
	Gfx::setValidOrientations(optionMenuOrientation, 1);
	Base::setAcceptDnd(1);

	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
	if(!Base::apkSignatureIsConsistent())
	{
		ynAlertView.init("Warning: App has been modified by 3rd party, use at your own risk", 0);
		ynAlertView.onNoDelegate().bind<&confirmExitAppAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
		Base::displayNeedsUpdate();
	}
	#endif

	mMenu.init(Config::envIsPS3);
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	Base::displayNeedsUpdate();
}

void OptionCategoryView::onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
{
	oCategoryMenu.init(i, !e.isPointer());
	viewStack.pushAndShow(&oCategoryMenu);
}

ButtonConfigView bcMenu;

OptionCategoryView oMenu;

StateSlotView ssMenu;

RecentGameView rMenu;

InputPlayerMapView ipmMenu;
