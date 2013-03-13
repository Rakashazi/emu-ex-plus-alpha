#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "GbcEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Select/Start";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
static const char *systemAspectRatioString = "10:9";

namespace EmuCheats
{

static const uint MAX = 255;
static const uint MAX_CODE_TYPES = 1;

}

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 14;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys;

}
