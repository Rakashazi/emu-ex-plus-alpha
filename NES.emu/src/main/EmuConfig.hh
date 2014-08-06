#pragma once

#define CONFIG_FILE_NAME "NesEmu.config"

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Select/Start";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuCheats
{

static const uint MAX = 254;
static const uint MAX_CODE_TYPES = 2;

}

namespace EmuControls
{

using namespace Input;
static const uint categories = 5;
static const uint gamepadKeys = 15;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*4;

}

//#define EMU_FRAMEWORK_BUNDLED_GAMES
