#pragma once

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "NgpEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Option";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2004\nthe NeoPop Team\nwww.nih.at";
static const uint systemFaceBtns = 2, systemCenterBtns = 1;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 1;
#define systemAspectRatioString "20:19"

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 13;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys;

}
