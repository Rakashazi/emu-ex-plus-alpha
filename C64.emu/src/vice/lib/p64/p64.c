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

#include "p64.h"

p64_uint32_t P64CRC32(p64_uint8_t* Data, p64_uint32_t Len) {

    const p64_uint32_t CRC32Table[16] = {0x00000000UL, 0x1db71064UL, 0x3b6e20c8UL, 0x26d930acUL,
                                         0x76dc4190UL, 0x6b6b51f4UL, 0x4db26158UL, 0x5005713cUL,
                                         0xedb88320UL, 0xf00f9344UL, 0xd6d6a3e8UL, 0xcb61b38cUL,
                                         0x9b64c2b0UL, 0x86d3d2d4UL, 0xa00ae278UL, 0xbdbdf21cUL
                                        };

    p64_uint32_t value, pos;

    if(!Len) {
        return 0;
    }

    for(value = 0xffffffffUL, pos = 0; pos < Len; pos++) {
        value ^= Data[pos];
        value = CRC32Table[value & 0xfUL] ^(value >> 4);
        value = CRC32Table[value & 0xfUL] ^(value >> 4);
    }

    return value ^ 0xffffffffUL;
}

typedef p64_uint32_t* PP64RangeCoderProbabilities;

typedef struct {
    p64_uint8_t* Buffer;
    p64_uint32_t BufferSize;
    p64_uint32_t BufferPosition;
    p64_uint32_t RangeCode;
    p64_uint32_t RangeLow;
    p64_uint32_t RangeHigh;
    p64_uint32_t RangeMiddle;
} TP64RangeCoder;

typedef TP64RangeCoder* PP64RangeCoder;

PP64RangeCoderProbabilities P64RangeCoderProbabilitiesAllocate(p64_uint32_t Count) {
    return p64_malloc(Count * sizeof(p64_uint32_t));
}

void P64RangeCoderProbabilitiesFree(PP64RangeCoderProbabilities Probabilities) {
    p64_free(Probabilities);
}

void P64RangeCoderProbabilitiesReset(PP64RangeCoderProbabilities Probabilities, p64_uint32_t Count) {
    p64_uint32_t Index;
    for(Index = 0; Index < Count; Index++) {
        Probabilities[Index] = 2048;
    }
}

p64_uint8_t P64RangeCoderRead(PP64RangeCoder Instance) {
    if(Instance->BufferPosition < Instance->BufferSize) {
        return Instance->Buffer[Instance->BufferPosition++];
    }
    return 0;
}

void P64RangeCoderWrite(PP64RangeCoder Instance, p64_uint8_t Value) {
    if(Instance->BufferPosition >= Instance->BufferSize) {
        if(Instance->BufferSize < 16) {
            Instance->BufferSize = 16;
        }
        while(Instance->BufferPosition >= Instance->BufferSize) {
            Instance->BufferSize += Instance->BufferSize;
        }
        if(Instance->Buffer) {
            Instance->Buffer = p64_realloc(Instance->Buffer, Instance->BufferSize);
        } else {
            Instance->Buffer = p64_malloc(Instance->BufferSize);
        }
    }
    Instance->Buffer[Instance->BufferPosition++] = Value;
}

void P64RangeCoderInit(PP64RangeCoder Instance) {
    Instance->RangeCode = 0;
    Instance->RangeLow = 0;
    Instance->RangeHigh = 0xffffffffUL;
}

void P64RangeCoderStart(PP64RangeCoder Instance) {
    p64_uint32_t Counter;
    for(Counter = 0; Counter < 4; Counter++) {
        Instance->RangeCode = (Instance->RangeCode << 8) | P64RangeCoderRead(Instance);
    }
}

void P64RangeCoderFlush(PP64RangeCoder Instance) {
    p64_uint32_t Counter;
    for(Counter = 0; Counter < 4; Counter++) {
        P64RangeCoderWrite(Instance, (p64_uint8_t)(Instance->RangeHigh >> 24));
        Instance->RangeHigh <<= 8;
    }
}

void P64RangeCoderEncodeNormalize(PP64RangeCoder Instance) {
    while(!((Instance->RangeLow ^ Instance->RangeHigh) & 0xff000000UL)) {
        P64RangeCoderWrite(Instance, (p64_uint8_t)(Instance->RangeHigh >> 24));
        Instance->RangeLow <<= 8;
        Instance->RangeHigh = (Instance->RangeHigh << 8) | 0xffUL;
    }
}

p64_uint32_t P64RangeCoderEncodeBit(PP64RangeCoder Instance, p64_uint32_t* Probability, p64_uint32_t Shift, p64_uint32_t BitValue) {
    Instance->RangeMiddle = Instance->RangeLow + ((p64_uint32_t)((p64_uint32_t)(Instance->RangeHigh - Instance->RangeLow) >> 12) * (*Probability));
    if(BitValue) {
        *Probability += (p64_uint32_t)((0xfffUL - *Probability) >> Shift);
        Instance->RangeHigh = Instance->RangeMiddle;
    } else {
        *Probability -= *Probability >> Shift;
        Instance->RangeLow = Instance->RangeMiddle + 1;
    }
    P64RangeCoderEncodeNormalize(Instance);
    return BitValue;
}

p64_uint32_t P64RangeCoderEncodeBitWithoutProbability(PP64RangeCoder Instance, p64_uint32_t BitValue) {
    Instance->RangeMiddle = Instance->RangeLow + ((Instance->RangeHigh - Instance->RangeLow) >> 1);
    if(BitValue) {
        Instance->RangeHigh = Instance->RangeMiddle;
    } else {
        Instance->RangeLow = Instance->RangeMiddle + 1;
    }
    P64RangeCoderEncodeNormalize(Instance);
    return BitValue;
}

void P64RangeCoderDecodeNormalize(PP64RangeCoder Instance) {
    while(!((Instance->RangeLow ^ Instance->RangeHigh) & 0xff000000UL)) {
        Instance->RangeLow <<= 8;
        Instance->RangeHigh = (Instance->RangeHigh << 8) | 0xffUL;
        Instance->RangeCode = (Instance->RangeCode << 8) | P64RangeCoderRead(Instance);
    }
}

p64_uint32_t P64RangeCoderDecodeBit(PP64RangeCoder Instance, p64_uint32_t *Probability, p64_uint32_t Shift) {
    p64_uint32_t bit;
    Instance->RangeMiddle = Instance->RangeLow + ((p64_uint32_t)((p64_uint32_t)(Instance->RangeHigh - Instance->RangeLow) >> 12) * (*Probability));
    if(Instance->RangeCode <= Instance->RangeMiddle) {
        *Probability += (p64_uint32_t)((0xfffUL - *Probability) >> Shift);
        Instance->RangeHigh = Instance->RangeMiddle;
        bit = 1;
    } else {
        *Probability -= *Probability >> Shift;
        Instance->RangeLow = Instance->RangeMiddle + 1;
        bit = 0;
    }
    P64RangeCoderDecodeNormalize(Instance);
    return bit;
}

p64_uint32_t P64RangeCoderDecodeBitWithoutProbability(PP64RangeCoder Instance) {
    p64_uint32_t bit;
    Instance->RangeMiddle = Instance->RangeLow + ((Instance->RangeHigh - Instance->RangeLow) >> 1);
    if(Instance->RangeCode <= Instance->RangeMiddle) {
        Instance->RangeHigh = Instance->RangeMiddle;
        bit = 1;
    } else {
        Instance->RangeLow = Instance->RangeMiddle + 1;
        bit = 0;
    }
    P64RangeCoderDecodeNormalize(Instance);
    return bit;
}

p64_uint32_t P64RangeCoderEncodeDirectBits(PP64RangeCoder Instance, p64_uint32_t Bits, p64_uint32_t Value) {
    while(Bits--) {
        P64RangeCoderEncodeBitWithoutProbability(Instance, (Value >> Bits) & 1);
    }
    return Value;
}

p64_uint32_t P64RangeCoderDecodeDirectBits(PP64RangeCoder Instance, p64_uint32_t Bits) {
    p64_uint32_t Value = 0;
    while(Bits--) {
        Value += Value + P64RangeCoderDecodeBitWithoutProbability(Instance);
    }
    return Value;
}

void P64MemoryStreamCreate(PP64MemoryStream Instance) {
    memset(Instance, 0, sizeof(TP64MemoryStream));
}

void P64MemoryStreamDestroy(PP64MemoryStream Instance) {
    if(Instance->Data) {
        p64_free(Instance->Data);
    }
    memset(Instance, 0, sizeof(TP64MemoryStream));
}

void P64MemoryStreamClear(PP64MemoryStream Instance) {
    if(Instance->Data) {
        p64_free(Instance->Data);
    }
    memset(Instance, 0, sizeof(TP64MemoryStream));
}

p64_uint32_t P64MemoryStreamSeek(PP64MemoryStream Instance, p64_uint32_t Position) {
    if(Position < Instance->Size) {
        Instance->Position = Position;
    }
    return Instance->Position;
}

p64_uint32_t P64MemoryStreamRead(PP64MemoryStream Instance, p64_uint8_t* Data, p64_uint32_t Count) {
    p64_uint32_t ToDo = 0;
    if((Count > 0) && (Instance->Position < Instance->Size)) {
        ToDo = Instance->Size - Instance->Position;
        if(ToDo > Count) {
            ToDo = Count;
        }
        memmove(Data, Instance->Data + Instance->Position, ToDo);
        Instance->Position += ToDo;
    }
    return ToDo;
}

p64_uint32_t P64MemoryStreamWrite(PP64MemoryStream Instance, p64_uint8_t* Data, p64_uint32_t Count) {
    if(Count) {
        if((Instance->Position + Count) >= Instance->Allocated) {
            if(Instance->Allocated < 16) {
                Instance->Allocated = 16;
            }
            while((Instance->Position + Count) >= Instance->Allocated) {
                Instance->Allocated += Instance->Allocated;
            }
            if(Instance->Data) {
                Instance->Data = p64_realloc(Instance->Data, Instance->Allocated);
            } else {
                Instance->Data = p64_malloc(Instance->Allocated);
            }
        }
        memmove(Instance->Data + Instance->Position, Data, Count);
        Instance->Position += Count;
        if(Instance->Size < Instance->Position) {
            Instance->Size = Instance->Position;
        }
        return Count;
    } else {
        return 0;
    }
}

p64_int32_t P64MemoryStreamReadByte(PP64MemoryStream Instance, p64_uint8_t* Data) {
    return P64MemoryStreamRead(Instance, Data, sizeof(p64_uint8_t)) ? 1 : 0;
}

p64_int32_t P64MemoryStreamReadWord(PP64MemoryStream Instance, p64_uint16_t* Data) {
    p64_uint8_t b[2];
    if(!P64MemoryStreamReadByte(Instance, &b[0])) {
        return 0;
    }
    if(!P64MemoryStreamReadByte(Instance, &b[1])) {
        return 0;
    }
    *Data = (p64_uint8_t)(((p64_uint16_t)b[0]) | (((p64_uint16_t)b[1]) << 8));
    return 1;
}

p64_int32_t P64MemoryStreamReadDWord(PP64MemoryStream Instance, p64_uint32_t* Data) {
    p64_uint16_t w[2];
    if(!P64MemoryStreamReadWord(Instance, &w[0])) {
        return 0;
    }
    if(!P64MemoryStreamReadWord(Instance, &w[1])) {
        return 0;
    }
    *Data = ((p64_uint32_t)w[0]) | (((p64_uint32_t)w[1]) << 16);
    return 1;
}

p64_int32_t P64MemoryStreamWriteByte(PP64MemoryStream Instance, p64_uint8_t* Data) {
    return P64MemoryStreamWrite(Instance, Data, sizeof(p64_uint8_t)) ? 1 : 0;
}

p64_int32_t P64MemoryStreamWriteWord(PP64MemoryStream Instance, p64_uint16_t* Data) {
    p64_uint8_t b[2];
    b[0] = (p64_uint8_t)(*Data & 0xffUL);
    b[1] = (p64_uint8_t)((*Data >> 8) & 0xffUL);
    if(!P64MemoryStreamWriteByte(Instance, &b[0])) {
        return 0;
    }
    if(!P64MemoryStreamWriteByte(Instance, &b[1])) {
        return 0;
    }
    return 1;
}

p64_int32_t P64MemoryStreamWriteDWord(PP64MemoryStream Instance, p64_uint32_t* Data) {
    p64_uint16_t w[2];
    w[0] = (p64_uint16_t)(*Data & 0xffffUL);
    w[1] = (p64_uint16_t)((*Data >> 16) & 0xffffUL);
    if(!P64MemoryStreamWriteWord(Instance, &w[0])) {
        return 0;
    }
    if(!P64MemoryStreamWriteWord(Instance, &w[1])) {
        return 0;
    }
    return 1;
}

p64_uint32_t P64MemoryStreamAssign(PP64MemoryStream Instance, PP64MemoryStream FromInstance) {
    if(Instance->Data) {
        p64_free(Instance->Data);
    }
    memset(Instance, 0, sizeof(TP64MemoryStream));
    Instance->Data = p64_malloc(FromInstance->Allocated);
    Instance->Size = FromInstance->Size;
    Instance->Allocated = FromInstance->Allocated;
    Instance->Position = 0;
    if(Instance->Size) {
        memmove(Instance->Data, FromInstance->Data, Instance->Size);
    }
    return Instance->Size;
}

p64_uint32_t P64MemoryStreamAppend(PP64MemoryStream Instance, PP64MemoryStream FromInstance) {
    if(FromInstance->Size) {
        FromInstance->Position = FromInstance->Size;
        return P64MemoryStreamWrite(Instance, FromInstance->Data, FromInstance->Size);
    }
    return 0;
}

p64_uint32_t P64MemoryStreamAppendFrom(PP64MemoryStream Instance, PP64MemoryStream FromInstance) {
    if((FromInstance->Size > 0) && (FromInstance->Position < FromInstance->Size)) {
        if(P64MemoryStreamWrite(Instance, FromInstance->Data + FromInstance->Position, FromInstance->Size - FromInstance->Position)) {
            FromInstance->Position = FromInstance->Size;
            return 1;
        }
        FromInstance->Position = FromInstance->Size;
    }
    return 0;
}

p64_uint32_t P64MemoryStreamAppendFromCount(PP64MemoryStream Instance, PP64MemoryStream FromInstance, p64_uint32_t Count) {
    p64_uint32_t ToDo = 0;
    if((Count > 0) && (FromInstance->Position < FromInstance->Size)) {
        ToDo = FromInstance->Size - FromInstance->Position;
        if(ToDo > Count) {
            ToDo = Count;
        }
        if(ToDo > 0) {
            ToDo = P64MemoryStreamWrite(Instance, FromInstance->Data + FromInstance->Position, ToDo);
            FromInstance->Position += ToDo;
        }
    }
    return ToDo;
}

void P64PulseStreamCreate(PP64PulseStream Instance) {
    memset(Instance, 0, sizeof(TP64PulseStream));
    Instance->Pulses = 0;
    Instance->PulsesAllocated = 0;
    Instance->PulsesCount = 0;
    Instance->UsedFirst = -1;
    Instance->UsedLast = -1;
    Instance->FreeList = -1;
    Instance->CurrentIndex = -1;
}

void P64PulseStreamDestroy(PP64PulseStream Instance) {
    P64PulseStreamClear(Instance);
    memset(Instance, 0, sizeof(TP64PulseStream));
}

void P64PulseStreamClear(PP64PulseStream Instance) {
    if(Instance->Pulses) {
        p64_free(Instance->Pulses);
    }
    Instance->Pulses = 0;
    Instance->PulsesAllocated = 0;
    Instance->PulsesCount = 0;
    Instance->UsedFirst = -1;
    Instance->UsedLast = -1;
    Instance->FreeList = -1;
    Instance->CurrentIndex = -1;
}

p64_int32_t P64PulseStreamAllocatePulse(PP64PulseStream Instance) {
    p64_int32_t Index;
    if(Instance->FreeList < 0) {
        if(Instance->PulsesCount >= Instance->PulsesAllocated) {
            if(Instance->PulsesAllocated < 16) {
                Instance->PulsesAllocated = 16;
            }
            while(Instance->PulsesCount >= Instance->PulsesAllocated) {
                Instance->PulsesAllocated += Instance->PulsesAllocated;
            }
            if(Instance->Pulses) {
                Instance->Pulses = p64_realloc(Instance->Pulses, Instance->PulsesAllocated * sizeof(TP64Pulse));
            } else {
                Instance->Pulses = p64_malloc(Instance->PulsesAllocated * sizeof(TP64Pulse));
            }
        }
        Index = (p64_int32_t)(Instance->PulsesCount++);
    } else {
        Index = Instance->FreeList;
        Instance->FreeList = Instance->Pulses[Index].Next;
    }
    Instance->Pulses[Index].Previous    = -1;
    Instance->Pulses[Index].Next    = -1;
    Instance->Pulses[Index].Position    = 0;
    Instance->Pulses[Index].Strength    = 0;
    return Index;
}

void P64PulseStreamFreePulse(PP64PulseStream Instance, p64_int32_t Index) {
    if(Instance->CurrentIndex == Index) {
        Instance->CurrentIndex = Instance->Pulses[Index].Next;
    }
    if(Instance->Pulses[Index].Previous < 0) {
        Instance->UsedFirst = Instance->Pulses[Index].Next;
    } else {
        Instance->Pulses[Instance->Pulses[Index].Previous].Next = Instance->Pulses[Index].Next;
    }
    if(Instance->Pulses[Index].Next < 0) {
        Instance->UsedLast = Instance->Pulses[Index].Previous;
    } else {
        Instance->Pulses[Instance->Pulses[Index].Next].Previous = Instance->Pulses[Index].Previous;
    }
    Instance->Pulses[Index].Previous = -1;
    Instance->Pulses[Index].Next = Instance->FreeList;
    Instance->FreeList = Index;
}

void P64PulseStreamAddPulse(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Strength) {
    p64_int32_t Current, Index;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Instance->UsedLast >= 0) && (Instance->Pulses[Instance->UsedLast].Position < Position)) {
        Current = -1;
    } else {
        if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
            Current = Instance->UsedFirst;
        }
        while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
            Current = Instance->Pulses[Current].Next;
        }
    }
    if(Current < 0) {
        Index = P64PulseStreamAllocatePulse(Instance);
        if(Instance->UsedLast < 0) {
            Instance->UsedFirst = Index;
        } else {
            Instance->Pulses[Instance->UsedLast].Next = Index;
            Instance->Pulses[Index].Previous = Instance->UsedLast;
        }
        Instance->UsedLast = Index;
    } else {
        if(Instance->Pulses[Current].Position == Position) {
            Index = Current;
        } else {
            Index = P64PulseStreamAllocatePulse(Instance);
            Instance->Pulses[Index].Previous = Instance->Pulses[Current].Previous;
            Instance->Pulses[Index].Next = Current;
            Instance->Pulses[Current].Previous = Index;
            if(Instance->Pulses[Index].Previous < 0) {
                Instance->UsedFirst = Index;
            } else {
                Instance->Pulses[Instance->Pulses[Index].Previous].Next = Index;
            }
        }
    }
    Instance->Pulses[Index].Position = Position;
    Instance->Pulses[Index].Strength = Strength;
    Instance->CurrentIndex = Index;
}

void P64PulseStreamRemovePulses(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Count) {
    p64_uint32_t ToDo;
    p64_int32_t Current, Next;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    while(Count) {
        ToDo = ((Position + Count) > P64PulseSamplesPerRotation) ? (P64PulseSamplesPerRotation - Position) : Count;
        Current = Instance->CurrentIndex;
        if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
            Current = Instance->UsedFirst;
        }
        while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
            Current = Instance->Pulses[Current].Next;
        }
        while((Current >= 0) && ((Instance->Pulses[Current].Position >= Position) && (Instance->Pulses[Current].Position < (Position + ToDo)))) {
            Next = Instance->Pulses[Current].Next;
            P64PulseStreamFreePulse(Instance, Current);
            Current = Next;
        }
        Position += ToDo;
        Count -= ToDo;
    }
}

void P64PulseStreamRemovePulse(PP64PulseStream Instance, p64_uint32_t Position) {
    p64_int32_t Current;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
        Current = Instance->UsedFirst;
    }
    while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
        Current = Instance->Pulses[Current].Next;
    }
    if((Current >= 0) && (Instance->Pulses[Current].Position == Position)) {
        P64PulseStreamFreePulse(Instance, Current);
    }
}

p64_uint32_t P64PulseStreamDeltaPositionToNextPulse(PP64PulseStream Instance, p64_uint32_t Position) {
    p64_int32_t Current;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
        Current = Instance->UsedFirst;
    }
    while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
        Current = Instance->Pulses[Current].Next;
    }
    if(Current < 0) {
        if(Instance->UsedFirst < 0) {
            return P64PulseSamplesPerRotation - Position;
        } else {
            return (P64PulseSamplesPerRotation + Instance->Pulses[Instance->UsedFirst].Position) - Position;
        }
    } else {
        Instance->CurrentIndex = Current;
        return Instance->Pulses[Current].Position - Position;
    }
}

p64_uint32_t P64PulseStreamGetNextPulse(PP64PulseStream Instance, p64_uint32_t Position) {
    p64_int32_t Current;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
        Current = Instance->UsedFirst;
    }
    while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
        Current = Instance->Pulses[Current].Next;
    }
    if(Current < 0) {
        if(Instance->UsedFirst < 0) {
            return 0;
        } else {
            return Instance->Pulses[Instance->UsedFirst].Strength;
        }
    } else {
        Instance->CurrentIndex = Current;
        return Instance->Pulses[Current].Strength;
    }
}

p64_uint32_t P64PulseStreamGetPulseCount(PP64PulseStream Instance) {
    p64_int32_t Current, Count = 0;
    Current = Instance->CurrentIndex;
    while(Current >= 0) {
        Count++;
        Current = Instance->Pulses[Current].Next;
    }
    return (p64_uint32_t)Count;
}

p64_uint32_t P64PulseStreamGetPulse(PP64PulseStream Instance, p64_uint32_t Position) {
    p64_int32_t Current;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
        Current = Instance->UsedFirst;
    }
    while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
        Current = Instance->Pulses[Current].Next;
    }
    if((Current < 0) || (Instance->Pulses[Current].Position != Position)) {
        return 0;
    } else {
        Instance->CurrentIndex = Current;
        return Instance->Pulses[Current].Strength;
    }
}

void P64PulseStreamSetPulse(PP64PulseStream Instance, p64_uint32_t Position, p64_uint32_t Strength) {
    if(Strength) {
        P64PulseStreamAddPulse(Instance, Position, Strength);
    } else {
        P64PulseStreamRemovePulse(Instance, Position);
    }
}

void P64PulseStreamSeek(PP64PulseStream Instance, p64_uint32_t Position) {
    p64_int32_t Current;
    while(Position >= P64PulseSamplesPerRotation) {
        Position -= P64PulseSamplesPerRotation;
    }
    Current = Instance->CurrentIndex;
    if((Current < 0) || ((Current != Instance->UsedFirst) && ((Instance->Pulses[Current].Previous >= 0) && (Instance->Pulses[Instance->Pulses[Current].Previous].Position >= Position)))) {
        Current = Instance->UsedFirst;
    }
    while((Current >= 0) && (Instance->Pulses[Current].Position < Position)) {
        Current = Instance->Pulses[Current].Next;
    }
    Instance->CurrentIndex = Current;
}

void P64PulseStreamConvertFromGCR(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len) {
    p64_uint32_t PositionHi, PositionLo, IncrementHi, IncrementLo, BitStreamPosition;
    P64PulseStreamClear(Instance);
    if(Len) {
        IncrementHi = P64PulseSamplesPerRotation / Len;
        IncrementLo = P64PulseSamplesPerRotation % Len;
        PositionHi = (P64PulseSamplesPerRotation >> 1) / Len;
        PositionLo = (P64PulseSamplesPerRotation >> 1) % Len;
        for(BitStreamPosition = 0; BitStreamPosition < Len; BitStreamPosition++) {
            if(((p64_uint8_t)(Bytes[BitStreamPosition >> 3])) & (1 << ((~BitStreamPosition) & 7))) {
                P64PulseStreamAddPulse(Instance, PositionHi, 0xffffffffUL);
            }
            PositionHi += IncrementHi;
            PositionLo += IncrementLo;
            while(PositionLo >= Len) {
                PositionLo -= Len;
                PositionHi++;
            }

        }
    }
}

void P64PulseStreamConvertToGCR(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len) {
    p64_uint32_t Range, PositionHi, PositionLo, IncrementHi, IncrementLo, BitStreamPosition;
    p64_int32_t Current;
    if(Len) {
        memset(Bytes, 0, (Len + 7) >> 3);
        Range = P64PulseSamplesPerRotation;
        IncrementHi = Range / Len;
        IncrementLo = Range % Len;
        Current = Instance->UsedFirst;
        PositionHi = (Current >= 0) ? Instance->Pulses[Current].Position - 1 : 0;
        PositionLo = Len - 1;
        for(BitStreamPosition = 0; BitStreamPosition < Len; BitStreamPosition++) {
            PositionHi += IncrementHi;
            PositionLo += IncrementLo;
            while(PositionLo >= Len) {
                PositionLo -= Len;
                PositionHi++;
            }
            while(1) {
                if((Current >= 0) && (Instance->Pulses[Current].Position < PositionHi)) {
                    PositionHi = (Instance->Pulses[Current].Position + IncrementHi) - 20; /* 1.25 microseconds headroom */
                    PositionLo = IncrementLo;
                    Current = Instance->Pulses[Current].Next;
                    Bytes[BitStreamPosition >> 3] |= (p64_uint8_t)(1 << ((~BitStreamPosition) & 7));
                } else if(PositionHi >= Range) {
                    PositionHi -= Range;
                    Current = Instance->UsedFirst;
                    continue;
                }
                break;
            }
        }

        /* optional: add here GCR byte-realigning-to-syncmark-borders code, if your GCR routines are working bytewise-only */

    }
}

p64_uint32_t P64PulseStreamConvertToGCRWithLogic(PP64PulseStream Instance, p64_uint8_t* Bytes, p64_uint32_t Len, p64_uint32_t SpeedZone) {
    p64_uint32_t Position, LastPosition, Delta, DelayCounter, FlipFlop, LastFlipFlop, Clock, Counter, BitStreamPosition;
    p64_int32_t Current;
    if(Len) {
        memset(Bytes, 0, (Len + 7) >> 3);
        LastPosition = 0;
        FlipFlop = 0;
        LastFlipFlop = 0;
        Clock = SpeedZone;
        Counter = 0;
        BitStreamPosition = 0;
        Current = Instance->UsedFirst;
        while((Current >= 0) && (BitStreamPosition < Len)) {
            if(Instance->Pulses[Current].Strength >= 0x80000000UL) {
                Position = Instance->Pulses[Current].Position;
                Delta = Position - LastPosition;
                LastPosition = Position;
                DelayCounter = 0;
                FlipFlop ^= 1;
                do {
                    if((DelayCounter == 40) && (LastFlipFlop != FlipFlop)) {
                        LastFlipFlop = FlipFlop;
                        Clock = SpeedZone;
                        Counter = 0;
                    }
                    if(Clock == 16) {
                        Clock = SpeedZone;
                        Counter = (Counter + 1) & 0xfUL;
                        if((Counter & 3) == 2) {
                            Bytes[BitStreamPosition >> 3] |= (p64_uint8_t)((((Counter + 0x1c) >> 4) & 1) << ((~BitStreamPosition) & 7));
                            BitStreamPosition++;
                        }
                    }
                    Clock++;
                } while(++DelayCounter < Delta);
            }
            Current = Instance->Pulses[Current].Next;
        }

        /* optional: add here GCR byte-realigning-to-syncmark-borders code, if your GCR routines are working bytewise-only */

        return BitStreamPosition;

    }

    return 0;
}

#define ModelPosition 0
#define ModelStrength 4
#define ModelPositionFlag 8
#define ModelStrengthFlag 9

#define ProbabilityModelCount 10

const p64_uint32_t ProbabilityCounts[ProbabilityModelCount] = {65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 4, 4};

p64_uint32_t P64PulseStreamReadFromStream(PP64PulseStream Instance, PP64MemoryStream Stream) {
    PP64RangeCoderProbabilities RangeCoderProbabilities;
    p64_uint32_t RangeCoderProbabilityOffsets[ProbabilityModelCount];
    p64_uint32_t RangeCoderProbabilityStates[ProbabilityModelCount];
    TP64RangeCoder RangeCoderInstance;
    p64_uint32_t ProbabilityCount, Index, Count, DeltaPosition, Position, Strength, result, CountPulses, Size;
    p64_uint8_t *Buffer;

    if(P64MemoryStreamReadDWord(Stream, &CountPulses)) {

        if(P64MemoryStreamReadDWord(Stream, &Size)) {

            if(!Size) {
                return CountPulses ? 0 : 1;
            }

            Buffer = p64_malloc(Size);

            if(P64MemoryStreamRead(Stream, Buffer, Size) == Size) {

                ProbabilityCount = 0;
                for(Index = 0; Index < ProbabilityModelCount; Index++) {
                    RangeCoderProbabilityOffsets[Index] = ProbabilityCount;
                    ProbabilityCount += ProbabilityCounts[Index];
                    RangeCoderProbabilityStates[Index] = 0;
                }
                RangeCoderProbabilities = P64RangeCoderProbabilitiesAllocate(ProbabilityCount);
                P64RangeCoderProbabilitiesReset(RangeCoderProbabilities, ProbabilityCount);

                memset(&RangeCoderInstance, 0, sizeof(TP64RangeCoder));
                P64RangeCoderInit(&RangeCoderInstance);

                RangeCoderInstance.Buffer = Buffer;
                RangeCoderInstance.BufferSize = Size;
                RangeCoderInstance.BufferPosition = 0;
                P64RangeCoderStart(&RangeCoderInstance);

                Count = 0;

                Position = 0;
                DeltaPosition = 0;

                Strength = 0;

#define ReadBit(Model) (RangeCoderProbabilityStates[Model] = P64RangeCoderDecodeBit(&RangeCoderInstance, RangeCoderProbabilities + (RangeCoderProbabilityOffsets[Model] + RangeCoderProbabilityStates[Model]), 4))

#define ReadDWord(Model) \
          { \
            p64_uint32_t ByteValue, ByteIndex, Context; \
            p64_int32_t Bit; \
                      result = 0; \
                      for (ByteIndex = 0; ByteIndex < 4; ByteIndex++) { \
                          Context = 1; \
                          for (Bit = 7; Bit >= 0; Bit--) { \
                              Context = (Context << 1) | P64RangeCoderDecodeBit(&RangeCoderInstance, RangeCoderProbabilities + (RangeCoderProbabilityOffsets[Model + ByteIndex] + (((RangeCoderProbabilityStates[Model + ByteIndex] << 8) | Context) & 0xffffUL)), 4); \
                          } \
                          ByteValue = Context & 0xffUL; \
                          RangeCoderProbabilityStates[Model + ByteIndex] = ByteValue; \
                          result |= (p64_uint32_t)(((ByteValue & 0xffUL) << (ByteIndex << 3))); \
            } \
                    } \

                while(Count < CountPulses) {

                    if(ReadBit(ModelPositionFlag)) {
                        ReadDWord(ModelPosition);
                        DeltaPosition = result;
                        if(!DeltaPosition) {
                            break;
                        }
                    }
                    Position += DeltaPosition;

                    if(ReadBit(ModelStrengthFlag)) {
                        ReadDWord(ModelStrength);
                        Strength += result;
                    }

                    P64PulseStreamAddPulse(Instance, Position, Strength);

                    Count++;
                }

                P64RangeCoderProbabilitiesFree(RangeCoderProbabilities);

                p64_free(Buffer);

                return Count == CountPulses;

            }

            p64_free(Buffer);
        }

    }

#undef ReadBit
#undef ReadDWord

    return 0;
}

p64_uint32_t P64PulseStreamWriteToStream(PP64PulseStream Instance, PP64MemoryStream Stream) {
    PP64RangeCoderProbabilities RangeCoderProbabilities;
    p64_uint32_t RangeCoderProbabilityOffsets[ProbabilityModelCount];
    p64_uint32_t RangeCoderProbabilityStates[ProbabilityModelCount];
    TP64RangeCoder RangeCoderInstance;
    p64_int32_t Index, Current;
    p64_uint32_t ProbabilityCount, LastPosition, PreviousDeltaPosition, DeltaPosition, LastStrength, CountPulses, Size;

    ProbabilityCount = 0;
    for(Index = 0; Index < ProbabilityModelCount; Index++) {
        RangeCoderProbabilityOffsets[Index] = ProbabilityCount;
        ProbabilityCount += ProbabilityCounts[Index];
        RangeCoderProbabilityStates[Index] = 0;
    }
    RangeCoderProbabilities = P64RangeCoderProbabilitiesAllocate(ProbabilityCount);
    P64RangeCoderProbabilitiesReset(RangeCoderProbabilities, ProbabilityCount);

    memset(&RangeCoderInstance, 0, sizeof(TP64RangeCoder));
    P64RangeCoderInit(&RangeCoderInstance);

    LastPosition = 0;
    PreviousDeltaPosition = 0;

    LastStrength = 0;

#define WriteBit(Model, BitValue) RangeCoderProbabilityStates[Model] = P64RangeCoderEncodeBit(&RangeCoderInstance, RangeCoderProbabilities + (RangeCoderProbabilityOffsets[Model] + RangeCoderProbabilityStates[Model]), 4, BitValue)

#define WriteDWord(Model, DWordValue) \
      { \
        p64_uint32_t ByteValue, ByteIndex, Context, Value; \
        p64_int32_t Bit; \
        Value = DWordValue; \
               for (ByteIndex = 0; ByteIndex < 4; ByteIndex++) { \
                  ByteValue = (Value >> (ByteIndex << 3)) & 0xffUL; \
                  Context = 1; \
                  for (Bit = 7; Bit >= 0; Bit--) { \
                      Context = (Context << 1) | P64RangeCoderEncodeBit(&RangeCoderInstance, RangeCoderProbabilities + (RangeCoderProbabilityOffsets[Model + ByteIndex] + (((RangeCoderProbabilityStates[Model+ByteIndex] << 8) | Context) & 0xffffUL)), 4, (ByteValue >> Bit) & 1); \
                  } \
                  RangeCoderProbabilityStates[Model + ByteIndex] = ByteValue; \
              }    \
            }

    CountPulses = 0;

    Current = Instance->UsedFirst;
    while(Current >= 0) {
        DeltaPosition = Instance->Pulses[Current].Position - LastPosition;
        if(PreviousDeltaPosition != DeltaPosition) {
            PreviousDeltaPosition = DeltaPosition;
            WriteBit(ModelPositionFlag, 1);
            WriteDWord(ModelPosition, DeltaPosition);
        } else {
            WriteBit(ModelPositionFlag, 0);
        }
        LastPosition = Instance->Pulses[Current].Position;

        if(LastStrength != Instance->Pulses[Current].Strength) {
            WriteBit(ModelStrengthFlag, 1);
            WriteDWord(ModelStrength, Instance->Pulses[Current].Strength - LastStrength);
        } else {
            WriteBit(ModelStrengthFlag, 0);
        }
        LastStrength = Instance->Pulses[Current].Strength;

        CountPulses++;

        Current = Instance->Pulses[Current].Next;
    }

    WriteBit(ModelPositionFlag, 1);
    WriteDWord(ModelPosition, 0);

    P64RangeCoderFlush(&RangeCoderInstance);

    P64RangeCoderProbabilitiesFree(RangeCoderProbabilities);

    if(RangeCoderInstance.Buffer) {
        Size = RangeCoderInstance.BufferPosition;
    } else {
        Size = 0;
    }

    if(P64MemoryStreamWriteDWord(Stream, &CountPulses)) {
        if(P64MemoryStreamWriteDWord(Stream, &Size)) {
            if(RangeCoderInstance.Buffer) {
                if(P64MemoryStreamWrite(Stream, RangeCoderInstance.Buffer, RangeCoderInstance.BufferPosition) == RangeCoderInstance.BufferPosition) {
                    p64_free(RangeCoderInstance.Buffer);
                    return 1;
                }
                p64_free(RangeCoderInstance.Buffer);
                return 0;
            }
            return 1;
        }
    }

#undef WriteBit
#undef WriteDWord
    return 0;
}

void P64ImageCreate(PP64Image Instance) {
    p64_int32_t HalfTrack, side;
    memset(Instance, 0, sizeof(TP64Image));
    Instance->noSides = 1;
    for(side=0; side<2; side++)
    for(HalfTrack = 0; HalfTrack <= P64LastHalfTrack; HalfTrack++) {
        P64PulseStreamCreate(&Instance->PulseStreams[side][HalfTrack]);
    }
    P64ImageClear(Instance);
}

void P64ImageDestroy(PP64Image Instance) {
    p64_int32_t HalfTrack, side;
    for(side=0; side<2; side++)
    for(HalfTrack = 0; HalfTrack <= P64LastHalfTrack; HalfTrack++) {
        P64PulseStreamDestroy(&Instance->PulseStreams[side][HalfTrack]);
    }
    memset(Instance, 0, sizeof(TP64Image));
}

void P64ImageClear(PP64Image Instance) {
    p64_int32_t HalfTrack, side;
    Instance->WriteProtected = 0;
    for(side=0; side<2; side++)
    for(HalfTrack = 0; HalfTrack <= P64LastHalfTrack; HalfTrack++) {
        P64PulseStreamClear(&Instance->PulseStreams[side][HalfTrack]);
    }
}

p64_uint32_t P64ImageReadFromStream(PP64Image Instance, PP64MemoryStream Stream) {
    TP64MemoryStream ChunksMemoryStream, ChunkMemoryStream;
    p64_uint32_t Version, Flags, Size, Checksum, HalfTrack, OK, side;
    TP64HeaderSignature HeaderSignature;
    TP64ChunkSignature ChunkSignature;

    OK = 0;
    P64ImageClear(Instance);
    if(P64MemoryStreamSeek(Stream, 0) == 0) {
        if(P64MemoryStreamRead(Stream, (void*)&HeaderSignature, sizeof(TP64HeaderSignature)) == sizeof(TP64HeaderSignature)) {
            if((HeaderSignature[0] == 'P') && (HeaderSignature[1] == '6') && (HeaderSignature[2] == '4') && (HeaderSignature[3] == '-') && (HeaderSignature[4] == '1') && (HeaderSignature[5] == '5') && (HeaderSignature[6] == '4') && (HeaderSignature[7] == '1')) {
                if(P64MemoryStreamReadDWord(Stream, &Version)) {
                    if(Version == 0x00000000) {
                        if(P64MemoryStreamReadDWord(Stream, &Flags)) {
                            if(P64MemoryStreamReadDWord(Stream, &Size)) {
                                if(P64MemoryStreamReadDWord(Stream, &Checksum)) {
                                    Instance->WriteProtected = (Flags & 1) != 0;
                                    Instance->noSides = 1+!!(Flags & 2);
                                    P64MemoryStreamCreate(&ChunksMemoryStream);
                                    if(P64MemoryStreamAppendFromCount(&ChunksMemoryStream, Stream, Size) == Size) {
                                        if(P64CRC32(ChunksMemoryStream.Data, Size) == Checksum) {
                                            if(P64MemoryStreamSeek(&ChunksMemoryStream, 0) == 0) {
                                                OK = 1;
                                                while(OK && (ChunksMemoryStream.Position < ChunksMemoryStream.Size)) {
                                                    if(P64MemoryStreamRead(&ChunksMemoryStream, (void*)&ChunkSignature, sizeof(TP64ChunkSignature)) == sizeof(TP64ChunkSignature)) {
                                                        if(P64MemoryStreamReadDWord(&ChunksMemoryStream, &Size)) {
                                                            if(P64MemoryStreamReadDWord(&ChunksMemoryStream, &Checksum)) {
                                                                OK = 0;
                                                                P64MemoryStreamCreate(&ChunkMemoryStream);
                                                                if(Size == 0) {
                                                                    if(Checksum == 0) {
                                                                        if((ChunkSignature[0] == 'D') && (ChunkSignature[1] == 'O') && (ChunkSignature[2] == 'N') && (ChunkSignature[3] == 'E')) {
                                                                            OK = 1;
                                                                        } else {
                                                                            OK = 1;
                                                                        }
                                                                    }
                                                                } else {
                                                                    if(P64MemoryStreamAppendFromCount(&ChunkMemoryStream, &ChunksMemoryStream, Size) == Size) {
                                                                        if(P64MemoryStreamSeek(&ChunkMemoryStream, 0) == 0) {
                                                                            if(P64CRC32(ChunkMemoryStream.Data, Size) == Checksum) {
                                                                                if((ChunkSignature[0] == 'H') && (ChunkSignature[1] == 'T') && (ChunkSignature[2] == 'P') && (((ChunkSignature[3] & 127) >= P64FirstHalfTrack) && ((ChunkSignature[3] & 127) <= P64LastHalfTrack))) {
                                                                                    HalfTrack = ChunkSignature[3] & 127;
                                                                                    side = !!(ChunkSignature[3] & 128);
                                                                                    OK = P64PulseStreamReadFromStream(&Instance->PulseStreams[side][HalfTrack], &ChunkMemoryStream);
                                                                                } else {
                                                                                    OK = 1;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                                P64MemoryStreamDestroy(&ChunkMemoryStream);
                                                                continue;
                                                            }
                                                        }
                                                    }
                                                    break;
                                                }

                                            }
                                        }
                                    }
                                    P64MemoryStreamDestroy(&ChunksMemoryStream);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return OK;
}

p64_uint32_t P64ImageWriteToStream(PP64Image Instance, PP64MemoryStream Stream) {
    TP64MemoryStream MemoryStream, ChunksMemoryStream, ChunkMemoryStream;
    p64_uint32_t Version, Flags, Size, Checksum, HalfTrack, result, WriteChunkResult, side;

    TP64HeaderSignature HeaderSignature;
    TP64ChunkSignature ChunkSignature;

#define WriteChunk() \
{ \
    WriteChunkResult = 0; \
    Size = ChunkMemoryStream.Size; \
    Checksum = (ChunkMemoryStream.Size > 0) ? P64CRC32(ChunkMemoryStream.Data,ChunkMemoryStream.Size) : 0; \
    if (P64MemoryStreamWrite(&ChunksMemoryStream, (void*)&ChunkSignature, sizeof(TP64ChunkSignature)) == sizeof(TP64ChunkSignature)) { \
        if (P64MemoryStreamWriteDWord(&ChunksMemoryStream, &Size)) { \
            if (P64MemoryStreamWriteDWord(&ChunksMemoryStream, &Checksum)) { \
                if (ChunkMemoryStream.Size == 0) { \
                    WriteChunkResult = 1; \
                }else{ \
                    if (P64MemoryStreamSeek(&ChunkMemoryStream, 0) == 0) { \
                        if (P64MemoryStreamAppendFromCount(&ChunksMemoryStream, &ChunkMemoryStream, ChunkMemoryStream.Size) == ChunkMemoryStream.Size) { \
                            WriteChunkResult = 1; \
                        } \
                    } \
          } \
            } \
        } \
    } \
}

    result = 0;

    HeaderSignature[0] = 'P';
    HeaderSignature[1] = '6';
    HeaderSignature[2] = '4';
    HeaderSignature[3] = '-';
    HeaderSignature[4] = '1';
    HeaderSignature[5] = '5';
    HeaderSignature[6] = '4';
    HeaderSignature[7] = '1';
    Version = 0x00000000;

    P64MemoryStreamCreate(&MemoryStream);
    P64MemoryStreamCreate(&ChunksMemoryStream);

    result = 1;
    for (side = 0; side < (p64_uint32_t)Instance->noSides; side++)
    for(HalfTrack = P64FirstHalfTrack; HalfTrack <= P64LastHalfTrack; HalfTrack++) {

        P64MemoryStreamCreate(&ChunkMemoryStream);
        result = P64PulseStreamWriteToStream(&Instance->PulseStreams[side][HalfTrack], &ChunkMemoryStream);
        if(result) {
            ChunkSignature[0] = 'H';
            ChunkSignature[1] = 'T';
            ChunkSignature[2] = 'P';
            ChunkSignature[3] = (p64_uint8_t)(HalfTrack + 128*side);
            WriteChunk();
            result = WriteChunkResult;
        }
        P64MemoryStreamDestroy(&ChunkMemoryStream);
        if(!result) {
            break;
        }
    }


    if(result) {

        P64MemoryStreamCreate(&ChunkMemoryStream);
        ChunkSignature[0] = 'D';
        ChunkSignature[1] = 'O';
        ChunkSignature[2] = 'N';
        ChunkSignature[3] = 'E';
        WriteChunk();
        result = WriteChunkResult;
        P64MemoryStreamDestroy(&ChunkMemoryStream);

        if(result) {
            result = 0;

            Flags = 0;
            if(Instance->WriteProtected) {
                Flags |= 1;
            }
            if(Instance->noSides == 2) {
                Flags |= 2;
            }

            Size = ChunksMemoryStream.Size;
            Checksum = P64CRC32(ChunksMemoryStream.Data, Size);

            if(P64MemoryStreamWrite(&MemoryStream, (void*)&HeaderSignature, sizeof(TP64HeaderSignature)) == sizeof(TP64HeaderSignature)) {
                if(P64MemoryStreamWriteDWord(&MemoryStream, &Version)) {
                    if(P64MemoryStreamWriteDWord(&MemoryStream, &Flags)) {
                        if(P64MemoryStreamWriteDWord(&MemoryStream, &Size)) {
                            if(P64MemoryStreamWriteDWord(&MemoryStream, &Checksum)) {
                                if(P64MemoryStreamSeek(&ChunksMemoryStream, 0) == 0) {
                                    if(P64MemoryStreamAppendFromCount(&MemoryStream, &ChunksMemoryStream, ChunksMemoryStream.Size) == ChunksMemoryStream.Size) {
                                        if(P64MemoryStreamSeek(&MemoryStream, 0) == 0) {
                                            if(P64MemoryStreamAppendFromCount(Stream, &MemoryStream, MemoryStream.Size) == MemoryStream.Size) {
                                                result = 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }

    }

    P64MemoryStreamDestroy(&ChunksMemoryStream);
    P64MemoryStreamDestroy(&MemoryStream);

#undef WriteChunk

    return result;
}
