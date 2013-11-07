#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "SaturnEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A-Z", *touchConfigCenterBtnName = "Start";
static const uint systemFaceBtns = 8, systemCenterBtns = 1;
static const bool systemHasTriggerBtns = 1, systemHasRevBtnLayout = 0;

namespace EmuControls
{

using namespace Input;
static const uint categories = 3;
static const uint gamepadKeys = 23;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*2;

}
