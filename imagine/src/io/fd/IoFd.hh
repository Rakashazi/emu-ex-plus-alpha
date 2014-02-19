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

#pragma once

#include <engine-globals.h>

#include <mem/interface.h>
#include <io/Io.hh>

class IoFd : public Io
{
public:
	static Io* open(const char * location, uint mode = 0, CallResult *errorOut = 0);
	static Io* create(const char * location, uint mode = 0, CallResult *errorOut = 0);
	~IoFd() { close(); }

	ssize_t readUpTo(void *buffer, size_t numBytes) override;
	size_t fwrite(const void *buffer, size_t size, size_t nmemb) override;
	CallResult tell(ulong &offset) override;
	CallResult seek(long offset, uint mode) override;
	void truncate(ulong offset) override;
	void close() override;
	ulong size() override;
	void sync() override;
	int eof() override;
	void advise(long offset, size_t len, Advice advice) override;

	static CallResult writeToNewFile(const char *path, void *data, size_t size)
	{
		CallResult ret = OK;
		Io *f = create(path, 0, &ret);
		if(!f)
		{
			return ret;
		}
		if(f->fwrite(data, size, 1) != 1)
			ret = IO_ERROR;
		delete f;
		return ret;
	}

	static size_t readFromFile(const char *path, void *data, size_t size)
	{
		Io *f = open(path);
		if(!f)
		{
			return 0;
		}
		size_t readSize = f->readUpTo(data, size);
		logMsg("read %d bytes from %s", (int)readSize, path);
		delete f;
		return readSize;
	}

private:
	int fd = -1;
};
