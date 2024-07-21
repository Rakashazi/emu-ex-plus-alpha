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

#define LOGTAG "BTInput"
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/bluetooth/Wiimote.hh>
#include <imagine/bluetooth/Zeemote.hh>
#include <imagine/bluetooth/IControlPad.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Timer.hh>
#ifdef CONFIG_BLUETOOTH_SERVER
#include <imagine/bluetooth/PS3Controller.hh>
#endif

namespace IG::Bluetooth
{

static std::vector<std::unique_ptr<Input::Device>> btInputDevPendingList;
static bool hidServiceActive = false;

#ifdef CONFIG_BLUETOOTH_SERVER
static std::unique_ptr<Input::Device> pendingPS3Controller;
static BluetoothPendingSocket pendingSocket;
static BluetoothAdapter::OnStatusDelegate onServerStatus;
static std::optional<Timer> unregisterHIDServiceCallback;
#endif

static bool testSupportedBTDevClasses(std::array<uint8_t, 3> devClass)
{
	return Wiimote::isSupportedClass(devClass) ||
			IControlPad::isSupportedClass(devClass) ||
			Zeemote::isSupportedClass(devClass);
}

static void removePendingDevs()
{
	if(btInputDevPendingList.size())
		logMsg("removing %d devices in pending list", (int)btInputDevPendingList.size());
	btInputDevPendingList.clear();
}

#ifdef CONFIG_BLUETOOTH_SERVER
bool listenForDevices(ApplicationContext ctx, BluetoothAdapter &bta, const BluetoothAdapter::OnStatusDelegate &onScanStatus)
{
	if(bta.isInScan() || hidServiceActive)
	{
		return false;
	}
	onServerStatus = onScanStatus;
	bta.onIncomingL2capConnection =
		[ctx](BluetoothAdapter &bta, BluetoothPendingSocket &pending)
		{
			if(!pending)
			{
				// TODO: use different status type for this
				//onServerStatus(bta, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
			}
			else if(pending.channel() == 0x13 && pendingPS3Controller)
			{
				logMsg("request for PSM 0x13");
				getAs<PS3Controller>(*pendingPS3Controller).open2Int(bta, pending);
				pendingPS3Controller = {};
			}
			else if(pending.channel() == 0x11)
			{
				logMsg("request for PSM 0x11");
				pendingSocket = pending;
				pending.requestName(bta,
					[ctx](BluetoothAdapter &bta, const char *name, BluetoothAddr addr)
					{
						if(!name)
						{
							onServerStatus(bta, BluetoothScanState::NameFailed, 0);
							pendingSocket = {};
							return;
						}
						logMsg("name: %s", name);
						if(pendingSocket)
						{
							if(strstr(name, "PLAYSTATION(R)3"))
							{
								auto dev = std::make_unique<Input::Device>(std::in_place_type<PS3Controller>, ctx, addr);
								if(!dev)
								{
									logErr("out of memory");
									return;
								}
								getAs<PS3Controller>(*dev).open1Ctl(bta, pendingSocket, *dev);
								pendingSocket = {};
								pendingPS3Controller = std::move(dev);
							}
						}
					}
				);
			}
			else
			{
				logMsg("unknown request for PSM 0x%X", pending.channel());
				pending.close();
			}
		};

	logMsg("registering HID PSMs");
	hidServiceActive = true;
	bta.setL2capService(0x13, true,
		[](BluetoothAdapter& bta, BluetoothScanState state, int)
		{
			if(state <= BluetoothScanState::Failed)
			{
				hidServiceActive = false;
				onServerStatus(bta, BluetoothScanState::InitFailed, 0);
				return;
			}
			logMsg("INT PSM registered");
			// now register the 2nd PSM
			bta.setL2capService(0x11, true,
				[](BluetoothAdapter& bta, BluetoothScanState state, int)
				{
					if(state <= BluetoothScanState::Failed)
					{
						bta.setL2capService(0x13, false, {});
						hidServiceActive = false;
						onServerStatus(bta, BluetoothScanState::InitFailed, 0);
						return;
					}
					logMsg("CTL PSM registered");
					onServerStatus(bta, BluetoothScanState::Complete, 0);
					// both PSMs are registered
					if(!unregisterHIDServiceCallback)
					{
						unregisterHIDServiceCallback.emplace(TimerDesc{.debugLabel = "unregisterHIDServiceCallback"},
							[&bta]
							{
								logMsg("unregistering HID PSMs from timeout");
								bta.setL2capService(0x11, false, {});
								bta.setL2capService(0x13, false, {});
								hidServiceActive = false;
							});
					}
					unregisterHIDServiceCallback->runIn(IG::Seconds{8});
				}
			);
		}
	);

	return true;
}
#endif

bool scanForDevices(ApplicationContext ctx, BluetoothAdapter &bta, BluetoothAdapter::OnStatusDelegate onScanStatus)
{
	if(!bta.isInScan() && !hidServiceActive)
	{
		removePendingDevs();
		return bta.startScan(onScanStatus,
			[](BluetoothAdapter &, std::array<uint8_t, 3> devClass) // on device class
			{
				logMsg("class: %X:%X:%X", devClass[0], devClass[1], devClass[2]);
				return testSupportedBTDevClasses(devClass);
			},
			[&onScanStatus, ctx](BluetoothAdapter &bta, const char *name, BluetoothAddr addr) // on device name
			{
				if(!name)
				{
					onScanStatus(bta, BluetoothScanState::NameFailed, 0);
					return;
				}
				if(strstr(name, "Nintendo RVL-CNT-01"))
				{
					btInputDevPendingList.emplace_back(std::make_unique<Input::Device>(std::in_place_type<Wiimote>, ctx, addr));
				}
				else if(strstr(name, "iControlPad-"))
				{
					btInputDevPendingList.emplace_back(std::make_unique<Input::Device>(std::in_place_type<IControlPad>, ctx, addr));
				}
				else if(strstr(name, "Zeemote JS1"))
				{
					btInputDevPendingList.emplace_back(std::make_unique<Input::Device>(std::in_place_type<Zeemote>, ctx, addr));
				}
			}
		);
	}
	return 0;
}

void closeDevices(BluetoothAdapter& bta)
{
	if(!bta.isOpen())
		return; // Bluetooth was never used
	logMsg("closing all BT input devs");
	auto ctx = bta.appContext();
	auto &app = ctx.application();
	app.removeInputDevices(ctx, Input::Map::WIIMOTE);
	app.removeInputDevices(ctx, Input::Map::WII_CC);
	app.removeInputDevices(ctx, Input::Map::ICONTROLPAD);
	app.removeInputDevices(ctx, Input::Map::ZEEMOTE);
	#ifdef CONFIG_BLUETOOTH_SERVER
	app.removeInputDevices(ctx, Input::Map::PS3PAD);
	#endif
}

size_t pendingDevs()
{
	return btInputDevPendingList.size();
}

void connectPendingDevs(BluetoothAdapter& bta)
{
	logMsg("connecting to %d devices", (int)btInputDevPendingList.size());
	for(auto &e : btInputDevPendingList)
	{
		visit([&](auto &btDev)
		{
			if constexpr(requires {btDev.open(bta, *e);})
				btDev.open(bta, *e);
		}, *e);
	}
}

void closeBT(BluetoothAdapter& bta)
{
	if(!bta.isOpen())
		return; // Bluetooth was never used
	if(bta.isInScan())
	{
		logMsg("keeping BT active due to scan");
		return;
	}
	removePendingDevs();
	closeDevices(bta);
	bta.close();
}

static bool isBluetoothDeviceInputMap(Input::Map map)
{
	switch(map)
	{
		case Input::Map::WIIMOTE:
		case Input::Map::WII_CC:
		case Input::Map::ICONTROLPAD:
		case Input::Map::ZEEMOTE:
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Input::Map::PS3PAD:
		#endif
			return true;
		default:
			return false;
	}
}

size_t devsConnected(ApplicationContext ctx)
{
	auto &devs = ctx.inputDevices();
	return std::count_if(devs.begin(), devs.end(), [](auto &devPtr){ return isBluetoothDeviceInputMap(devPtr->map()); });
}

}

namespace IG
{

BluetoothInputDevice::BluetoothInputDevice(ApplicationContext ctx, Input::Map map,
	Input::DeviceTypeFlags typeFlags, const char *name):
	BaseDevice{0, map, typeFlags, name},
	ctx{ctx} {}

void BaseApplication::bluetoothInputDeviceStatus(ApplicationContext ctx, Input::Device &dev, BluetoothSocketState status)
{
	using namespace Bluetooth;
	switch(status)
	{
		case BluetoothSocketState::ConnectError:
			std::erase_if(btInputDevPendingList, [&](auto &devPtr){ return devPtr.get() == &dev; });
			break;
		case BluetoothSocketState::Opened:
		{
			logMsg("back %p, param %p", btInputDevPendingList.back().get(), &dev);
			auto devPtr = moveOut(btInputDevPendingList, [&](auto &devPtr){ return devPtr.get() == &dev; });
			logMsg("moving %p", devPtr.get());
			if(devPtr)
			{
				addInputDevice(ctx, std::move(devPtr), true);
			}
			break;
		}
		case BluetoothSocketState::ReadError:
		case BluetoothSocketState::Closed:
			removeInputDevice(ctx, dev, true);
			break;
		case BluetoothSocketState::Connecting: break;
	}
}

}
