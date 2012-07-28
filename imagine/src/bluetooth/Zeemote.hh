#pragma once

#include "sys.hh"
#include <input/interface.h>
#include <util/collection/DLList.hh>

struct Zeemote : public BluetoothInputDevice
{
public:
	//constexpr Zeemote() { }
	CallResult open(BluetoothAddr addr, BluetoothAdapter &adapter);

	void close();

	void removeFromSystem();

	uint statusHandler(BluetoothSocket &sock, uint status);
	bool dataHandler(const uchar *packet, size_t size);

	static const uchar btClass[3];

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3);
	}

	static StaticDLList<Zeemote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys sock;
	uchar inputBuffer[46] = {0};
	uint inputBufferPos = 0;
	uint packetSize = 0;
	bool prevBtnPush[4] = {0}, stickBtn[4] = {0};
	uint player = 0;

	static const uint RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static uint findFreeDevId();
	static const char *reportIDToStr(uint id);
	void processBtnReport(const uchar *btnData, uint player);
	void processStickDataForButtonEmulation(const schar *pos, int player);
};
