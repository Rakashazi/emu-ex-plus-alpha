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

#define thisModuleName "btInput"
#include "BluetoothAdapter.hh"
#include "Wiimote.hh"
#include "Zeemote.hh"
#include "IControlPad.hh"
#include <util/collection/DLList.hh>

StaticDLList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticDLList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> btInputDevPendingList;

namespace Bluetooth
{
	uint scanSecs = 4;
	uint maxGamepadsPerType = 5;
	static BluetoothAdapter *bta = nullptr;

	static bool testSupportedBTDevClasses(const uchar devClass[3])
	{
		return Wiimote::isSupportedClass(devClass) ||
				IControlPad::isSupportedClass(devClass) ||
				Zeemote::isSupportedClass(devClass);
	}

	bool onClass(const uchar devClass[3])
	{
		logMsg("class: %X:%X:%X", devClass[0], devClass[1], devClass[2]);
		return testSupportedBTDevClasses(devClass);
	}

	void onName(const char *name, BluetoothAddr addr)
	{
		logMsg("name: %s", name);
		if(strstr(name, "Nintendo RVL-CNT-01"))
		{
			Wiimote *dev = new Wiimote(addr);
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			btInputDevPendingList.add(dev);
		}
		else if(strstr(name, "iControlPad-"))
		{
			IControlPad *dev = new IControlPad(addr);
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			btInputDevPendingList.add(dev);
		}
		else if(strstr(name, "Zeemote JS1"))
		{
			Zeemote *dev = new Zeemote(addr);
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			btInputDevPendingList.add(dev);
		}
	}

	static void removePendingDevs()
	{
		if(btInputDevPendingList.size)
			logMsg("removing %d devices in pending list", btInputDevPendingList.size);
		forEachInDLList(&btInputDevPendingList, e)
		{
			delete e;
			e_it.removeElem();
		}
	}

	bool startBT()
	{
		assert(bta);
		if(!bta->inDetect)
		{
			removePendingDevs();
			return bta->startScan();
		}
		return 0;
	}

	CallResult initBT()
	{
		if(bta)
			return OK; // already init

		bta = BluetoothAdapter::defaultAdapter();
		if(!bta)
			return UNSUPPORTED_OPERATION;
		bta->scanDeviceClassDelegate().bind<&onClass>();
		bta->scanDeviceNameDelegate().bind<&onName>();
		return OK;
	}

	void closeDevs()
	{
		if(!bta)
			return; // Bluetooth was never used
		logMsg("closing all BT input devs");
		while(btInputDevList.size)
		{
			(*btInputDevList.first())->removeFromSystem();
		}
	}

	uint pendingDevs()
	{
		return btInputDevPendingList.size;
	}

	void connectPendingDevs()
	{
		logMsg("connecting to %d devices", btInputDevPendingList.size);
		forEachInDLList(&btInputDevPendingList, e)
		{
			if(e->open(*bta) != OK)
			{
				delete e;
			}
			// e is added to btInputDevList
			e_it.removeElem();
		}
	}

	void closeBT()
	{
		if(!bta)
			return; // Bluetooth was never used
		if(bta->inDetect)
		{
			logMsg("keeping BT active due to scan");
			return;
		}
		removePendingDevs();
		closeDevs();
		bta->close();
		bta = nullptr;
	}

	uint devsConnected()
	{
		return btInputDevList.size;
	}
}
