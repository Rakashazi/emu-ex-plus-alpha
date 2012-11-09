#pragma once

#include <engine-globals.h>
#include <util/bits.h>

typedef int(*FsDirFilterFunc)(const char *name, int type);
typedef int(*FsDirSortFunc)(const char *name1, long int mtime1, const char *name2, long int mtime2);

class Fs
{
public:
	constexpr Fs() { }
	virtual uint numEntries() const = 0;
	virtual const char *entryFilename(uint index) const = 0;
	virtual void closeDir() = 0;

	static constexpr uint OPEN_UNSORT = BIT(0);
	enum { TYPE_NONE = 0, TYPE_FILE, TYPE_DIR };

	static int sortMTime(const char *name1, long int mtime1, const char *name2, long int mtime2)
	{
		if(mtime1 < mtime2)
			return 1;
		else if(mtime1 > mtime2)
			return -1;
		else
			return 0;
	}
};
