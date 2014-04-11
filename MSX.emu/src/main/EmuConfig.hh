#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "MsxEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Space/KB";
#define CONFIG_VCONTROLLER_KEYBOARD
static const uint systemFaceBtns = 2, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint joystickKeys = 12;
static const uint colecoNumericKeys = 12;
static const uint kbKeys = 93;
static const uint systemTotalKeys = gameActionKeys + joystickKeys*2 + colecoNumericKeys*2 + kbKeys;

}

