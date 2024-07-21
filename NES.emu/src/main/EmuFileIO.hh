#pragma once

/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <fceu/emufile.h>
#include <imagine/io/MapIO.hh>

namespace EmuEx
{

class EmuFileIO final : public EMUFILE
{
public:
	IG::MapIO io;

	EmuFileIO(IG::IO &);
	EmuFileIO(IG::MapIO);
	~EmuFileIO() = default;
	FILE *get_fp() final { return nullptr; }
	EMUFILE* memwrap() final { return nullptr; }
	void truncate(size_t) final {}
	int fprintf(const char*, ...) final { return 0; };
	int fgetc() final;
	int fputc(int c) final;
	size_t _fread(const void *ptr, size_t bytes) final;
	void fwrite(const void *ptr, size_t bytes) final;
	int fseek(long int offset, int origin) final;
	long int ftell() final;
	size_t size() final { return io.size(); }
	void fflush() final {}
};

}
