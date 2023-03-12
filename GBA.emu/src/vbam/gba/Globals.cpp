#include "GBA.h"

#ifdef BKPT_SUPPORT
int  oldreg[18];
char oldbuffer[10];
#endif

// this is an optional hack to change the backdrop/background color:
// -1: disabled
// 0x0000 to 0x7FFF: set custom 15 bit color
//int customBackdropColor = -1;
