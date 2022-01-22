#include "../System.h"

void gbafilter_pal(uint16_t* buf, int count, int redShift, int greenShift, int blueShift);
void gbafilter_pal32(uint32_t* buf, int count, int redShift, int greenShift, int blueShift);
void gbafilter_pad(uint8_t* buf, int count, int redShift, int greenShift, int blueShift);
