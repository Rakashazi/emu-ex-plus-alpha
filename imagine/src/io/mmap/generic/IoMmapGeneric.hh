#pragma once

#include <engine-globals.h>
#include <io/mmap/IoMmap.hh>
#include <mem/interface.h>

class IoMmapGeneric : public IoMmap
{
public:
	static Io* open(const uchar * buffer, size_t size);
	~IoMmapGeneric() { close(); }
	void close();

	// optional function to call on close, <ptr> is the buffer passed during open()
	typedef void (*FreeFunc)(void *ptr);
	void memFreeFunc(FreeFunc free);

private:
	FreeFunc free = nullptr;
};
