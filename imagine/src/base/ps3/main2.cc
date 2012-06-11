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

#define thisModuleName "base:ps3"

// This file is compiled with lv2 GCC 4.1 so the Sony startup code
// can call main() and cellSysutilRegisterCallback works

#include <sys/process.h>
#include <sysutil/sysutil_sysparam.h>
#include <util/ansiTypes.h>
#include <util/branch.h>

namespace Base
{
	void exitVal(int returnVal);
	void onFocusChange(uint in);

	void sysutilCallback(uint64_t status, uint64_t param, void* userdata)
	{
		switch(status)
		{
			bcase CELL_SYSUTIL_REQUEST_EXITGAME:
				exitVal(0);
			bcase CELL_SYSUTIL_DRAWING_BEGIN:
				onFocusChange(0);
			bcase CELL_SYSUTIL_DRAWING_END:
				onFocusChange(1);
		}
	}

	int main2();
}

SYS_PROCESS_PARAM(1001, 0x100000); // using 1Meg stack just in case for now

int main()
{
	cellSysutilRegisterCallback(0, &Base::sysutilCallback, 0);
	return Base::main2();
}
