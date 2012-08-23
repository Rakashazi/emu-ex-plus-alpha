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
#include <util/fd-utils.h>
#include <errno.h>
#include <util/collection/DLList.hh>

#ifdef CONFIG_BASE_ANDROID
// Bluez dlsym functions
CLINK CallResult bluez_dl();
#endif

static BluezBluetoothAdapter defaultBluezAdapter;
uint scanSecs = 4;

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
		logMsg("no routes");
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

void BluezBluetoothAdapter::close()
{
	if(socket > 0)
	{
		logMsg("closing BT device socket");
		::close(socket);
		socket = 0;
	}
	inDetect = 0;
}

BluezBluetoothAdapter *BluezBluetoothAdapter::defaultAdapter()
{
	if(defaultBluezAdapter.openDefault())
		return &defaultBluezAdapter;
	else
		return nullptr;
}

CallResult BluezBluetoothAdapter::doScan()
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

	logMsg("%d devices", devices);
	if(devices == 0)
	{
		sendBTScanStatusDelegate(runThread, SCAN_NO_DEVS);
		if(deviceInfo) free(deviceInfo);
		return OK;
	}
	else
		sendBTScanStatusDelegate(runThread, SCAN_PROCESSING, devices);
	bool usedDevice = 0, hadError = 0;
	iterateTimes(devices, i)
	{
		if(!onScanDeviceClass.invoke(deviceInfo[i].dev_class))
		{
			logMsg("skipping device due to class %X:%X:%X", deviceInfo[i].dev_class[0], deviceInfo[i].dev_class[1], deviceInfo[i].dev_class[2]);
			continue;
		}
		char name[248];
		if(hci_read_remote_name(socket, &deviceInfo[i].bdaddr, sizeof(name), name, 0) < 0)
		{
			logMsg("error reading device name");
			sendBTScanStatusDelegate(runThread, SCAN_NAME_FAILED);
			hadError = 1;
			continue;
		}
		logMsg("device name: %s", name);
		usedDevice = 1;
		BluetoothAddr addr;
		memcpy(addr.b, deviceInfo[i].bdaddr.b, 6);
		onScanDeviceName.invoke(name, addr);
	}
	if(deviceInfo) free(deviceInfo);
	if(usedDevice)
		sendBTScanStatusDelegate(runThread, SCAN_COMPLETE);
	else if(!hadError) // don't clobber error message
		sendBTScanStatusDelegate(runThread, SCAN_COMPLETE_NO_DEVS_USED);
	return OK;
}

static int runScan(ThreadPThread &thread)
{
	BluezBluetoothAdapter *adapter = (BluezBluetoothAdapter*)thread.arg;
	adapter->doScan();
	adapter->inDetect = 0;
	return 0;
}

fbool BluezBluetoothAdapter::startScan()
{
	if(!inDetect)
	{
		inDetect = 1;
		runThread.create(1, runScan, this);
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

void BluezBluetoothAdapter::constructSocket(void *mem)
{
	new(mem) BluezBluetoothSocket();
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
			BluezBluetoothAdapter::defaultAdapter()->statusDelegate().invoke(BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
		}
		else
		{
			logMsg("socket %d disconnected", fd);
		}
		onStatus.invoke(*this, STATUS_ERROR);
		return 0;
	}
	else if(events & Base::POLLEV_IN)
	{
		uchar buff[48];
		int bytesToRead = fd_bytesReadable(fd);
		logMsg("%d bytes ready on socket %d", bytesToRead, fd);
		do
		{
			int len = read(fd, buff, IG::min((uint)bytesToRead, uint(sizeof buff)));
			if(unlikely(len <= 0))
			{
				logMsg("error %d reading packet from socket %d", len == -1 ? errno : len, fd);
				onStatus.invoke(*this, STATUS_ERROR);
				return 0;
			}
			//logMsg("read %d bytes from socket %d", len, fd);
			bytesToRead -= len;
			if(!onDataEvent.invoke(buff, len))
				break; // socket was closed
		} while(bytesToRead > 0);
		/*bytesToRead = fd_bytesReadable(fd);
		logMsg("%d bytes left on socket %d", bytesToRead, fd);*/
	}
	else if(events & Base::POLLEV_OUT)
	{
		logMsg("finished opening socket %d", fd);
		if(onStatus.invoke(*this, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
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

	//pollEvDel.bind<BluezBluetoothSocket, &BluezBluetoothSocket::readPendingData>(this);
	Base::addPollEvent2(fd, pollEvDel, Base::POLLEV_OUT);
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
	//pollEvDel.bind<BluezBluetoothSocket, &BluezBluetoothSocket::readPendingData>(this);
	Base::addPollEvent2(fd, pollEvDel, Base::POLLEV_OUT);
	return OK;
}

void BluezBluetoothSocket::close()
{
	if(fd > 0)
	{
		if(Base::hasFDEvents)
			Base::removePollEvent(fd);
		if(::close(fd) != 0)
			logWarn("error closing socket");
		fd = 0;
	}
}

CallResult BluezBluetoothSocket::write(const void *data, size_t size)
{
	assert(fd > 0);
	if(fd_writeAll(fd, data, size) != (ssize_t)size)
	{
		return IO_ERROR;
	}
	return OK;
}
