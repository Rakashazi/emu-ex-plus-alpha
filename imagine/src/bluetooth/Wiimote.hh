#pragma once

#include "sys.hh"
#include <input/interface.h>
#include <util/collection/DLList.hh>

class Wiimote : public BluetoothInputDevice
{
public:
	CallResult open(const char *name, BluetoothAddr addr, BluetoothAdapter &adapter);
	void close();
	void removeFromSystem();
	bool dataHandler(const uchar *data, size_t size);
	uint statusHandler(BluetoothSocket &sock, uint status);
	void requestStatus();
	void setLEDs(uint player);
	void sendDataMode(uchar mode);
	void writeReg(uchar offset, uchar val);
	void readReg(uint offset, uchar size);

	static const uchar btClass[3], btClassDevOnly[3], btClassRemotePlus[3];

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3)
			|| mem_equal(devClass, btClassDevOnly, 3)
			|| mem_equal(devClass, btClassRemotePlus, 3);
	}

	static StaticDLList<Wiimote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys ctlSock, intSock;
	int extension = EXT_NONE;
	uint player = 0;
	uint function = FUNC_NONE;
	fbool stickBtn[8] = {0};
	uchar prevBtnData[2] = {0};
	uchar prevExtData[6] = {0};
	BluetoothAddr addr;

	enum
	{
		FUNC_NONE,
		FUNC_INIT_EXT, // extension init part 1
		FUNC_INIT_EXT_DONE, // extension init part 2
		FUNC_GET_EXT_TYPE, // return the extension type register
	};

	enum
	{
		EXT_NONE, EXT_CC, EXT_NUNCHUK, EXT_UNKNOWN
	};

	static uint findFreeDevId();
	void initExtension();
	void initExtensionPart2();
	static uchar playerLEDs(int player);
	void sendDataModeByExtension();
	static void decodeCCSticks(const uchar *ccSticks, int &lX, int &lY, int &rX, int &rY);
	void processStickDataForButtonEmulation(int player, const uchar *data);
	void processCoreButtons(const uchar *packet, uint player);
	void processClassicButtons(const uchar *packet, uint player);
	void processNunchukStickDataForButtonEmulation(int player, const uchar *data);
	void processNunchukButtons(const uchar *packet, uint player);
};

static const PackedInputAccess wiimoteDataAccess[] =
{
	{ 0, BIT(0), Input::Wiimote::DOWN }, // map to sideways control
	{ 0, BIT(1), Input::Wiimote::UP },
	{ 0, BIT(2), Input::Wiimote::RIGHT },
	{ 0, BIT(3), Input::Wiimote::LEFT },
	{ 0, BIT(4), Input::Wiimote::PLUS },

	{ 1, BIT(0), Input::Wiimote::_2 },
	{ 1, BIT(1), Input::Wiimote::_1 },
	{ 1, BIT(2), Input::Wiimote::B },
	{ 1, BIT(3), Input::Wiimote::A },
	{ 1, BIT(4), Input::Wiimote::MINUS },
	{ 1, BIT(7), Input::Wiimote::HOME },
};

static const PackedInputAccess wiimoteCCDataAccess[] =
{
	{ 4, BIT(7), Input::Wiimote::RIGHT },
	{ 4, BIT(6), Input::Wiimote::DOWN },
	{ 4, BIT(5), Input::Wiimote::L },
	{ 4, BIT(4), Input::Wiimote::MINUS },
	{ 4, BIT(3), Input::Wiimote::HOME },
	{ 4, BIT(2), Input::Wiimote::PLUS },
	{ 4, BIT(1), Input::Wiimote::R },

	{ 5, BIT(7), Input::Wiimote::ZL },
	{ 5, BIT(6), Input::Wiimote::B },
	{ 5, BIT(5), Input::Wiimote::Y },
	{ 5, BIT(4), Input::Wiimote::A },
	{ 5, BIT(3), Input::Wiimote::X },
	{ 5, BIT(2), Input::Wiimote::ZR },
	{ 5, BIT(1), Input::Wiimote::LEFT },
	{ 5, BIT(0), Input::Wiimote::UP },
};

static const PackedInputAccess wiimoteNunchukDataAccess[] =
{
	{ 5, BIT(1), Input::Wiimote::NUN_C },
	{ 5, BIT(0), Input::Wiimote::NUN_Z },
};
