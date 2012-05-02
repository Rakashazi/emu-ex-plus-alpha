/*
**
** File: ym2151.h - header file for software implementation of YM2151
**                                            FM Operator Type-M(OPM)
**
** (c) 1997-2002 Jarek Burczynski (s0246@poczta.onet.pl, bujar@mame.net)
** Some of the optimizing ideas by Tatsuyuki Satoh
**
** Version 2.150 final beta May, 11th 2002
**
**
** I would like to thank following people for making this project possible:
**
** Beauty Planets - for making a lot of real YM2151 samples and providing
** additional informations about the chip. Also for the time spent making
** the samples and the speed of replying to my endless requests.
**
** Shigeharu Isoda - for general help, for taking time to scan his YM2151
** Japanese Manual first of all, and answering MANY of my questions.
**
** Nao - for giving me some info about YM2151 and pointing me to Shigeharu.
** Also for creating fmemu (which I still use to test the emulator).
**
** Aaron Giles and Chris Hardy - they made some samples of one of my favourite
** arcade games so I could compare it to my emulator.
**
** Bryan McPhail and Tim (powerjaw) - for making some samples.
**
** Ishmair - for the datasheet and motivation.
*/

#ifndef _H_YM2151_
#define _H_YM2151_

#include "MsxTypes.h"

typedef struct MameYm2151 MameYm2151;

MameYm2151* YM2151Create(void* ref, int clock, int rate);
void YM2151Destroy(MameYm2151* chip);
void YM2151ResetChip(MameYm2151* chip);
void YM2151UpdateOne(MameYm2151* chip,Int16* bufL, Int16* bufR, int length);
void YM2151WriteReg(MameYm2151* chip, int r, int v);
int  YM2151ReadStatus(MameYm2151* chip);
void YM2151TimerCallback(MameYm2151* chip, int timer);
void YM2151LoadState(MameYm2151* chip);
void YM2151SaveState(MameYm2151* chip);
#endif /*_H_YM2151_*/
