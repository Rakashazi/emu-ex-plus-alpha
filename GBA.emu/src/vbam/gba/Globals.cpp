#include "GBA.h"

#ifdef BKPT_SUPPORT
int  oldreg[18];
char oldbuffer[10];
#endif

int saveType = 0;
bool useBios = false;
bool skipBios = false;
bool cpuIsMultiBoot = false;
bool parseDebug = true;
int cpuSaveType = 0;
#ifdef USE_CHEATS
bool cheatsEnabled = false;
#endif
bool skipSaveGameBattery = false;
bool skipSaveGameCheats = false;

// this is an optional hack to change the backdrop/background color:
// -1: disabled
// 0x0000 to 0x7FFF: set custom 15 bit color
//int customBackdropColor = -1;
