#pragma once

#include <fs/Fs.hh>

#ifdef CONFIG_FS_POSIX
	#include <fs/posix/FsPosix.hh>
	#define FsSys FsPosix
#endif

#ifdef CONFIG_FS_PS3
	#include <fs/ps3/FsPs3.hh>
	#define FsSys FsPs3
#endif

#ifdef CONFIG_FS
template <uint SIZE>
class WorkDirStack
{
	FsSys::cPath dir[SIZE] { {0} };
public:
	uint size = 0;
	constexpr WorkDirStack() { }

	void push()
	{
		assert(size < SIZE);
		string_copy(dir[size], FsSys::workDir());
		logMsg("pushed work dir %s", dir[size]);
		size++;
	}

	void pop()
	{
		if(!size)
		{
			logWarn("no work dir in stack");
			return;
		}
		size--;
		logMsg("popped work dir %s", dir[size]);
		FsSys::chdir(dir[size]);
	}
};
#endif
