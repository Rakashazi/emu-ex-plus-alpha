#pragma once

#define CONFIG_FILE_NAME "MdEmu.config"

static const char *touchConfigFaceBtnName = "A/B/C", *touchConfigCenterBtnName = "Mode/Start";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 1;

namespace EmuCheats
{

static const uint MAX = 100;
static const uint MAX_CODE_TYPES = 1;

}

namespace EmuControls
{

using namespace Input;
static const uint categories = 5;
static const uint gamepadKeys = 22;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*4;

}
