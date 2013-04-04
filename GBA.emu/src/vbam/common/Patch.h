#ifndef PATCH_H
#define PATCH_H

#include "Types.h"

bool applyPatch(const char *patchname, u8 **rom, int *size);
bool patchApplyIPS(const char *patchname, u8 **rom, int *size);
bool patchApplyUPS(const char *patchname, u8 **rom, int *size);
bool patchApplyPPF(const char *patchname, u8 **rom, int *size);

#endif // PATCH_H
