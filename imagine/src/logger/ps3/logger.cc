/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "logger:ps3"
#include <engine-globals.h>
#include <gfx/viewport.hh>
#include <logger/interface.h>

#include <cell/dbgfont.h>
#include <stdarg.h>
#include <stdio.h>

static CellDbgFontConsoleId cID = -1;
uint loggerVerbosity = loggerMaxVerbosity;

CallResult logger_ps3_init(uint w, uint h)
{
	CellDbgFontConfig cfg;
	cfg.bufSize      = 4096;
	cfg.screenWidth  = w;
	cfg.screenHeight = h;
	cellDbgFontInit(&cfg);

	CellDbgFontConsoleConfig ccfg0;
	ccfg0.posLeft     = 0.01f;
	ccfg0.posTop      = 0.01f;
	ccfg0.cnsWidth    = 128;
	ccfg0.cnsHeight   = 32;
	ccfg0.scale       = 0.75f;
	ccfg0.color       = 0xff0080ff;  // ABGR orange
	cID = cellDbgFontConsoleOpen(&ccfg0);

	//logMsg("init logger");
	return OK;
}

void logger_update()
{
	cellDbgFontDraw();
}

void logger_vprintf(LoggerSeverity severity, const char* msg, va_list args)
{
	cellDbgFontConsoleVprintf(cID, msg, args);
}

void logger_printf(LoggerSeverity severity, const char* msg, ...)
{
	if(severity > loggerVerbosity) return;
	va_list args;
	va_start(args, msg);

	cellDbgFontConsoleVprintf(cID, msg, args );

	va_end(args);
}

#undef thisModuleName
