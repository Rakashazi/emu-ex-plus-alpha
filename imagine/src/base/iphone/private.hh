#pragma once

#include <util/ansiTypes.h>
#include <CoreGraphics/CGColorSpace.h>

namespace Base
{

void nsLog(const char* str);
void nsLogv(const char* format, va_list arg);

extern CGColorSpaceRef grayColorSpace, rgbColorSpace;

}

namespace Audio
{
	void initSession();
}
