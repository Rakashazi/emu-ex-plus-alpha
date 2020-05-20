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
#include <imagine/io/BufferMapIO.hh>

class EmuFileIO : public EMUFILE {
protected:
	BufferMapIO io{};

public:

	EmuFileIO(IO &io);

	~EmuFileIO() {
	}

	FILE *get_fp() {
		return nullptr;
	}

	EMUFILE* memwrap() { return nullptr; }

	//bool is_open() { return io; }

	void truncate(s32 length);

	int fprintf(const char *format, ...) {
		return 0;
	};

	int fgetc();

	int fputc(int c) {
		return 0;
	}

	size_t _fread(const void *ptr, size_t bytes);

	//removing these return values for now so we can find any code that might be using them and make sure
	//they handle the return values correctly

	void fwrite(const void *ptr, size_t bytes){
		failbit = true;
	}

	int fseek(int offset, int origin);

	int ftell();

	int size();

	void fflush() {
	}

};
