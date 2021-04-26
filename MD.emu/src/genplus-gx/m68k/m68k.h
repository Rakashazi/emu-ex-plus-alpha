#pragma once

// TODO: add Cyclone API
/*#if defined(__arm__) && !defined(__APPLE__)
	#include "cyclone/m68k.h"
#else*/
	#include "musashi/m68k.h"
//#endif

const char *m68KAddrToStr(M68KCPU &cpu, unsigned addr);
