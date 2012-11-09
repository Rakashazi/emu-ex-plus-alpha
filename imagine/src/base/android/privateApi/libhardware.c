#include <util/builtins.h>
#include <config/imagineTypes.h>
#include <logger/interface.h>
#include <assert.h>
#include <dlfcn.h>
#include "libhardware.h"

typedef int (*hw_get_moduleProto)(const char *id, const struct hw_module_t **module);

static hw_get_moduleProto hw_get_moduleSym = 0;

CallResult libhardware_dl()
{
	if(hw_get_moduleSym)
		return OK;
	void *libhardware = dlopen("/system/lib/libhardware.so", RTLD_LOCAL | RTLD_LAZY);
	if(!libhardware)
	{
		logErr("libhardware not found");
		return INVALID_PARAMETER;
	}
	hw_get_moduleSym = (hw_get_moduleProto)dlsym(libhardware, "hw_get_module");
	if(!hw_get_moduleSym)
	{
		logErr("missing libhardware functions");
		dlclose(libhardware);
		hw_get_moduleSym = 0;
		return INVALID_PARAMETER;
	}
	logMsg("libhardware symbols loaded");
	return OK;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
	assert(hw_get_moduleSym);
	return hw_get_moduleSym(id, module);
}
