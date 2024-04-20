#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/util/DelegateFunc.hh>
#include <array>
#include <cstdint>

#if defined __ANDROID__
#define CONFIG_BLUETOOTH_ANDROID
#elif defined __APPLE__ && TARGET_OS_IPHONE
#define CONFIG_BLUETOOTH_BTSTACK
#elif defined __linux__
#define CONFIG_BLUETOOTH_BLUEZ
#endif

namespace Config::Bluetooth
{

#if defined CONFIG_BLUETOOTH_BLUEZ || defined CONFIG_BLUETOOTH_BTSTACK
#define CONFIG_BLUETOOTH_SERVER
constexpr bool server = true;
#else
constexpr bool server = false;
#endif

#if defined CONFIG_BLUETOOTH_BLUEZ || defined CONFIG_BLUETOOTH_BTSTACK
#define CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
#define CONFIG_BLUETOOTH_SCAN_SECS
constexpr bool scanCache = true;
constexpr bool scanTime = true;
#else
constexpr bool scanCache = false;
constexpr bool scanTime = false;
#endif

}

namespace IG
{

class BluetoothAdapter;
class BluetoothSocket;
class BluetoothPendingSocket;
struct BluetoothAddr;

enum class BluetoothState: uint8_t { Off, On, TurningOff, TurningOn, Error };
enum class BluetoothScanState: uint8_t { InitFailed = 1, Failed, Processing, NoDevs, NameFailed, Complete, Cancelled /*, SocketOpenFailed*/ };

// socket
enum class BluetoothSocketState: uint8_t { Connecting, ConnectError, Opened, ReadError, Closed };

using BTOnStateChangeDelegate = DelegateFunc<void (BluetoothAdapter&, BluetoothState)>;
using BTOnStatusDelegate = DelegateFunc<void (BluetoothAdapter&, BluetoothScanState, int arg)>;
using BTOnScanDeviceClassDelegate = DelegateFunc<bool (BluetoothAdapter&, std::array<uint8_t, 3> devClass)>;
using BTOnScanDeviceNameDelegate = DelegateFunc<void (BluetoothAdapter&, const char *name, BluetoothAddr)>;
using BTOnIncomingL2capConnectionDelegate = DelegateFunc<void (BluetoothAdapter&, BluetoothPendingSocket&)>;

using BluetoothAddrString = std::array<char, 18>;

struct BluetoothAddr : public std::array<uint8_t, 6>
{
	using BaseArray = std::array<uint8_t, 6>;
	using BaseArray::BaseArray;

	constexpr BluetoothAddr(const uint8_t b[6]): std::array<uint8_t, 6>{b[0], b[1], b[2], b[3], b[4], b[5]} {}
};

}
