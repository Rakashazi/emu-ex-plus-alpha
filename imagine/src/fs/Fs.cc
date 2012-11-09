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

#define thisModuleName "fs"
#if defined (CONFIG_BASE_WIN32)
	#include <direct.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#include <fs/Fs.hh>
#include <logger/interface.h>
#include <util/strings.h>
#include <base/Base.hh>
#include "sys.hh"
