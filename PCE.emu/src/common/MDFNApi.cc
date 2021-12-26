#define LOGTAG "main"
#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/state-driver.h>
#include <mednafen/movie.h>
#include <mednafen/cputest/cputest.h>
#include <imagine/logger/logger.h>
#include <imagine/config/defs.hh>

namespace Mednafen
{

int MDFNnetplay{};

#ifndef NDEBUG
void MDFN_printf(const char *format, ...) noexcept
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_M, format, args);
	va_end( args );
}

void MDFN_Notify(MDFN_NoticeType t, const char* format, ...) noexcept
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_E, format, args);
	va_end( args );
}
#endif

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

void MDFN_StateAction(StateMem *sm, const unsigned load, const bool data_only)
{
	SFORMAT StateRegs[]{SFEND};
	MDFNSS_StateAction(sm, load, data_only, StateRegs, "MDFNRINP", true);

	if(data_only)
		MDFNMOV_StateAction(sm, load);

	MDFNGameInfo->StateAction(sm, load, data_only);
}

void MDFN_MidLineUpdate(EmulateSpecStruct *espec, int y) {}

namespace Time
{

int64 EpochTime(void)
{
 time_t ret = time(nullptr);
 return (int64)ret;
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
