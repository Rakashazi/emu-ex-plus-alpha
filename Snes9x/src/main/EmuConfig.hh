#pragma once

#include <cheats.h>
#include <meta.h>

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#ifdef SNES9X_VERSION_1_4
		#define CONFIG_FILE_NAME "Snes9x.config"
	#else
		#define CONFIG_FILE_NAME "Snes9xP.config"
	#endif
#else
	#define CONFIG_FILE_NAME "config"
#endif

static const char *touchConfigFaceBtnName = "A/B/X/Y", *touchConfigCenterBtnName = "Select/Start";
static const uint systemFaceBtns = 6, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 1, systemHasRevBtnLayout = 0;

namespace EmuCheats
{

static const uint MAX = MAX_CHEATS;
static const uint MAX_CODE_TYPES = 1;

}

namespace EmuControls
{

using namespace Input;
static const uint categories = 6;
static const uint gamepadKeys = 20;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys*5;

}

#ifndef SNES9X_VERSION_1_4
#define EMU_FRAMEWORK_BUNDLED_GAMES
#endif
