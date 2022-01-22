#ifndef PATCH_H
#define PATCH_H

#include "../Util.h"
#include "Types.h"

namespace IG
{
class ApplicationContext;
}

bool applyPatch(IG::ApplicationContext ctx, const char *patchname, uint8_t** rom, int *size);
bool patchApplyIPS(IG::ApplicationContext ctx, const char *patchname, uint8_t** rom, int *size);
bool patchApplyUPS(IG::ApplicationContext ctx, const char *patchname, uint8_t** rom, int *size);
bool patchApplyPPF(IG::ApplicationContext ctx, const char *patchname, uint8_t** rom, int *size);

#endif // PATCH_H
