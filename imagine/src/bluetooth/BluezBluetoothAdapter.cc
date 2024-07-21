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
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hidp.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/ranges.hh>
#include <imagine/util/algorithm.h>
#include <cerrno>

#ifdef __ANDROID__
// Bluez dlsym functions
CLINK int bluez_dl();
#endif

namespace IG
{

struct ScanStatusMessage
{
	BluetoothScanState type;
	uint8_t arg;
};

void BluezBluetoothAdapter::sendBTScanStatusDelegate(BluetoothScanState type, uint8_t arg = 0)
{
	ScanStatusMessage msg {type, arg};
	statusPipe.sink().put(msg);
//	if(write(statusPipe[1], &msg, sizeof(msg)) == -1)
//	{
//		logErr("error writing BT scan status to pipe");
//	}
}

// TODO: allow providing specific EventLoop to handle events
bool BluetoothAdapter::openDefault()
{
	if(isOpen())
		return true;
	logMsg("opening default BT adapter");
	#ifdef __ANDROID__
	if(bluez_dl() != 0)
		return;
	#endif
	devId = hci_get_route(0);
	if(devId < 0)
	{
		logMsg("no routes, errno: %d, %s", errno, strerror(errno));
		return false;
	}
	socket = hci_open_dev(devId);
	if(socket < 0)
	{
		logMsg("error opening socket");
		return false;
	}
	statusPipe.attach(
		[this](auto &io)
		{
			while(statusPipe.hasData())
			{
				auto &bta = static_cast<BluetoothAdapter&>(*this);
				auto msg = io.template getExpected<ScanStatusMessage>();
				if(!msg.has_value())
				{
					logErr("error reading BT socket status message in pipe");
					return true;
				}
				logMsg("got bluetooth adapter status delegate message");
				bta.onScanStatus(bta, msg->type, msg->arg);
			}
			return true;
		});
	return true;
}

bool BluetoothAdapter::isOpen() const { return socket != -1; }

void BluetoothAdapter::cancelScan()
{
	scanCancelled = 1;
}

bool BluetoothAdapter::isInScan() const { return inDetect; }

void BluetoothAdapter::close()
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

bool BluezBluetoothAdapter::doScan(const BTOnScanDeviceClassDelegate &onDeviceClass, const BTOnScanDeviceNameDelegate &onDeviceName)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	logMsg("starting Bluetooth scan, cache %d", bta.useScanCache);
	int devices = 0, maxDevices = 10;
	inquiry_info *deviceInfo = 0;
	devices = hci_inquiry(devId, bta.scanSecs, maxDevices, 0, &deviceInfo,
		bta.useScanCache ? 0 : IREQ_CACHE_FLUSH);
	if(devices == -1)
	{
		logMsg("inquiry failed");
		if(deviceInfo) free(deviceInfo);
		sendBTScanStatusDelegate(BluetoothScanState::Failed);
		return false;
	}

	if(scanCancelled)
	{
		logMsg("cancelled scan after hci_inquiry");
		sendBTScanStatusDelegate(BluetoothScanState::Cancelled);
		return true;
	}

	logMsg("%d devices", devices);
	if(devices == 0)
	{
		sendBTScanStatusDelegate(BluetoothScanState::NoDevs);
		if(deviceInfo) free(deviceInfo);
		return true;
	}
	else
		sendBTScanStatusDelegate(BluetoothScanState::Processing, devices);
	for(auto i : iotaCount(devices))
	{
		if(!onDeviceClass(bta, std::to_array(deviceInfo[i].dev_class)))
		{
			logMsg("skipping device due to class %X:%X:%X", deviceInfo[i].dev_class[0], deviceInfo[i].dev_class[1], deviceInfo[i].dev_class[2]);
			continue;
		}

		if(scanCancelled)
		{
			logMsg("cancelled scan in hci_read_remote_name loop");
			sendBTScanStatusDelegate(BluetoothScanState::Cancelled);
			return true;
		}

		char name[248];
		if(hci_read_remote_name(socket, &deviceInfo[i].bdaddr, sizeof(name), name, 0) < 0)
		{
			logMsg("error reading device name");
			sendBTScanStatusDelegate(BluetoothScanState::NameFailed);
			continue;
		}
		logMsg("device name: %s", name);
		onDeviceName(bta, name, deviceInfo[i].bdaddr.b);
	}
	if(deviceInfo) free(deviceInfo);

	if(scanCancelled)
	{
		logMsg("canceled scan after hci_read_remote_name loop");
		sendBTScanStatusDelegate(BluetoothScanState::Cancelled);
		return true;
	}

	sendBTScanStatusDelegate(BluetoothScanState::Complete);
	return true;
}

bool BluetoothAdapter::startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName)
{
	if(!inDetect)
	{
		scanCancelled = 0;
		inDetect = 1;
		onScanStatus = onResult;
		onScanDeviceClass = onDeviceClass;
		onScanDeviceName = onDeviceName;
		IG::makeDetachedThread(
			[this]()
			{
				doScan(onScanDeviceClass, onScanDeviceName);
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

void BluetoothAdapter::requestName(BluetoothPendingSocket& pending, BTOnScanDeviceNameDelegate onDeviceName)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	char name[248];
	auto &baddr = pending.addr.l2_bdaddr.b;
	if(hci_read_remote_name(bta.socket, &pending.addr.l2_bdaddr, sizeof(name), name, 0) < 0)
	{
		logErr("error reading device name");
		onDeviceName(bta, nullptr, baddr);
		return;
	}
	logMsg("device name: %s", name);
	onDeviceName(bta, name, baddr);
}

BluetoothAdapter::State BluetoothAdapter::state()
{
	// TODO
	return BluetoothState::On;
}

void BluetoothAdapter::setActiveState(bool on, OnStateChangeDelegate onStateChange)
{
	// TODO
	onStateChange(*this, on ? BluetoothState::On : BluetoothState::Off);
}

void BluetoothAdapter::setL2capService(uint32_t psm, bool active, OnStatusDelegate onResult)
{
	if(!active)
	{
		logMsg("unregistering psm: 0x%X", psm);
		if(auto removedServer = moveOut(serverList, [&](const L2CapServer& server){ return server.psm == psm; });
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
		onResult(*this, BluetoothScanState::Failed, 0);
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
		onResult(*this, BluetoothScanState::Failed, 0);
		return;
	}

	int ret = bind(serverFd, (struct sockaddr *)&addr, sizeof(addr));
	if(ret == -1)
	{
		logErr("error in bind()");
		::close(serverFd);
		onResult(*this, BluetoothScanState::Failed, 0);
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
		onResult(*this, BluetoothScanState::Failed, 0);
		return;
	}

	auto &server = serverList.emplace_back(psm, serverFd);
	server.connectSrc = {serverFd, {.eventLoop = EventLoop::forThread()},
		[this](int fd, int)
		{
			logMsg("incoming l2cap connection from server fd %d", fd);
			BluetoothPendingSocket pending;
			socklen_t addrLen = sizeof(pending.addr);
			pending.fd = accept(fd, (struct sockaddr *)&pending.addr, &addrLen);
			if(pending.fd == -1)
			{
				logErr("failed accepting connection");
				BluetoothPendingSocket error;
				onIncomingL2capConnection(*this, error);
				return true;
			}
			logMsg("for PSM 0x%X, fd %d", pending.addr.l2_psm, pending.fd);
			onIncomingL2capConnection(*this, pending);
			return true;
		}
	};
	//addPollEvent(serverFd, serverList.back().onConnect, pollEventInput);
	onResult(*this, BluetoothScanState::Complete, 0);
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

void BluetoothPendingSocket::close()
{
	::close(fd);
}

void BluetoothPendingSocket::requestName(BluetoothAdapter& bta, BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName)
{
	bta.requestName(*this, onDeviceName);
}

std::system_error BluetoothSocket::open(BluetoothAdapter&, BluetoothPendingSocket& pending)
{
	assert(pending);
	logMsg("accepting connection from fd %d", pending.fd);
	fd = pending.fd;
	pending = {};
	if(onStatus(*this, BluetoothSocketState::Opened) == 1)
		setupFDEvents(pollEventInput);
		//addPollEvent(fd, pollEvDel, pollEventInput);
	return std::error_code{};
}

bool BluezBluetoothSocket::readPendingData(PollEventFlags events)
{
	auto &sock = static_cast<BluetoothSocket&>(*this);
	if(events & pollEventError)
	{
		logMsg("poll error with events %X", events);
		if(events & pollEventOutput) // happened while connecting
		{
			logErr("error connecting socket %d", fd);
			if(Config::DEBUG_BUILD)
			{
				socklen_t optLen = sizeof(int), opt = 0;
				getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &optLen);
				logMsg("got so_error %d", opt);
			}
			sock.onStatus(sock, BluetoothSocketState::ConnectError);
			//defaultBluezAdapter.onScanStatus()(defaultBluezAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
		}
		else
		{
			logMsg("socket %d disconnected", fd);
			sock.onStatus(sock, BluetoothSocketState::ReadError);
		}
		return false;
	}
	else if(events & pollEventInput)
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
				sock.onStatus(sock, BluetoothSocketState::ReadError);
				return false;
			}
			//logMsg("read %d bytes from socket %d", len, fd);
			if(!sock.onData(buff, len))
				break; // socket was closed
		}
	}
	else if(events & pollEventOutput)
	{
		logMsg("finished opening socket %d", fd);
		if(sock.onStatus(sock, BluetoothSocketState::Opened) == 1)
			fdSrc.setEvents(pollEventInput);
		else
			fdSrc.detach();
	}

	return true;
}

void BluezBluetoothSocket::setupFDEvents(PollEventFlags events)
{
	fdSrc = {fd, {.eventLoop = EventLoop::forThread(), .events = events}, [this](int, int events) {return readPendingData(events); }};
}

std::system_error BluetoothSocket::openRfcomm(BluetoothAdapter &, BluetoothAddr bdaddr, uint32_t channel)
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
		return std::system_error{errno, std::generic_category()};
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
	setupFDEvents(pollEventOutput);
	return std::error_code{};
}

std::system_error BluetoothSocket::openL2cap(BluetoothAdapter &, BluetoothAddr bdaddr, uint32_t psm)
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
		return std::system_error{errno, std::generic_category()};
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
	setupFDEvents(pollEventOutput);
	return std::error_code{};
}

BluezBluetoothSocket::~BluezBluetoothSocket()
{
	close();
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

ssize_t BluetoothSocket::write(const void *data, size_t size)
{
	assert(fd >= 0);
	return fd_writeAll(fd, data, size);
}

}
