#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "NesEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Select/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nFCEUX Team\nfceux.com";
static const uint systemFaceBtns = 2, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 5;
static const uint gamepadKeys = 15;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*4;

}

