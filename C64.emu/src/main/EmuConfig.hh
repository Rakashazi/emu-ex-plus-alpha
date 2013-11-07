#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "C64Emu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "JS Buttons", *touchConfigCenterBtnName = "F1/KB";
#define CONFIG_VCONTROLLER_KEYBOARD
static const uint systemFaceBtns = 2, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 4;
static const uint gamepadKeys = 11;
static const uint kbKeys = 74;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*2 + kbKeys;

}
