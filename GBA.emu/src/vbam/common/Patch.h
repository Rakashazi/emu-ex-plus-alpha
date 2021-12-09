#ifndef PATCH_H
#define PATCH_H

#include "Types.h"

namespace Base
{
class ApplicationContext;
}

bool applyPatch(Base::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyIPS(Base::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyUPS(Base::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);
bool patchApplyPPF(Base::ApplicationContext ctx, const char *patchname, u8 **rom, int *size);

#endif // PATCH_H
