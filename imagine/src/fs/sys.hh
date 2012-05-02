#pragma once

#include <fs/Fs.hh>

#ifdef CONFIG_FS_POSIX
	#include <fs/posix/FsPosix.h>
	#define FsSys FsPosix
#endif

#ifdef CONFIG_FS_PS3
	#include <fs/ps3/FsPs3.hh>
	#define FsSys FsPs3
#endif
