#pragma once

#define CONFIG_FILE_NAME "2600emu.config"

static const char *touchConfigFaceBtnName = "JS Buttons", *touchConfigCenterBtnName = "Select/Reset";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint joystickKeys = 10;
static const uint switchKeys = 5;
static const uint keyboardKeys = 12;
static const uint systemTotalKeys = gameActionKeys + joystickKeys*2 + switchKeys + keyboardKeys*2;

}

//#define EMU_FRAMEWORK_BUNDLED_GAMES
