/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "MDFN"
#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/state-driver.h>
#include <mednafen/movie.h>
#include <mednafen/cputest/cputest.h>
#include <imagine/logger/logger.h>
#include <imagine/config/defs.hh>

namespace Mednafen
{

MDFNGI *MDFNGameInfo{};
int MDFNnetplay{};

void MDFN_DoSimpleCommand(int cmd)
{
	MDFNGameInfo->DoSimpleCommand(cmd);
}

void MDFN_printf(const char *format, ...) noexcept
{
	if(!Config::DEBUG_BUILD || !logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_M, format, args);
	va_end( args );
}

void MDFN_Notify(MDFN_NoticeType t, const char* format, ...) noexcept
{
	if(!Config::DEBUG_BUILD || !logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_E, format, args);
	va_end( args );
}

void MDFND_OutputNotice(MDFN_NoticeType t, const char* s) noexcept
{
	if(!Config::DEBUG_BUILD || !logger_isEnabled())
		return;
	logMsg("%s", s);
}

void MDFN_indent(int indent) {}
void MDFND_SetMovieStatus(StateStatusStruct *status) noexcept {}
void MDFND_SetStateStatus(StateStatusStruct *status) noexcept {}
void NetplaySendState(void) {}
void MDFND_NetplayText(const char *text, bool NetEcho) {}

[[gnu::weak]] void MDFN_MediaStateAction(StateMem *sm, const unsigned load, const bool data_only) {}

void MDFN_StateAction(StateMem *sm, const unsigned load, const bool data_only)
{
	MDFN_MediaStateAction(sm, load, data_only);
	SFORMAT StateRegs[]{SFEND};
	MDFNSS_StateAction(sm, load, data_only, StateRegs, "MDFNRINP", true);

	if(data_only)
		MDFNMOV_StateAction(sm, load);

	MDFNGameInfo->StateAction(sm, load, data_only);
}

void MDFN_MidLineUpdate(EmulateSpecStruct *espec, int y) {}

void Player_Init(int tsongs, const std::string &album, const std::string &artist, const std::string &copyright, const std::vector<std::string> &snames = std::vector<std::string>(), bool override_gi = true) {}
void Player_Draw(MDFN_Surface *surface, MDFN_Rect *dr, int CurrentSong, int16 *samples, int32 sampcount) {}

namespace Time
{

int64 EpochTime(void)
{
 time_t ret = time(nullptr);
 return (int64)ret;
}

struct tm LocalTime(const int64 ept)
{
 struct tm tout;
 time_t tt = (time_t)ept;
 if(!localtime_r(&tt, &tout))
 {
  ErrnoHolder ene(errno);
  throw MDFN_Error(ene.Errno(), _("%s failed: %s"), "localetime_r()", ene.StrError());
 }
 return tout;
}

}

}

#if defined(__i386__) || defined(__x86_64__)
int ff_get_cpu_flags_x86(void)
{
	int flags = CPUTEST_FLAG_CMOV | CPUTEST_FLAG_MMX | CPUTEST_FLAG_SSE | CPUTEST_FLAG_SSE2;
	return flags;
}
#endif
