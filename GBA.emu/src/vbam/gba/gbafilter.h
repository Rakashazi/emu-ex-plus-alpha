#include "../System.h"

void gbafilter_pal(u16 * buf, int count, int redShift, int greenShift, int blueShift);
void gbafilter_pal32(u32 * buf, int count, int redShift, int greenShift, int blueShift);
void gbafilter_pad(u8 * buf, int count, int colorDepth, int redShift, int greenShift, int blueShift);
