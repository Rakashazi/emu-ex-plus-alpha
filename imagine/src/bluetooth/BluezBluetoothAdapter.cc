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

#define LOGTAG "Bluez"
#include <imagine/bluetooth/BluezBluetoothAdapter.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hidp.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/algorithm.h>
#include <errno.h>

#ifdef __ANDROID__
// Bluez dlsym functions
CLINK int bluez_dl();
#endif

struct ScanStatusMessage
{
	uint8_t type, arg;
};

static BluezBluetoothAdapter defaultBluezAdapter;

void BluezBluetoothAdapter::sendBTScanStatusDelegate(uint8_t type, uint8_t arg = 0)
{
	ScanStatusMessage msg {type, arg};
	statusPipe.sink().write(&msg, sizeof(msg));
//	if(write(statusPipe[1], &msg, sizeof(msg)) == -1)
//	{
//		logErr("error writing BT scan status to pipe");
//	}
}

// TODO: allow providing specific EventLoop to handle events
bool BluezBluetoothAdapter::openDefault()
{
	if(socket > 0)
		return 1;
	logMsg("opening default BT adapter");
	#ifdef __ANDROID__
	if(bluez_dl() != 0)
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

	{
		statusPipe.attach(
			[this](auto &io)
			{
				while(statusPipe.hasData())
				{
					auto [msg, size] = io.template read<ScanStatusMessage>();
					if(size == -1)
					{
						logErr("error reading BT socket status message in pipe");
						return 1;
					}
					logMsg("got bluetooth adapter status delegate message");
					onScanStatus()(*this, msg.type, msg.arg);
				}
				return 1;
			});
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
	statusPipe.detach();
}

BluezBluetoothAdapter *BluezBluetoothAdapter::defaultAdapter(Base::ApplicationContext)
{
	if(defaultBluezAdapter.openDefault())
		return &defaultBluezAdapter;
	else
		return nullptr;
}

IG::ErrorCode BluezBluetoothAdapter::doScan(const OnScanDeviceClassDelegate &onDeviceClass, const OnScanDeviceNameDelegate &onDeviceName)
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
		sendBTScanStatusDelegate(SCAN_FAILED);
		return {EINVAL};
	}

	if(scanCancelled)
	{
		logMsg("cancelled scan after hci_inquiry");
		sendBTScanStatusDelegate(SCAN_CANCELLED);
		return {};
	}

	logMsg("%d devices", devices);
	if(devices == 0)
	{
		sendBTScanStatusDelegate(SCAN_NO_DEVS);
		if(deviceInfo) free(deviceInfo);
		return {};
	}
	else
		sendBTScanStatusDelegate(SCAN_PROCESSING, devices);
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
			sendBTScanStatusDelegate(SCAN_CANCELLED);
			return {};
		}

		char name[248];
		if(hci_read_remote_name(socket, &deviceInfo[i].bdaddr, sizeof(name), name, 0) < 0)
		{
			logMsg("error reading device name");
			sendBTScanStatusDelegate(SCAN_NAME_FAILED);
			continue;
		}
		logMsg("device name: %s", name);
		onDeviceName(*this, name, deviceInfo[i].bdaddr.b);
	}
	if(deviceInfo) free(deviceInfo);

	if(scanCancelled)
	{
		logMsg("canceled scan after hci_read_remote_name loop");
		sendBTScanStatusDelegate(SCAN_CANCELLED);
		return {};
	}

	sendBTScanStatusDelegate(SCAN_COMPLETE);
	return {};
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
		IG::makeDetachedThread(
			[this]()
			{
				doScan(onScanDeviceClassD, onScanDeviceNameD);
				inDetect = 0;
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
void BluezBluetoothAdapter::setL2capService(uint32_t psm, bool active, OnStatusDelegate onResult)
{
	if(!active)
	{
		logMsg("unregistering psm: 0x%X", psm);
		if(auto removedServer = IG::moveOutIf(serverList, [&](L2CapServer &server){ return server.psm == psm; });
			removedServer.fd != -1)
		{
			::close(removedServer.fd);
			return;
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
	struct sockaddr_l2 addr{};
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
		struct l2cap_options opts{};
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

	auto &server = serverList.emplace_back(psm, serverFd);
	server.connectSrc = {serverFd, {},
		[this](int fd, int events)
		{
			logMsg("incoming l2cap connection from server fd %d", fd);
			BluetoothPendingSocket pending;
			socklen_t addrLen = sizeof(pending.addr);
			pending.fd = accept(fd, (struct sockaddr *)&pending.addr, &addrLen);
			if(pending.fd == -1)
			{
				logErr("failed accepting connection");
				BluetoothPendingSocket error;
				onIncomingL2capConnectionD(*this, error);
				return true;
			}
			logMsg("for PSM 0x%X, fd %d", pending.addr.l2_psm, pending.fd);
			onIncomingL2capConnectionD(*this, pending);
			return true;
		}};
	//Base::addPollEvent(serverFd, serverList.back().onConnect, Base::POLLEV_IN);
	onResult(*this, 1, 0);
	return;
}

/*bool BluezBluetoothAdapter::l2capServiceRegistered(uint32_t psm)
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

IG::ErrorCode BluezBluetoothSocket::open(BluetoothAdapter &, BluetoothPendingSocket &pending)
{
	assert(pending);
	logMsg("accepting connection from fd %d", pending.fd);
	fd = pending.fd;
	pending = {};
	if(onStatusD(*this, STATUS_OPENED) == OPEN_USAGE_READ_EVENTS)
		setupFDEvents(Base::POLLEV_IN);
		//Base::addPollEvent(fd, pollEvDel, Base::POLLEV_IN);
	return {};
}

int BluezBluetoothSocket::readPendingData(int events)
{
	if(events & Base::POLLEV_ERR)
	{
		logMsg("poll error with events %X", events);
		if(events & Base::POLLEV_OUT) // happened while connecting
		{
			logErr("error connecting socket %d", fd);
			if(Config::DEBUG_BUILD)
			{
				socklen_t optLen = sizeof(int), opt = 0;
				getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &optLen);
				logMsg("got so_error %d", opt);
			}
			onStatusD(*this, STATUS_CONNECT_ERROR);
			//defaultBluezAdapter.onScanStatus()(defaultBluezAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
		}
		else
		{
			logMsg("socket %d disconnected", fd);
			onStatusD(*this, STATUS_READ_ERROR);
		}
		return false;
	}
	else if(events & Base::POLLEV_IN)
	{
		char buff[50];
		//logMsg("at least %d bytes ready on socket %d", fd_bytesReadable(fd), fd);
		while(fd_bytesReadable(fd))
		{
			//auto len = read(fd, buff, IG::min((size_t)bytesToRead, sizeof buff));
			auto len = read(fd, buff, sizeof buff);
			if(len <= 0) [[unlikely]]
			{
				logMsg("error %d reading packet from socket %d", len == -1 ? errno : 0, fd);
				onStatusD(*this, STATUS_READ_ERROR);
				return false;
			}
			//logMsg("read %d bytes from socket %d", len, fd);
			if(!onDataD(buff, len))
				break; // socket was closed
		}
	}
	else if(events & Base::POLLEV_OUT)
	{
		logMsg("finished opening socket %d", fd);
		if(onStatusD(*this, STATUS_OPENED) == OPEN_USAGE_READ_EVENTS)
			fdSrc.setEvents(Base::POLLEV_IN);
		else
			fdSrc.detach();
	}

	return true;
}

void BluezBluetoothSocket::setupFDEvents(int events)
{
	fdSrc = {fd, {},
		[this](int fd, int events)
		{
			return readPendingData(events);
		},
		Base::POLLEV_OUT};
}

IG::ErrorCode BluezBluetoothSocket::openRfcomm(BluetoothAdapter &, BluetoothAddr bdaddr, uint32_t channel)
{
	struct sockaddr_rc addr{};
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t)channel;
	memcpy(addr.rc_bdaddr.b, bdaddr.data(), 6);
	//addr.rc_bdaddr = bdaddr;
	fd = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if(fd == -1)
	{
		logMsg("error creating RFCOMM socket with channel %d", channel);
		//onStatus.invoke(*this, STATUS_ERROR);
		return {EIO};
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
	setupFDEvents(Base::POLLEV_OUT);
	return {};
}

IG::ErrorCode BluezBluetoothSocket::openL2cap(BluetoothAdapter &, BluetoothAddr bdaddr, uint32_t psm)
{
	struct sockaddr_l2 addr{};
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(psm);
	memcpy(addr.l2_bdaddr.b, bdaddr.data(), 6);
	//addr.l2_bdaddr = bdaddr;
	fd = ::socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if(fd == -1)
	{
		logMsg("error creating L2CAP socket with PSM %d", psm);
		return {EIO};
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
	setupFDEvents(Base::POLLEV_OUT);
	return {};
}

void BluezBluetoothSocket::close()
{
	if(fd >= 0)
	{
		/*if(shutdown(fd, SHUT_RDWR) != 0)
			logWarn("error shutting down socket");*/
		fdSrc.detach();
		if(::close(fd) != 0)
			logWarn("error closing socket");
		fd = -1;
	}
}

IG::ErrorCode BluezBluetoothSocket::write(const void *data, size_t size)
{
	assert(fd >= 0);
	if(fd_writeAll(fd, data, size) != (ssize_t)size)
	{
		return {EIO};
	}
	return {};
}
