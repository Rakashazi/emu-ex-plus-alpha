#ifndef _MVS_H_
#define _MVS_H_

//#include "SDL.h"

/* compatibility layer */
/*
#define s8  signed char
#define s16 signed short
#define s32 signed long

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned long
*/
#define s8  Sint8
#define s16 Sint16
#define s32 Sint32

#define u8  Uint8
#define u16 Uint16
#define u32 Uint32

#define ALIGN_DATA
#ifndef INLINE
#define INLINE static __inline__
#endif
#define SOUND_SAMPLES 512

#define Limit(val, max, min)                    \
{                                               \
        if (val > max) val = max;               \
        else if (val < min) val = min;          \
}

#endif
