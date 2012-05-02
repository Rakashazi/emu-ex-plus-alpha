#pragma once

#include <engine-globals.h>

// TODO: reimplement/replace base class

class Resource
{
public:
	static void classInit();
	void init();
	CallResult initWithName(const char * name);
	static Resource * findExisting(const char * name);
	void release();
	static Resource * loadWithPath(char * name);
	virtual void free() = 0;

#ifdef NDEBUG
private:
#endif

	const char *name;
	int refCount;
};

