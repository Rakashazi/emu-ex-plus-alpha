#include <util/builtins.h>
#include <config/imagineTypes.h>
#include <logger/interface.h>
#include <assert.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <dlfcn.h>

typedef int (*hci_inquiryProto)(int dev_id, int len, int num_rsp, const uint8_t *lap, inquiry_info **ii, long flags);
typedef int (*hci_close_devProto)(int dd);
typedef int (*hci_open_devProto)(int dev_id);
typedef int (*hci_read_remote_nameProto)(int dd, const bdaddr_t *bdaddr, int len, char *name, int to);
typedef int (*hci_get_routeProto)(bdaddr_t *bdaddr);
typedef int (*hci_devidProto)(const char *str);

static hci_inquiryProto hci_inquirySym = 0;
static hci_close_devProto hci_close_devSym = 0;
static hci_open_devProto hci_open_devSym = 0;
static hci_read_remote_nameProto hci_read_remote_nameSym = 0;
static hci_get_routeProto hci_get_routeSym = 0;

CallResult bluez_dl()
{
	if(hci_inquirySym)
		return OK;
	void *libbluetooth = dlopen("/system/lib/libbluetooth.so", RTLD_LOCAL | RTLD_LAZY);
	if(!libbluetooth)
	{
		logErr("libbluetooth not found");
		return INVALID_PARAMETER;
	}
	logMsg("libbluetooth present");
	hci_inquirySym = (hci_inquiryProto)dlsym(libbluetooth, "hci_inquiry");
	hci_close_devSym = (hci_close_devProto)dlsym(libbluetooth, "hci_close_dev");
	hci_open_devSym = (hci_open_devProto)dlsym(libbluetooth, "hci_open_dev");
	hci_read_remote_nameSym = (hci_read_remote_nameProto)dlsym(libbluetooth, "hci_read_remote_name");
	hci_get_routeSym = (hci_get_routeProto)dlsym(libbluetooth, "hci_get_route");
	if(!hci_inquirySym || !hci_close_devSym || !hci_open_devSym || !hci_read_remote_nameSym || !hci_get_routeSym)
	{
		logErr("missing bluetooth functions");
		dlclose(libbluetooth);
		hci_inquirySym = 0;
		return INVALID_PARAMETER;
	}
	logMsg("all symbols loaded");
	return OK;
}

int hci_inquiry(int dev_id, int len, int num_rsp, const uint8_t *lap, inquiry_info **ii, long flags)
{
	assert(hci_inquirySym);
	return hci_inquirySym(dev_id, len, num_rsp, lap, ii, flags);
}

int hci_close_dev(int dd)
{
	assert(hci_close_devSym);
	return hci_close_devSym(dd);
}

int hci_open_dev(int dev_id)
{
	assert(hci_open_devSym);
	return hci_open_devSym(dev_id);
}

int hci_read_remote_name(int dd, const bdaddr_t *bdaddr, int len, char *name, int to)
{
	assert(hci_read_remote_nameSym);
	return hci_read_remote_nameSym(dd, bdaddr, len, name, to);
}

int hci_get_route(bdaddr_t *bdaddr)
{
	assert(hci_get_routeSym);
	return hci_get_routeSym(bdaddr);
}
