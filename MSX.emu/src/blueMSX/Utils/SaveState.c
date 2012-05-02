/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Utils/SaveState.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008/06/25 22:26:17 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "SaveState.h"
#include "ziphelper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Must be power of 2
#define ALLOC_BLOCK_SIZE 256

struct SaveState {
    UInt32 allocSize;
    UInt32 size;
    UInt32 offset;
    UInt32 *buffer;
    char   fileName[64];
};

static char stateFile[512];

static UInt32 tagFromName(const char* tagName)
{
    UInt32 tag = 0;
    UInt32 mod = 1;

    while (*tagName) {
        mod *= 19219;
        tag += mod * *tagName++;
    }

    return tag;
}

#if 0
static void checkTag(SaveState* state, const char* tagName)
{
    UInt32 tag = tagFromName(tagName);
    UInt32 offset = 0;

    while (offset < state->offset) {
        UInt32 elemTag = state->buffer[offset++];
        UInt32 elemLen = state->buffer[offset++];
        if (elemTag == tag) {
            printf("Tag duplicated: %s : %d\n", state->fileName, tagName);
        }
        offset += (elemLen + sizeof(UInt32) - 1) / sizeof(UInt32);
    }
}
#else
#define checkTag(state, tagName)
#endif

static struct {
    char fileName[64];
    int  count;
} saveFileTable[128];

static int tableCount;

static char* getIndexedFilename(const char* fileName)
{
    static char indexedFileName[64];
    int i;

    for (i = 0; i < tableCount; i++) {
        if (0 == strcmp(fileName, saveFileTable[i].fileName)) {
            sprintf(indexedFileName, "%s_%.2d", fileName, ++saveFileTable[i].count);
            return indexedFileName;
        }
    }
    strcpy(saveFileTable[tableCount].fileName, fileName);
    saveFileTable[tableCount++].count = 0;

    strcpy(indexedFileName, fileName);
    strcat(indexedFileName, "_00");

    return indexedFileName;
}

void saveStateCreateForRead(const char* fileName)
{
    tableCount = 0;
    strcpy(stateFile, fileName);
    zipCacheReadOnlyZip(fileName);
}

void saveStateCreateForWrite(const char* fileName)
{
    tableCount = 0;
    strcpy(stateFile, fileName);
}

void saveStateDestroy(void)
{
    zipCacheReadOnlyZip(NULL);
}

SaveState* saveStateOpenForRead(const char* fileName) {
    SaveState* state = (SaveState*)malloc(sizeof(SaveState));
    Int32 size = 0;
    void* buffer = zipLoadFile(stateFile, getIndexedFilename(fileName), &size);

    state->allocSize = size;
    state->buffer = buffer;
    state->size = size / sizeof(UInt32);
    state->offset = 0;
    state->fileName[0] = 0;

    return state;
}

SaveState* saveStateOpenForWrite(const char* fileName) {
    SaveState* state = (SaveState*)malloc(sizeof(SaveState));

    state->size      = 0;
    state->offset    = 0;
    state->buffer    = NULL;
    state->allocSize = 0;

    strcpy(state->fileName, getIndexedFilename(fileName));

    return state;
}

void saveStateClose(SaveState* state) {
    if (state->fileName[0]) {
        zipSaveFile(stateFile, state->fileName, 1, state->buffer, state->offset * sizeof(UInt32));
    }
    if (state->buffer != NULL) {
        free(state->buffer);
    }
    state->allocSize = 0;
    free(state);
}

static void stateExtendBuffer(SaveState* state, UInt32 extend) {
    state->size += extend;
    if (state->size > state->allocSize)
    {
        state->allocSize = (state->size + ALLOC_BLOCK_SIZE - 1) & ~(ALLOC_BLOCK_SIZE - 1);
        state->buffer = realloc(state->buffer, state->allocSize * sizeof(UInt32));
    }
}

void saveStateSet(SaveState* state, const char* tagName, UInt32 value)
{
    checkTag(state, tagName);

    stateExtendBuffer(state, 3);
    state->buffer[state->offset++] = tagFromName(tagName);
    state->buffer[state->offset++] = sizeof(UInt32);
    state->buffer[state->offset++] = value;
}

void saveStateSetBuffer(SaveState* state, const char* tagName, void* buffer, UInt32 length)
{
    checkTag(state, tagName);

    stateExtendBuffer(state, 2 + (length + sizeof(UInt32) - 1) / sizeof(UInt32));
    state->buffer[state->offset++] = tagFromName(tagName);
    state->buffer[state->offset++] = length;
    memcpy(state->buffer + state->offset, buffer, length);
    state->offset += (length + sizeof(UInt32) - 1) / sizeof(UInt32);
}

UInt32 saveStateGet(SaveState* state, const char* tagName, UInt32 defValue)
{
    UInt32 tag = tagFromName(tagName);
    UInt32 startOffset = state->offset;
    UInt32 offset = state->offset;
    UInt32 value = defValue;
    UInt32 wrapAround = 0;
    UInt32 elemTag;
    UInt32 elemLen;

    if (state->size == 0) {
        return value;
    }

    do {
        elemTag = state->buffer[offset++];
        elemLen = state->buffer[offset++];
        if (elemTag == tag) {
            value = state->buffer[offset];
        }
        offset += (elemLen + sizeof(UInt32) - 1) / sizeof(UInt32);
        if (offset >= state->size) {
            if (++wrapAround > 1) {
                break;
            }
            offset = 0;
        }
    } while (offset != startOffset && elemTag != tag);

    return value;
}

void saveStateGetBuffer(SaveState* state, const char* tagName, void* buffer, UInt32 length)
{
    UInt32 tag = tagFromName(tagName);
    UInt32 startOffset = state->offset;
    UInt32 offset = state->offset;
    UInt32 wrapAround = 0;
    UInt32 elemTag;
    UInt32 elemLen;

    if (state->size == 0) {
        return;
    }

    do {
        elemTag = state->buffer[offset++];
        elemLen = state->buffer[offset++];
        if (elemTag == tag) {
            memcpy(buffer, state->buffer + offset, length < elemLen ? length : elemLen);
        }
        offset += (elemLen + sizeof(UInt32) - 1) / sizeof(UInt32);
        if (offset >= state->size) {
            if (++wrapAround > 1) {
                break;
            }
            offset = 0;
        }
    } while (offset != startOffset && elemTag != tag);

    state->offset = offset;
}
