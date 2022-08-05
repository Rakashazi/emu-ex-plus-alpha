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

#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include <assert.h>
#include <dlfcn.h>
#include <imagine/base/android/libhardware.h>

typedef int (*hw_get_moduleProto)(const char *id, const struct hw_module_t **module);

static hw_get_moduleProto hw_get_moduleSym = 0;

bool libhardware_dl()
{
	if(hw_get_moduleSym)
		return true;
	void *libhardware = dlopen("libhardware.so", RTLD_LAZY);
	if(!libhardware)
	{
		logErr("libhardware not found");
		return false;
	}
	hw_get_moduleSym = (hw_get_moduleProto)dlsym(libhardware, "hw_get_module");
	if(!hw_get_moduleSym)
	{
		logErr("missing libhardware functions");
		dlclose(libhardware);
		hw_get_moduleSym = 0;
		return false;
	}
	logMsg("libhardware symbols loaded");
	return true;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
	assert(hw_get_moduleSym);
	return hw_get_moduleSym(id, module);
}
