#pragma once

#include <util/ansiTypes.h>

namespace Base
{

void nsLog(const char* str);
void nsLogv(const char* format, va_list arg);

}

namespace Audio
{
	void initSession();
}
