#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "MdEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B/C", *touchConfigCenterBtnName = "Mode/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 1;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 5;
static const uint gamepadKeys = 22;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*4;

}
