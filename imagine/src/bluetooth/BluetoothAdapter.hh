#pragma once

#include <engine-globals.h>
#include <util/Delegate.hh>
#include <util/operators.hh>

struct BluetoothAddr : public NotEquals<BluetoothAddr>
{
	constexpr BluetoothAddr() { }
	uchar b[6] = {0};

	bool operator ==(BluetoothAddr const& rhs) const
	{
		return memcmp(b, rhs.b, sizeof(b)) == 0;
	}

} __attribute__((packed));

class BluetoothAdapter
{
public:
	constexpr BluetoothAdapter() { }
	static BluetoothAdapter *defaultAdapter();
	virtual bool startScan() = 0;
	virtual void cancelScan() = 0;
	static void setScanCacheUsage(bool on) { useScanCache = on; }
	static bool scanCacheUsage() { return useScanCache; }
	virtual void close() = 0;
	virtual void constructSocket(void *mem);

	enum { INIT_FAILED, SCAN_FAILED, SCAN_PROCESSING, SCAN_NO_DEVS, SCAN_NAME_FAILED,
		SCAN_COMPLETE, SCAN_CANCELLED, SOCKET_OPEN_FAILED };
	typedef Delegate<void (uint statusCode, int arg)> OnStatusDelegate;
	OnStatusDelegate &statusDelegate() { return onStatus; }

	typedef Delegate<bool (const uchar devClass[3])> OnScanDeviceClassDelegate;
	OnScanDeviceClassDelegate &scanDeviceClassDelegate() { return onScanDeviceClass; }

	typedef Delegate<void (const char *name, BluetoothAddr addr)> OnScanDeviceNameDelegate;
	OnScanDeviceNameDelegate &scanDeviceNameDelegate() { return onScanDeviceName; }

	bool inDetect = 0;
	static bool useScanCache;
protected:
	OnStatusDelegate onStatus;
	OnScanDeviceClassDelegate onScanDeviceClass;
	OnScanDeviceNameDelegate onScanDeviceName;
};

class BluetoothSocket
{
public:
	constexpr BluetoothSocket() { }
	virtual CallResult openL2cap(BluetoothAddr addr, uint psm) = 0;
	virtual CallResult openRfcomm(BluetoothAddr addr, uint channel) = 0;
	virtual void close() = 0;
	virtual CallResult write(const void *data, size_t size) = 0;
	typedef Delegate<bool (const uchar *data, size_t size)> OnDataDelegate;
	OnDataDelegate &onDataDelegate() { return onDataEvent; }
	typedef Delegate<uint (BluetoothSocket &sock, uint status)> OnStatusDelegate;
	OnStatusDelegate &onStatusDelegate() { return onStatus; }
	enum { STATUS_CONNECTING, STATUS_OPENED, STATUS_ERROR, STATUS_CLOSED };
	enum { REPLY_OPENED_NONE = 0, REPLY_OPENED_USE_READ_EVENTS };
protected:
	OnDataDelegate onDataEvent;
	OnStatusDelegate onStatus;
};

class BluetoothInputDevice
{
public:
	virtual CallResult open(BluetoothAdapter &adapter) = 0;
	virtual ~BluetoothInputDevice() { }
	virtual void removeFromSystem() = 0;
};
