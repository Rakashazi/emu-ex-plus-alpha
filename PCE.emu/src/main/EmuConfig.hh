#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "PceEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "I/II", *touchConfigCenterBtnName = "Select/Run";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.sourceforge.net";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint gamepadKeys = 18;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*5;

}
