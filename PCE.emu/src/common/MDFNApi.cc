#define LOGTAG "main"
#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/state-driver.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/thread/pthread.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/fs/sys.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/strings.h>

int get_line(IO &file, std::string &str)
{
 uint8 c;

 str.clear();	// or str.resize(0)??

 while(file.readAll(&c, sizeof(c)) == OK)
 {
  if(c == '\r' || c == '\n' || c == 0)
   return(c);

  str.push_back(c);
 }

 return(str.length() ? 256 : -1);
}

#ifndef NDEBUG
void MDFN_printf(const char *format, ...) throw()
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_M, format, args);
	va_end( args );
}

void MDFN_PrintError(const char *format, ...) throw()
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_E, format, args);
	va_end( args );
}

void MDFN_DispMessage(const char *format, ...) throw()
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, format );
	logger_vprintf(LOG_M, format, args);
	va_end( args );
}
#endif

int MDFN_SavePNGSnapshot(const char *fname, const MDFN_Surface *src, const MDFN_Rect *rect, const MDFN_Rect *LineWidths)
{
	return 1;
}

void MDFN_ResetMessages(void) { }

struct MDFN_Thread : public ThreadPThread { };

MDFN_Thread *MDFND_CreateThread(void* (*fn)(void *), void *data)
{
	MDFN_Thread *thread = new MDFN_Thread();

	if(!thread || !thread->create(0, fn, data))
		goto FAIL;

	return thread;

	FAIL:
	if(thread)
	{
		delete thread;
	}
	return 0;
}

void MDFND_WaitThread(MDFN_Thread *thread, int *status)
{
	thread->join();
	delete thread;
}

struct MDFN_Mutex : public MutexPThread { };

MDFN_Mutex *MDFND_CreateMutex(void)
{
	MDFN_Mutex *mutex = new MDFN_Mutex();
	if(!mutex || !mutex->create())
		goto FAIL;

	return mutex;

	FAIL:
	if(mutex)
	{
		mutex->destroy();
		delete mutex;
	}
	return 0;
}

void MDFND_DestroyMutex(MDFN_Mutex *mutex)
{
	mutex->destroy();
	delete mutex;
}

int MDFND_LockMutex(MDFN_Mutex *mutex)
{
	//logMsg("lock %p", mutex);
	if(!mutex->lock())
	{
		return -1;
	}
	return 0;
}

int MDFND_UnlockMutex(MDFN_Mutex *mutex)
{
	//logMsg("unlock %p", mutex);
	if(!mutex->unlock())
	{
		return -1;
	}
	return 0;
}

struct MDFN_Cond : public CondVarPThread {};

MDFN_Cond* MDFND_CreateCond(void)
{
	MDFN_Cond *cond = new MDFN_Cond();
	if(!cond)
		return nullptr;
	if(!cond->init())
	{
		delete cond;
		return nullptr;
	}
	return cond;
}

void MDFND_DestroyCond(MDFN_Cond* cond)
{
	cond->deinit();
	delete cond;
}

int MDFND_SignalCond(MDFN_Cond* cond)
{
	cond->signal();
	return 0;
}

int MDFND_WaitCond(MDFN_Cond* cond, MDFN_Mutex* mutex)
{
	cond->wait(*mutex);
	return 0;
}

#ifdef CONFIG_BASE_IOS

// Mach semaphores
#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/mach.h>

struct MDFN_Semaphore
{
	semaphore_t s;
};

MDFN_Semaphore *MDFND_CreateSemaphore(void)
{
	MDFN_Semaphore *semaphore = new MDFN_Semaphore();
	if(!semaphore || semaphore_create(mach_task_self(), &semaphore->s, SYNC_POLICY_FIFO, 0) != KERN_SUCCESS)
	{
		delete semaphore;
		return 0;
	}
	return semaphore;
}

void MDFND_DestroySemaphore(MDFN_Semaphore *semaphore)
{
	semaphore_destroy(mach_task_self(), semaphore->s);
	delete semaphore;
}

int MDFND_SignalSemaphore(MDFN_Semaphore *semaphore)
{
	return semaphore_signal(semaphore->s) == KERN_SUCCESS ? 0 : -1;
}

int MDFND_WaitSemaphore(MDFN_Semaphore *semaphore)
{
	return semaphore_wait(semaphore->s) == KERN_SUCCESS ? 0 : -1;
}

#else

// Posix semaphores
#include <semaphore.h>

struct MDFN_Semaphore
{
	sem_t s;
};

MDFN_Semaphore *MDFND_CreateSemaphore(void)
{
	MDFN_Semaphore *semaphore = new MDFN_Semaphore();
	if(!semaphore || sem_init(&semaphore->s, 0, 0) == -1)
	{
		delete semaphore;
		return 0;
	}
	return semaphore;
}

void MDFND_DestroySemaphore(MDFN_Semaphore *semaphore)
{
	sem_destroy(&semaphore->s);
	delete semaphore;
}

int MDFND_SignalSemaphore(MDFN_Semaphore *semaphore)
{
	return sem_post(&semaphore->s);
}

int MDFND_WaitSemaphore(MDFN_Semaphore *semaphore)
{
	return sem_wait(&semaphore->s);
}

#endif

void GetFileBase(const char *f) { }

void MDFN_indent(int indent) { }
int MDFN_RawInputStateAction(StateMem *sm, int load, int data_only) { return 1; }
void MDFND_SetMovieStatus(StateStatusStruct *status) { }
void MDFND_SetStateStatus(StateStatusStruct *status) { }
void NetplaySendState(void) { }
void MDFND_NetplayText(const uint8 *text, bool NetEcho) { }

uint32 MDFND_GetTime(void)
{
	auto time = TimeSys::now();
	return (uint32)(time.divByUSecs(1000));
}

int Player_Init(int tsongs, UTF8 *album, UTF8 *artist, UTF8 *copyright, UTF8 **snames) { return 1; }
void Player_Draw(const MDFN_Surface *surface, MDFN_Rect *dr, int CurrentSong, int16 *samples, int32 sampcount) { }

int MDFNnetplay=0;

/*void ErrnoHolder::SetErrno(int the_errno)
{
	logMsg("set errno %d", the_errno);
}*/

/*CLINK char *trio_vaprintf (const char *format, va_list args)
{
	return 0; //TODO
}

CLINK int trio_snprintf (char *buffer, size_t max, const char *format, ...)
{
	va_list args;
	va_start( args, format );
	int ret = vsnprintf(buffer, max, format, args);
	va_end( args );
	return ret;
}

CLINK int trio_fprintf (FILE *file, const char *format, ...)
{
	va_list args;
	va_start( args, format );
	int ret = vfprintf(file, format, args);
	va_end( args );
	return ret;
}

CLINK int trio_sscanf (const char *buffer, const char *format, ...)
{
	va_list args;
	va_start( args, format );
	int ret = vsscanf(buffer, format, args);
	va_end( args );
	return ret;
}

CLINK int trio_fscanf (FILE * stream, const char *format, ...)
{
	va_list args;
	va_start( args, format );
	int ret = vfscanf(stream, format, args);
	va_end( args );
	return ret;
}*/
