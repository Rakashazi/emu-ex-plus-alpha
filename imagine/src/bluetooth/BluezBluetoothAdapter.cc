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

#define thisModuleName "bluez"
#include "BluezBluetoothAdapter.hh"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hidp.h>
#include <util/fd-utils.h>
#include <errno.h>
#include <util/collection/DLList.hh>

#ifdef CONFIG_BASE_ANDROID
// Bluez dlsym functions
CLINK CallResult bluez_dl();
#endif

static BluezBluetoothAdapter defaultBluezAdapter;

bool BluezBluetoothAdapter::openDefault()
{
	if(socket > 0)
		return 1;
	logMsg("opening default BT adapter");
	#ifdef CONFIG_BASE_ANDROID
	if(bluez_dl() != OK)
		return 0;
	#endif
	devId = hci_get_route(0);
	if(devId < 0)
	{
		logMsg("no routes, errno: %d, %s", errno, strerror(errno));
		return 0;
	}
	socket = hci_open_dev(devId);
	if(socket < 0)
	{
		logMsg("error opening socket");
		return 0;
	}
	return 1;
}

void BluezBluetoothAdapter::cancelScan()
{
	scanCancelled = 1;
}

void BluezBluetoothAdapter::close()
{
	if(inDetect)
	{
		cancelScan();
		inDetect = 0;
	}
	if(socket > 0)
	{
		logMsg("closing BT device socket");
		::close(socket);
		socket = -1;
	}
}

BluezBluetoothAdapter *BluezBluetoothAdapter::defaultAdapter()
{
	if(defaultBluezAdapter.openDefault())
		return &defaultBluezAdapter;
	else
		return nullptr;
}

CallResult BluezBluetoothAdapter::doScan(const OnScanDeviceClassDelegate &onDeviceClass, const OnScanDeviceNameDelegate &onDeviceName)
{
	using namespace Base;
	logMsg("starting Bluetooth scan, cache %d", BluetoothAdapter::useScanCache);
	int devices = 0, maxDevices = 10;
	inquiry_info *deviceInfo = 0;
	devices = hci_inquiry(devId, scanSecs, maxDevices, 0, &deviceInfo,
		BluetoothAdapter::useScanCache ? 0 : IREQ_CACHE_FLUSH);
	if(devices == -1)
	{
		logMsg("inquiry failed");
		if(deviceInfo) free(deviceInfo);
		sendBTScanStatusDelegate(runThread, SCAN_FAILED);
		return INVALID_PARAMETER;
	}

	if(scanCancelled)
	{
		logMsg("cancelled scan after hci_inquiry");
		sendBTScanStatusDelegate(runThread, SCAN_CANCELLED);
		return OK;
	}

	logMsg("%d devices", devices);
	if(devices == 0)
	{
		sendBTScanStatusDelegate(runThread, SCAN_NO_DEVS);
		if(deviceInfo) free(deviceInfo);
		return OK;
	}
	else
		sendBTScanStatusDelegate(runThread, SCAN_PROCESSING, devices);
	iterateTimes(devices, i)
	{
		if(!onDeviceClass(*this, deviceInfo[i].dev_class))
		{
			logMsg("skipping device due to class %X:%X:%X", deviceInfo[i].dev_class[0], deviceInfo[i].dev_class[1], deviceInfo[i].dev_class[2]);
			continue;
		}

		if(scanCancelled)
		{
			logMsg("cancelled scan in hci_read_remote_name loop");
			sendBTScanStatusDelegate(runThread, SCAN_CANCELLED);
			return OK;
		}

		char name[248];
		if(hci_read_remote_name(socket, &deviceInfo[i].bdaddr, sizeof(name), name, 0) < 0)
		{
			logMsg("error reading device name");
			sendBTScanStatusDelegate(runThread, SCAN_NAME_FAILED);
			continue;
		}
		logMsg("device name: %s", name);
		onDeviceName(*this, name, deviceInfo[i].bdaddr.b);
	}
	if(deviceInfo) free(deviceInfo);

	if(scanCancelled)
	{
		logMsg("canceled scan after hci_read_remote_name loop");
		sendBTScanStatusDelegate(runThread, SCAN_CANCELLED);
		return OK;
	}

	sendBTScanStatusDelegate(runThread, SCAN_COMPLETE);
	return OK;
}

bool BluezBluetoothAdapter::startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName)
{
	if(!inDetect)
	{
		scanCancelled = 0;
		inDetect = 1;
		onScanStatusD = onResult;
		onScanDeviceClassD = onDeviceClass;
		onScanDeviceNameD = onDeviceName;
		runThread.create(1,
			[this](ThreadPThread &thread)
			{
				doScan(onScanDeviceClassD, onScanDeviceNameD);
				inDetect = 0;
				return 0;
			});
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

void BluezBluetoothAdapter::requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName)
{
	char name[248];
	auto &baddr = pending.addr.l2_bdaddr.b;
	if(hci_read_remote_name(defaultBluezAdapter.socket, &pending.addr.l2_bdaddr, sizeof(name), name, 0) < 0)
	{
		logErr("error reading device name");
		onDeviceName(*this, nullptr, baddr);
		return;
	}
	logMsg("device name: %s", name);
	onDeviceName(*this, name, baddr);
}

BluetoothAdapter::State BluezBluetoothAdapter::state()
{
	// TODO
	return STATE_ON;
}

void BluezBluetoothAdapter::setActiveState(bool on, OnStateChangeDelegate onStateChange)
{
	// TODO
	onStateChange(*this, on ? STATE_ON : STATE_OFF);
}

#ifdef CONFIG_BLUETOOTH_SERVER
void BluezBluetoothAdapter::setL2capService(uint psm, bool active, OnStatusDelegate onResult)
{
	if(!active)
	{
		logMsg("unregistering psm: 0x%X", psm);
		forEachInDLList(&serverList, e)
		{
			if(e.psm == psm)
			{
				::close(e.fd);
				e_it.removeElem();
				return;
			}
		}
		logMsg("psm not found in server list");
		return;
	}

	if(serverList.isFull())
	{
		logErr("too many l2cap services registered");
		onResult(*this, 0, 0);
		return;
	}
	logMsg("registering l2cap service for PSM 0x%X", psm);
	struct sockaddr_l2 addr;
	mem_zero(addr);
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(psm);

	auto serverFd = ::socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if(serverFd == -1)
	{
		logErr("error creating L2CAP socket with PSM %d", psm);
		onResult(*this, 0, 0);
		return;
	}

	int ret = bind(serverFd, (struct sockaddr *)&addr, sizeof(addr));
	if(ret == -1)
	{
		logErr("error in bind()");
		::close(serverFd);
		onResult(*this, 0, 0);
		return;
	}

	/*{
		int lm = L2CAP_LM_MASTER;
		setsockopt(serverFd, SOL_L2CAP, L2CAP_LM, &lm, sizeof(lm));
		struct l2cap_options opts;
		mem_zero(opts);
		opts.imtu = 64;
		opts.omtu = HIDP_DEFAULT_MTU;
		opts.flush_to = 0xffff;
		setsockopt(serverFd, SOL_L2CAP, L2CAP_OPTIONS, &opts, sizeof(opts));
	}*/

	ret = listen(serverFd, 1);
	if(ret == -1)
	{
		logErr("error in listen()");
		::close(serverFd);
		onResult(*this, 0, 0);
		return;
	}

	serverList.emplace_back(psm, serverFd,
		[this, serverFd](int events)
		{
			logMsg("incoming l2cap connection from server fd %d", serverFd);
			BluetoothPendingSocket pending;
			socklen_t addrLen = sizeof(pending.addr);
			pending.fd = accept(serverFd, (struct sockaddr *)&pending.addr, &addrLen);
			if(pending.fd == -1)
			{
				logErr("failed accepting connection");
				BluetoothPendingSocket error;
				onIncomingL2capConnectionD(*this, error);
				return 1;
			}
			logMsg("for PSM 0x%X, fd %d", pending.addr.l2_psm, pending.fd);
			onIncomingL2capConnectionD(*this, pending);
			return 1;
		}
	);
	Base::addPollEvent(serverFd, serverList.back().onConnect, Base::POLLEV_IN);
	onResult(*this, 1, 0);
	return;
}

/*bool BluezBluetoothAdapter::l2capServiceRegistered(uint psm)
{
	forEachInDLList(&serverList, e)
	{
		if(e.psm == psm)
		{
			return true;
		}
	}
	return false;
}*/
#endif

void BluetoothPendingSocket::close()
{
	::close(fd);
}

void BluetoothPendingSocket::requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName)
{
	defaultBluezAdapter.requestName(*this, onDeviceName);
}

CallResult BluezBluetoothSocket::open(BluetoothPendingSocket &pending)
{
	assert(pending);
	logMsg("accepting connection from fd %d", pending.fd);
	fd = pending.fd;
	pending = {};
	if(onStatusD(*this, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		Base::addPollEvent(fd, pollEvDel, Base::POLLEV_IN);
	return OK;
}

int BluezBluetoothSocket::readPendingData(int events)
{
	if(events & Base::POLLEV_ERR)
	{
		logMsg("poll error with events %X", events);
		if(events & Base::POLLEV_OUT) // happened while connecting
		{
			logErr("error connecting socket %d", fd);
			#ifndef NDEBUG
			socklen_t optLen = sizeof(int), opt = 0;
			getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &optLen);
			logMsg("got so_error %d", opt);
			#endif
			defaultBluezAdapter.onScanStatus()(defaultBluezAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
		}
		else
		{
			logMsg("socket %d disconnected", fd);
		}
		onStatusD(*this, STATUS_ERROR);
		return 0;
	}
	else if(events & Base::POLLEV_IN)
	{
		uchar buff[50];
		//logMsg("at least %d bytes ready on socket %d", fd_bytesReadable(fd), fd);
		while(fd_bytesReadable(fd))
		{
			//auto len = read(fd, buff, IG::min((size_t)bytesToRead, sizeof buff));
			auto len = read(fd, buff, sizeof buff);
			if(unlikely(len <= 0))
			{
				logMsg("error %d reading packet from socket %d", len == -1 ? errno : 0, fd);
				onStatusD(*this, STATUS_ERROR);
				return 0;
			}
			//logMsg("read %d bytes from socket %d", len, fd);
			if(!onDataD(buff, len))
				break; // socket was closed
		}
	}
	else if(events & Base::POLLEV_OUT)
	{
		logMsg("finished opening socket %d", fd);
		if(onStatusD(*this, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
			Base::modPollEvent(fd, pollEvDel, Base::POLLEV_IN);
		else
			Base::removePollEvent(fd);
	}

	return 1;
}

CallResult BluezBluetoothSocket::openRfcomm(BluetoothAddr bdaddr, uint channel)
{
	struct sockaddr_rc addr;
	mem_zero(addr);
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t)channel;
	memcpy(addr.rc_bdaddr.b, bdaddr.b, 6);
	//addr.rc_bdaddr = bdaddr;
	fd = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if(fd == -1)
	{
		logMsg("error creating RFCOMM socket with channel %d", channel);
		//onStatus.invoke(*this, STATUS_ERROR);
		return IO_ERROR;
	}
	fd_setNonblock(fd, 1);
	if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)
	{
		/*::close(fd);
		fd = 0;
		logMsg("error connecting RFCOMM socket with channel %d", channel);
		onStatus.invoke(*this, STATUS_ERROR);
		return IO_ERROR;*/
	}
	fd_setNonblock(fd, 0);

	Base::addPollEvent(fd, pollEvDel, Base::POLLEV_OUT);
	return OK;
}

CallResult BluezBluetoothSocket::openL2cap(BluetoothAddr bdaddr, uint psm)
{
	struct sockaddr_l2 addr;
	mem_zero(addr);
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(psm);
	memcpy(addr.l2_bdaddr.b, bdaddr.b, 6);
	//addr.l2_bdaddr = bdaddr;
	fd = ::socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if(fd == -1)
	{
		logMsg("error creating L2CAP socket with PSM %d", psm);
		return IO_ERROR;
	}

	fd_setNonblock(fd, 1);
	if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)
	{
		/*::close(fd);
		fd = 0;
		logMsg("error connecting L2CAP socket with PSM %d, errno %d", psm, errno);
		onStatus.invoke(*this, STATUS_ERROR);
		return IO_ERROR;*/
	}
	else
	{
		//logMsg("success");
	}
	fd_setNonblock(fd, 0);
	Base::addPollEvent(fd, pollEvDel, Base::POLLEV_OUT);
	return OK;
}

void BluezBluetoothSocket::close()
{
	if(fd >= 0)
	{
		/*if(shutdown(fd, SHUT_RDWR) != 0)
			logWarn("error shutting down socket");*/
		if(Base::hasFDEvents)
			Base::removePollEvent(fd);
		if(::close(fd) != 0)
			logWarn("error closing socket");
		fd = -1;
	}
}

CallResult BluezBluetoothSocket::write(const void *data, size_t size)
{
	assert(fd >= 0);
	if(fd_writeAll(fd, data, size) != (ssize_t)size)
	{
		return IO_ERROR;
	}
	return OK;
}
