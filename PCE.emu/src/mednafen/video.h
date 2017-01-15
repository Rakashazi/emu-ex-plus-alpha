#ifndef __MDFN_VIDEO_H
#define __MDFN_VIDEO_H

#include "video/surface.h"
#include "video/primitives.h"
#include "video/text.h"

void MDFN_ResetMessages(void);
void MDFN_InitFontData(void);
void MDFN_DispMessage(const char* format, ...) noexcept MDFN_FORMATSTR(gnu_printf, 1, 2);

#endif
