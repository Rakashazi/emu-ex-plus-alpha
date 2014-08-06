#pragma once

#define CONFIG_FILE_NAME "PceEmu.config"

static const char *touchConfigFaceBtnName = "I/II", *touchConfigCenterBtnName = "Select/Run";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint gamepadKeys = 18;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*5;

}
