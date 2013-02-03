#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "Snes9xP.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B/X/Y", *touchConfigCenterBtnName = "Select/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 1996-2011 the\nSnes9x Team\nwww.snes9x.com";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 1, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint gamepadKeys = 20;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*5;

}
