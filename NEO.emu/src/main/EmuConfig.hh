#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "NeoEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B/C/D", *touchConfigCenterBtnName = "Select/Start";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
static const uint systemFaceBtns = 4, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
#define systemAspectRatioString "4:3"

namespace EmuControls
{

using namespace Input;
static const uint categories = 4;
static const uint switchKeys = 1;
static const uint joystickKeys = 19;
static const uint systemTotalKeys = gameActionKeys + switchKeys + joystickKeys*2;

}
