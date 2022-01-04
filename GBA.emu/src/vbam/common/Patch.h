#ifndef PATCH_H
#define PATCH_H

#include "Types.h"

namespace IG
{
class ApplicationContext;
}

bool applyPatch(IG::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyIPS(IG::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyUPS(IG::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyPPF(IG::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);

#endif // PATCH_H
