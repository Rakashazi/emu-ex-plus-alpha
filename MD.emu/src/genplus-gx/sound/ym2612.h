/*
**
** software implementation of Yamaha FM sound generator (YM2612/YM3438)
**
** Original code (MAME fm.c)
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta) 
**
** Additional code & fixes by Eke-Eke for Genesis Plus GX
**
*/

#ifndef _H_YM2612_
#define _H_YM2612_

#include "genplus-config.h"

extern void YM2612Init(double clock, int rate);
extern void YM2612ResetChip(void);
extern void YM2612Update(FMSampleType *buffer, int length);
extern void YM2612Write(unsigned int a, unsigned int v);
extern unsigned int YM2612Read(void);
extern unsigned char *YM2612GetContextPtr(void);
extern unsigned int YM2612GetContextSize(void);
extern int YM2612Restore(unsigned char *state, bool hasExcessData, unsigned ptrSize);
extern void YM2612RestoreContext(unsigned char *buffer);
extern int YM2612LoadContext(unsigned char *state, bool hasExcessData, unsigned ptrSize);
extern int YM2612SaveContext(unsigned char *state);

#endif /* _YM2612_ */
