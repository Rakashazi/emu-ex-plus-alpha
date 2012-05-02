#pragma once

#include "sys.hh"
#include <input/interface.h>
#include <util/collection/DLList.hh>

struct Zeemote : public BluetoothInputDevice
{
public:
	constexpr Zeemote(): inputBuffer{0}, inputBufferPos(0), packetSize(0),
	prevBtnPush{0}, stickBtn{0}, player(0)
	{ }

	CallResult open(BluetoothAddr addr);

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
	uchar inputBuffer[46];
	uint inputBufferPos;
	uint packetSize;
	bool prevBtnPush[4], stickBtn[4];
	uint player;

	static const uint RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static uint findFreeDevId();
	static const char *reportIDToStr(uint id);
	void processBtnReport(const uchar *btnData, uint player/*, ThreadPThread *thread = nullptr*/);
	void processStickDataForButtonEmulation(const schar *pos, int player/*, ThreadPThread *thread = nullptr*/);
};
