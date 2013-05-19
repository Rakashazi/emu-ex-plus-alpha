#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		return 1; // no path argument
	}
	const char *path = argv[1];
	char realPath[PATH_MAX];
	if(!realpath(path, realPath))
	{
		return 2; // path invalid or too long
	}
	if(!strstr(realPath, "/private/var/mobile/Media") && !strstr(realPath, "/private/var/mobile/Documents"))
	{
		return 3; // path isn't part of mobile's files
	}
	uid_t realUID = getuid();
	if(seteuid(geteuid()) == -1)
	{
		// can't set effective UID, but still try to change owner
	}
	if(chown(realPath, realUID, realUID) == -1)
	{
		return 4; // can't change owner
	}
	return 0;
}

