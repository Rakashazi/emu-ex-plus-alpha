/*
*************************************************************
** P64 reference implementation by Benjamin 'BeRo' Rosseaux *
*************************************************************
**
** Copyright (c) 2011-2012, Benjamin Rosseaux
**
** This software is provided 'as-is', without any express or implied
** warranty. In no event will the authors be held liable for any damages
** arising from the use of this software.
**
** Permission is granted to anyone to use this software for any purpose,
** including commercial applications, and to alter it and redistribute it
** freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**    claim that you wrote the original software. If you use this software
**    in a product, an acknowledgment in the product documentation would be
**    appreciated but is not required.
**
**    2. Altered source versions must be plainly marked as such, and must not be
**    misrepresented as being the original software.
**
**    3. This notice may not be removed or altered from any source
**   distribution.
**
*/

#ifndef P64_H
#define P64_H

#include "p64config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef P64_USE_STDINT
#include <stdint.h>
#endif
#include <string.h>

#ifndef p64_malloc
#define p64_malloc malloc
#endif

#ifndef p64_realloc
#define p64_realloc realloc
#endif

#ifndef p64_free
#define p64_free free
#endif

/* (16 MHz * 60) / 300 = 3200000 samples per track rotation (at 5 rotations per second) */
#define P64PulseSamplesPerRotation 3200000

#define P64FirstHalfTrack 2

/* including 42.5 */
#define P64LastHalfTrack 85

#ifdef P64_USE_STDINT
typedef int8_t p64_int8_t;
typedef int16_t p64_int16_t;
typedef int32_t p64_int32_t;

typedef uint8_t p64_uint8_t;
typedef uint16_t p64_uint16_t;
typedef uint32_t p64_uint32_t;
#else
#ifndef P64_USE_OWN_TYPES
typedef signed char p64_int8_t;
typedef signed short p64_int16_t;
typedef signed int p64_int32_t;

typedef unsigned char p64_uint8_t;
typedef unsigned short p64_uint16_t;
typedef unsigned int p64_uint32_t;
#endif
#endif

typedef p64_uint8_t TP64HeaderSignature[8];

typedef TP64HeaderSignature* PP64HeaderSignature;

typedef p64_uint8_t TP64ChunkSignature[4];

typedef TP64ChunkSignature* PP64ChunkSignature;

typedef struct {
	p64_int32_t Previous;
	p64_int32_t Next;
	p64_uint32_t Position;
	p64_uint32_t Strength;
} TP64Pulse;

typedef TP64Pulse* PP64Pulse;

typedef TP64Pulse* PP64Pulses;

typedef struct {
	PP64Pulses Pulses;
	p64_uint32_t PulsesAllocated;
	p64_uint32_t PulsesCount;
	p64_int32_t UsedFirst;
	p64_int32_t UsedLast;
	p64_int32_t FreeList;
	p64_int32_t CurrentIndex;
} TP64PulseStream;

typedef TP64PulseStream* PP64PulseStream;

typedef TP64PulseStream TP64PulseStreams[(P64LastHalfTrack-0)+2];

typedef TP64PulseStreams* PP64PulseStreams;

typedef struct {
	TP64PulseStreams PulseStreams;
	p64_uint32_t WriteProtected;
} TP64Image;

typedef TP64Image* PP64Image;

typedef struct {
	p64_uint8_t* Data;
	p64_uint32_t Allocated;
	p64_uint32_t Size;
	p64_uint32_t Position;
} TP64MemoryStream;

typedef TP64MemoryStream* PP64MemoryStream;

extern void P64MemoryStreamCreate(PP64MemoryStream Instance);
extern void P64MemoryStreamDestroy(PP64MemoryStream Instance);
extern void P64MemoryStreamClear(PP64MemoryStream Instance);
extern p64_uint32_t P64MemoryStreamSeek(PP64MemoryStream Instance, p64_uint32_t Position);
extern p64_uint32_t P64MemoryStreamRead(PP64MemoryStream Instance, p64_uint8_t* Data, p64_uint32_t Count);
extern p64_uint32_t P64MemoryStreamWrite(PP64MemoryStream Instance, p64_uint8_t* Data, p64_uint32_t Count);
extern p64_int32_t P64MemoryStreamReadByte(PP64MemoryStream Instance, p64_uint8_t* Data);
extern p64_int32_t P64MemoryStreamReadWord(PP64MemoryStream Instance, p64_uint16_t* Data);
extern p64_int32_t P64MemoryStreamReadDWord(PP64MemoryStream Instance, p64_uint32_t* Data);
extern p64_int32_t P64MemoryStreamWriteByte(PP64MemoryStream Instance, p64_uint8_t* Data);
extern p64_int32_t P64MemoryStreamWriteWord(PP64MemoryStream Instance, p64_uint16_t* Data);
extern p64_int32_t P64MemoryStreamWriteDWord(PP64MemoryStream Instance, p64_uint32_t* Data);
extern p64_uint32_t P64MemoryStreamAssign(PP64MemoryStream Instance, PP64MemoryStream FromInstance);
extern p64_uint32_t P64MemoryStreamAppend(PP64MemoryStream Instance, PP64MemoryStream FromInstance);
extern p64_uint32_t P64MemoryStreamAppendFrom(PP64MemoryStream Instance, PP64MemoryStream FromInstance);
extern p64_uint32_t P64MemoryStreamAppendFromCount(PP64MemoryStream Instance, PP64MemoryStream FromInstance, p64_uint32_t Count);

extern void P64PulseStreamCreate(PP64PulseStream Instance);
extern void P64PulseStreamDestroy(PP64PulseStream Instance);
extern void P64PulseStreamClear(PP64PulseStream Instance);
extern p64_int32_t P64PulseStreamAllocatePulse(PP64PulseStream Instance);
extern void P64PulseStreamFreePulse(PP64PulseStream Instance, p64_int32_t Index);
extern void P64PulseStreamAddPulse(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Strength);
extern void P64PulseStreamRemovePulses(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Count);
extern void P64PulseStreamRemovePulse(PP64PulseStream Instance, p64_uint32_t Position);
extern p64_uint32_t P64PulseStreamDeltaPositionToNextPulse(PP64PulseStream Instance, p64_uint32_t Position);
extern p64_uint32_t P64PulseStreamGetNextPulse(PP64PulseStream Instance, p64_uint32_t Position);
extern p64_uint32_t P64PulseStreamGetPulseCount(PP64PulseStream Instance);
extern p64_uint32_t P64PulseStreamGetPulse(PP64PulseStream Instance, p64_uint32_t Position);
extern void P64PulseStreamSetPulse(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Strength);
extern void P64PulseStreamSeek(PP64PulseStream Instance, p64_uint32_t Position);
extern void P64PulseStreamConvertFromGCR(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len);
extern void P64PulseStreamConvertToGCR(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len);
extern p64_uint32_t P64PulseStreamConvertToGCRWithLogic(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len, p64_uint32_t SpeedZone);
extern p64_uint32_t P64PulseStreamReadFromStream(PP64PulseStream Instance, PP64MemoryStream Stream);
extern p64_uint32_t P64PulseStreamWriteToStream(PP64PulseStream Instance, PP64MemoryStream Stream);

extern void P64ImageCreate(PP64Image Instance);
extern void P64ImageDestroy(PP64Image Instance);
extern void P64ImageClear(PP64Image Instance);
extern p64_uint32_t P64ImageReadFromStream(PP64Image Instance, PP64MemoryStream Stream);
extern p64_uint32_t P64ImageWriteToStream(PP64Image Instance, PP64MemoryStream Stream);

#endif
