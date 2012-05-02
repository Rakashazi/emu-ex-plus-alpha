#pragma once

#include "sys.hh"
#include <input/interface.h>
#include <util/collection/DLList.hh>

struct IControlPad : public BluetoothInputDevice
{
public:
	constexpr IControlPad(): inputBuffer{0}, inputBufferPos(0), player(0),
		function(FUNC_NONE), prevBtnData{0}, nubBtn{0}
		{ }

	enum
	{
		FUNC_NONE,
		FUNC_SET_LED_MODE,
		FUNC_GP_REPORTS,
	};

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

	static StaticDLList<IControlPad*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys sock;
	uchar inputBuffer[6];
	uint inputBufferPos;
	uint player;
	int function;
	uchar prevBtnData[2];
	bool nubBtn[8];
	static const int nubDeadzone = 64;

	static uint findFreeDevId();
	void processBtnReport(const uchar *btnData, uint player/*, ThreadPThread *thread = nullptr*/);
	void processNubDataForButtonEmulation(const schar *nubData, uint player/*, ThreadPThread *thread = nullptr*/);
};
