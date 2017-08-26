#include <emuframework/EmuApp.hh>
#include "internal.hh"

enum
{
	ssKeyIdxUp = EmuControls::systemKeyMapStart,
	ssKeyIdxRight,
	ssKeyIdxDown,
	ssKeyIdxLeft,
	ssKeyIdxLeftUp,
	ssKeyIdxRightUp,
	ssKeyIdxRightDown,
	ssKeyIdxLeftDown,
	ssKeyIdxStart,
	ssKeyIdxA,
	ssKeyIdxB,
	ssKeyIdxC,
	ssKeyIdxX,
	ssKeyIdxY,
	ssKeyIdxZ,
	ssKeyIdxL,
	ssKeyIdxR,
	ssKeyIdxATurbo,
	ssKeyIdxBTurbo,
	ssKeyIdxCTurbo,
	ssKeyIdxXTurbo,
	ssKeyIdxYTurbo,
	ssKeyIdxZTurbo,
	ssKeyIdxLastGamepad = ssKeyIdxZTurbo
};

const char *EmuSystem::inputFaceBtnName = "A-Z";
const char *EmuSystem::inputCenterBtnName = "Start";
const uint EmuSystem::inputFaceBtns = 8;
const uint EmuSystem::inputCenterBtns = 1;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
const uint EmuSystem::maxPlayers = 2;

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerOffset = player ? EmuControls::gamepadKeys : 0;
	map[SysVController::F_ELEM] = ssKeyIdxA + playerOffset;
	map[SysVController::F_ELEM+1] = ssKeyIdxB + playerOffset;
	map[SysVController::F_ELEM+2] = ssKeyIdxC + playerOffset;
	map[SysVController::F_ELEM+3] = ssKeyIdxX + playerOffset;
	map[SysVController::F_ELEM+4] = ssKeyIdxY + playerOffset;
	map[SysVController::F_ELEM+5] = ssKeyIdxZ + playerOffset;
	map[SysVController::F_ELEM+6] = ssKeyIdxL + playerOffset;
	map[SysVController::F_ELEM+7] = ssKeyIdxR + playerOffset;

	map[SysVController::C_ELEM] = ssKeyIdxStart + playerOffset;

	map[SysVController::D_ELEM] = ssKeyIdxLeftUp + playerOffset;
	map[SysVController::D_ELEM+1] = ssKeyIdxUp + playerOffset;
	map[SysVController::D_ELEM+2] = ssKeyIdxRightUp + playerOffset;
	map[SysVController::D_ELEM+3] = ssKeyIdxLeft + playerOffset;
	map[SysVController::D_ELEM+5] = ssKeyIdxRight + playerOffset;
	map[SysVController::D_ELEM+6] = ssKeyIdxLeftDown + playerOffset;
	map[SysVController::D_ELEM+7] = ssKeyIdxDown + playerOffset;
	map[SysVController::D_ELEM+8] = ssKeyIdxRightDown + playerOffset;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case ssKeyIdxXTurbo:
		case ssKeyIdxYTurbo:
		case ssKeyIdxZTurbo:
		case ssKeyIdxATurbo:
		case ssKeyIdxBTurbo:
		case ssKeyIdxCTurbo:
		case ssKeyIdxXTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxYTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxZTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxATurbo + EmuControls::gamepadKeys:
		case ssKeyIdxBTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxCTurbo + EmuControls::gamepadKeys:
			turbo = 1; [[fallthrough]];
		default: return input;
	}
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = 0;
	if(emuKey > ssKeyIdxLastGamepad)
	{
		player = 1;
		emuKey -= EmuControls::gamepadKeys;
	}
	PerPad_struct *p = (player == 1) ? pad[1] : pad[0];
	bool pushed = state == Input::PUSHED;
	switch(emuKey)
	{
		bcase ssKeyIdxUp: if(pushed) PerPadUpPressed(p); else PerPadUpReleased(p);
		bcase ssKeyIdxRight: if(pushed) PerPadRightPressed(p); else PerPadRightReleased(p);
		bcase ssKeyIdxDown: if(pushed) PerPadDownPressed(p); else PerPadDownReleased(p);
		bcase ssKeyIdxLeft: if(pushed) PerPadLeftPressed(p); else PerPadLeftReleased(p);
		bcase ssKeyIdxLeftUp: if(pushed) { PerPadLeftPressed(p); PerPadUpPressed(p); }
			else { PerPadLeftReleased(p); PerPadUpReleased(p); }
		bcase ssKeyIdxRightUp: if(pushed) { PerPadRightPressed(p); PerPadUpPressed(p); }
			else { PerPadRightReleased(p); PerPadUpReleased(p); }
		bcase ssKeyIdxRightDown: if(pushed) { PerPadRightPressed(p); PerPadDownPressed(p); }
			else { PerPadRightReleased(p); PerPadDownReleased(p); }
		bcase ssKeyIdxLeftDown: if(pushed) { PerPadLeftPressed(p); PerPadDownPressed(p); }
			else { PerPadLeftReleased(p); PerPadDownReleased(p); }
		bcase ssKeyIdxStart: if(pushed) PerPadStartPressed(p); else PerPadStartReleased(p);
		bcase ssKeyIdxXTurbo:
		case ssKeyIdxX: if(pushed) PerPadXPressed(p); else PerPadXReleased(p);
		bcase ssKeyIdxYTurbo:
		case ssKeyIdxY: if(pushed) PerPadYPressed(p); else PerPadYReleased(p);
		bcase ssKeyIdxZTurbo:
		case ssKeyIdxZ: if(pushed) PerPadZPressed(p); else PerPadZReleased(p);
		bcase ssKeyIdxATurbo:
		case ssKeyIdxA: if(pushed) PerPadAPressed(p); else PerPadAReleased(p);
		bcase ssKeyIdxBTurbo:
		case ssKeyIdxB: if(pushed) PerPadBPressed(p); else PerPadBReleased(p);
		bcase ssKeyIdxCTurbo:
		case ssKeyIdxC: if(pushed) PerPadCPressed(p); else PerPadCReleased(p);
		bcase ssKeyIdxL: if(pushed) PerPadLTriggerPressed(p); else PerPadLTriggerReleased(p);
		bcase ssKeyIdxR: if(pushed) PerPadRTriggerPressed(p); else PerPadRTriggerReleased(p);
		bdefault: bug_unreachable("input == %d", emuKey);
	}
}

void EmuSystem::clearInputBuffers(EmuInputView &)
{
	PerPortReset();
	pad[0] = PerPadAdd(&PORTDATA1);
	pad[1] = PerPadAdd(&PORTDATA2);
}
