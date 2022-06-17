#ifndef PATCH_H
#define PATCH_H

#include "../Util.h"
#include "Types.h"

bool applyPatch(const char *patchname, uint8_t **rom, int *size);
bool patchApplyIPS(FILE* f, uint8_t** rom, int *size);
bool patchApplyUPS(FILE* f, uint8_t** rom, int *size);
bool patchApplyPPF(FILE* f, uint8_t** rom, int *size);

#endif // PATCH_H
