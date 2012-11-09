#pragma once

#include <bluetooth/sys.hh>
#include <input/Input.hh>
#include <util/collection/DLList.hh>

struct IControlPad : public BluetoothInputDevice
{
public:
	IControlPad(BluetoothAddr addr): addr(addr) { }
	enum
	{
		FUNC_NONE,
		FUNC_SET_LED_MODE,
		FUNC_GP_REPORTS,
	};

	CallResult open(BluetoothAdapter &adapter) override;
	void close();
	void removeFromSystem() override;

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
	uchar inputBuffer[6] = {0};
	uint inputBufferPos = 0;
	uint player = 0;
	int function = 0;
	uchar prevBtnData[2] = {0};
	bool nubBtn[8] = {0};
	static const int nubDeadzone = 64;
	BluetoothAddr addr;

	static uint findFreeDevId();
	void processBtnReport(const uchar *btnData, uint player);
	void processNubDataForButtonEmulation(const schar *nubData, uint player);
};
