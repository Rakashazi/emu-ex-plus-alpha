#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "GbcEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Select/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011\nthe Gambatte Team\ngambatte.sourceforge.net";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
static const char *systemAspectRatioString = "10:9";

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 14;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys;

}
