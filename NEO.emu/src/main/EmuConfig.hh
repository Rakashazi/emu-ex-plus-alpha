#pragma once

#define CONFIG_FILE_NAME "NeoEmu.config"

static const char *touchConfigFaceBtnName = "A/B/C/D", *touchConfigCenterBtnName = "Select/Start";
static const uint systemFaceBtns = 4, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 4;
static const uint switchKeys = 1;
static const uint joystickKeys = 19;
static const uint systemTotalKeys = gameActionKeys + switchKeys + joystickKeys*2;

}
