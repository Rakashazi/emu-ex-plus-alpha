#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "MdEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B/C", *touchConfigCenterBtnName = "Mode/Start";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 1;
#define systemAspectRatioString "4:3"

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
