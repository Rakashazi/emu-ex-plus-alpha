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

#define LOGTAG "AndroidBT"
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/jni.hh>
#include <cerrno>
#include "../base/android/android.hh"
#include "utils.hh"

namespace IG
{

struct SocketStatusMessage
{
	BluetoothSocket *socket{};
	BluetoothSocketState type{};

	constexpr SocketStatusMessage() {}
	constexpr SocketStatusMessage(BluetoothSocket &socket, BluetoothSocketState type): socket(&socket), type(type) {}
};

// From Android source header abort_socket.h
struct asocket
{
	int fd;           /* primary socket fd */
	int abort_fd[2];  /* pipe used to abort */
};

static JNI::InstMethod<jint(jobject)> jStartScan;
static JNI::InstMethod<jint(jbyteArray, jint, jint)> jInRead;
static JNI::InstMethod<jint()> jGetFd;
static JNI::InstMethod<jint(jobject)> jState;
static JNI::InstMethod<jobject(jlong)> jDefaultAdapter;
static JNI::InstMethod<jobject(jobject, jstring, jint, jboolean)> jOpenSocket;
static JNI::InstMethod<jobject()> jBtSocketInputStream, jBtSocketOutputStream;
static JNI::InstMethod<void()> jBtSocketClose;
static JNI::InstMethod<void(jbyteArray, jint, jint)> jOutWrite;
static JNI::InstMethod<void(jobject)> jCancelScan;
static JNI::InstMethod<void()> jTurnOn;
static jfieldID fdDataId{};

void AndroidBluetoothAdapter::sendSocketStatusMessage(const SocketStatusMessage &msg)
{
	if(write(statusPipe[1], &msg, sizeof(msg)) == -1)
	{
		logErr("error writing BT socket status to pipe");
	}
}

static void JNICALL btScanStatus(JNIEnv*, jobject, jlong btaAddr, jint res)
{
	auto &bta = *reinterpret_cast<BluetoothAdapter*>(btaAddr);
	bta.handleScanStatus(BluetoothScanState(res));
}

void AndroidBluetoothAdapter::handleScanStatus(BluetoothScanState)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	assert(inDetect);
	logMsg("scan complete");
	if(scanCancelled)
		bta.onScanStatus(bta, BluetoothScanState::Cancelled, 0);
	else
		bta.onScanStatus(bta, BluetoothScanState::Complete, 0);
	inDetect = 0;
}

static jboolean JNICALL scanDeviceClass(JNIEnv*, jobject, jlong btaAddr, jint classInt)
{
	auto &bta = *reinterpret_cast<BluetoothAdapter*>(btaAddr);
	return bta.handleScanClass(classInt);
}

bool AndroidBluetoothAdapter::handleScanClass(uint32_t classInt)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	if(scanCancelled)
	{
		logMsg("scan canceled while handling device class");
		return 0;
	}
	logMsg("got class %X", classInt);
	std::array<uint8_t, 3> classByte;
	classByte[2] = classInt >> 16;
	classByte[1] = (classInt >> 8) & 0xff;
	classByte[0] = classInt & 0xff;
	if(!bta.onScanDeviceClass(bta, classByte))
	{
		logMsg("skipping device due to class %X:%X:%X", classByte[0], classByte[1], classByte[2]);
		return 0;
	}
	return 1;
}

static void JNICALL scanDeviceName(JNIEnv* env, jobject, jlong btaAddr, jstring name, jstring addr)
{
	auto &bta = *reinterpret_cast<BluetoothAdapter*>(btaAddr);
	bta.handleScanName(env, name, addr);
}

void AndroidBluetoothAdapter::handleScanName(JNIEnv* env, jstring name, jstring addr)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	if(scanCancelled)
	{
		logMsg("scan canceled while handling device name");
		return;
	}
	const char *nameStr = env->GetStringUTFChars(name, nullptr);
	BluetoothAddr addrByte;
	{
		const char *addrStr = env->GetStringUTFChars(addr, nullptr);
		str2ba(addrStr, &addrByte);
		env->ReleaseStringUTFChars(addr, addrStr);
	}
	logMsg("got name %s", nameStr);
	bta.onScanDeviceName(bta, nameStr, addrByte);
	env->ReleaseStringUTFChars(name, nameStr);
}

static void JNICALL turnOnResult(JNIEnv*, jobject, jlong btaAddr, jboolean success)
{
	auto &bta = *reinterpret_cast<BluetoothAdapter*>(btaAddr);
	bta.handleTurnOnResult(success);
}

void AndroidBluetoothAdapter::handleTurnOnResult(bool success)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	logMsg("bluetooth power on result: %d", int(success));
	if(turnOnD)
	{
		turnOnD(bta, success ? BluetoothState::On : BluetoothState::Error);
		turnOnD = {};
	}
}

bool BluetoothAdapter::openDefault()
{
	if(isOpen())
		return true;

	// setup JNI
	auto env = ctx.mainThreadJniEnv();
	if(!jDefaultAdapter)
	{
		logMsg("JNI setup");
		auto baseActivityCls = (jclass)env->GetObjectClass(ctx.baseActivityObject());
		jDefaultAdapter = {env, baseActivityCls, "btDefaultAdapter", "(J)Landroid/bluetooth/BluetoothAdapter;"};
		jStartScan = {env, baseActivityCls, "btStartScan", "(Landroid/bluetooth/BluetoothAdapter;)I"};
		jCancelScan = {env, baseActivityCls, "btCancelScan", "(Landroid/bluetooth/BluetoothAdapter;)V"};
		jOpenSocket = {env, baseActivityCls, "btOpenSocket", "(Landroid/bluetooth/BluetoothAdapter;Ljava/lang/String;IZ)Landroid/bluetooth/BluetoothSocket;"};
		jState = {env, baseActivityCls, "btState", "(Landroid/bluetooth/BluetoothAdapter;)I"};
		jTurnOn = {env, baseActivityCls, "btTurnOn", "()V"};

		jclass jBluetoothSocketCls = env->FindClass("android/bluetooth/BluetoothSocket");
		assert(jBluetoothSocketCls);
		jBtSocketClose = {env, jBluetoothSocketCls, "close", "()V"};
		jBtSocketInputStream = {env, jBluetoothSocketCls, "getInputStream", "()Ljava/io/InputStream;"};
		jBtSocketOutputStream = {env, jBluetoothSocketCls, "getOutputStream", "()Ljava/io/OutputStream;"};

		jclass jInputStreamCls = env->FindClass("java/io/InputStream");
		assert(jInputStreamCls);
		jInRead = {env, jInputStreamCls, "read", "([BII)I"};

		jclass jOutputStreamCls = env->FindClass("java/io/OutputStream");
		assert(jOutputStreamCls);
		jOutWrite = {env, jOutputStreamCls, "write", "([BII)V"};

		if(ctx.androidSDK() < 17)
		{
			// pre-Android 4.2, int mSocketData is a pointer to an asocket C struct
			fdDataId = env->GetFieldID(jBluetoothSocketCls, "mSocketData", "I");
			if(!fdDataId)
			{
				logWarn("can't find mSocketData member of BluetoothSocket class, not using native FDs");
				env->ExceptionClear();
			}
		}
		else
		{
			// In Android 4.2, use ParcelFileDescriptor mPfd
			fdDataId = env->GetFieldID(jBluetoothSocketCls, "mPfd", "Landroid/os/ParcelFileDescriptor;");
			if(fdDataId)
			{
				auto parcelFileDescriptorCls = env->FindClass("android/os/ParcelFileDescriptor");
				assert(parcelFileDescriptorCls);
				jGetFd = {env, parcelFileDescriptorCls, "getFd", "()I"};
			}
			else
			{
				logWarn("can't find mPfd member of BluetoothSocket class, not using native FDs");
				env->ExceptionClear();
			}
		}

		static JNINativeMethod activityMethods[] =
		{
			{"onBTScanStatus", "(JI)V", (void *)&btScanStatus},
			{"onScanDeviceClass", "(JI)Z", (void *)&scanDeviceClass},
			{"onScanDeviceName", "(JLjava/lang/String;Ljava/lang/String;)V", (void *)&scanDeviceName},
			{"onBTOn", "(JZ)V", (void *)&turnOnResult}
		};

		env->RegisterNatives(baseActivityCls, activityMethods, std::size(activityMethods));
	}

	logMsg("opening default BT adapter");
	adapter = jDefaultAdapter(env, ctx.baseActivityObject(), (jlong)this);
	if(!adapter)
	{
		logErr("error opening adapter");
		return false;
	}
	adapter = env->NewGlobalRef(adapter);
	assert(adapter);
	{
		[[maybe_unused]] int ret = pipe(statusPipe);
		assert(ret == 0);
		ret = ALooper_addFd(EventLoop::forThread().nativeObject(), statusPipe[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
			[](int fd, int events, void* data)
			{
				if(events & pollEventError)
					return 0;
				while(fd_bytesReadable(fd))
				{
					SocketStatusMessage msg;
					if(read(fd, &msg, sizeof(msg)) != sizeof(msg))
					{
						logErr("error reading BT socket status message in pipe");
						return 1;
					}
					logMsg("got bluetooth socket status delegate message");
					auto &bta = *static_cast<BluetoothAdapter*>(data);
					msg.socket->onStatusDelegateMessage(bta, msg.type);
				}
				return 1;
			}, this);
		assert(ret == 1);
	}
	return true;
}

bool BluetoothAdapter::isOpen() const { return adapter; }

void BluetoothAdapter::cancelScan()
{
	scanCancelled = 1;
	jCancelScan(appContext().mainThreadJniEnv(), appContext().baseActivityObject(), adapter);
}

void BluetoothAdapter::close()
{
	if(inDetect)
	{
		cancelScan();
		inDetect = 0;
	}
}

bool BluetoothAdapter::startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName)
{
 	if(!inDetect)
	{
 		logMsg("preparing to start scan");
 		auto doScan =
 			[this](BluetoothAdapter &, State newState)
 			{
 				if(newState != BluetoothState::On)
 				{
 					logMsg("failed to turn on bluetooth");
 					inDetect = 0;
 					onScanStatus(*this, BluetoothScanState::Failed, 0);
 					return;
 				}

 				logMsg("starting scan");
 				if(!jStartScan(appContext().mainThreadJniEnv(), appContext().baseActivityObject(), adapter))
 				{
 					inDetect = 0;
 					logMsg("failed to start scan");
 					onScanStatus(*this, BluetoothScanState::Failed, 0);
 				}
 			};

		scanCancelled = 0;
		inDetect = 1;
		onScanStatus = onResult;
		onScanDeviceClass = onDeviceClass;
		onScanDeviceName = onDeviceName;
		if(state() != BluetoothState::On)
		{
			setActiveState(true, doScan);
		}
		else
		{
			doScan(*this, BluetoothState::On);
		}
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

BluetoothAdapter::State BluetoothAdapter::state()
{
	auto currState = jState(appContext().mainThreadJniEnv(), appContext().baseActivityObject(), adapter);
	switch(currState)
	{
		case 10: return BluetoothState::Off;
		case 12: return BluetoothState::On;
		case 13: return BluetoothState::TurningOff;
		case 11: return BluetoothState::TurningOn;
	}
	logMsg("unknown state: %d", currState);
	return BluetoothState::Off;
}

void BluetoothAdapter::setActiveState(bool on, OnStateChangeDelegate onStateChange)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	if(on)
	{
		auto currState = state();
		if(currState != BluetoothState::On)
		{
			logMsg("radio is off, requesting activation");
			turnOnD = onStateChange;
			jTurnOn(bta.appContext().mainThreadJniEnv(), appContext().baseActivityObject());
		}
		else
		{
			onStateChange(*this, BluetoothState::On);
		}
	}
	else
	{
		onStateChange(*this, BluetoothState::Error);
	}
}


jobject AndroidBluetoothAdapter::openSocket(JNIEnv *env, const char *addrStr, int channel, bool isL2cap)
{
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	return jOpenSocket(env, bta.appContext().baseActivityObject(), adapter, env->NewStringUTF(addrStr), channel, isL2cap ? 1 : 0);
}

bool BluetoothAdapter::isInScan() const { return inDetect; }

bool AndroidBluetoothSocket::readPendingData(int events)
{
	auto &sock = static_cast<BluetoothSocket&>(*this);
	if(events & pollEventError)
	{
		logMsg("socket %d disconnected", nativeFd);
		sock.onStatus(sock, BluetoothSocketState::ReadError);
		return false;
	}
	else if(events & pollEventInput)
	{
		char buff[50];
		//logMsg("at least %d bytes ready on socket %d", fd_bytesReadable(nativeFd), nativeFd);
		while(fd_bytesReadable(nativeFd))
		{
			auto len = read(nativeFd, buff, sizeof buff);
			if(len <= 0) [[unlikely]]
			{
				logMsg("error %d reading packet from socket %d", len == -1 ? errno : 0, nativeFd);
				sock.onStatus(sock, BluetoothSocketState::ReadError);
				return false;
			}
			//logMsg("read %d bytes from socket %d", len, nativeFd);
			if(!sock.onData(buff, len))
				break; // socket was closed
		}
	}
	return true;
}

void AndroidBluetoothSocket::onStatusDelegateMessage(BluetoothAdapter& bta, BluetoothSocketState status)
{
	auto &sock = static_cast<BluetoothSocket&>(*this);
	if(status != BluetoothSocketState::Opened)
	{
		// error
		sock.onStatus(sock, status);
		return;
	}
	if(sock.onStatus(sock, BluetoothSocketState::Opened) == 1)
	{
		if(nativeFd != -1)
		{
			fdSrc = {nativeFd, {.eventLoop = EventLoop::forThread()}, [this](int, int events) { return readPendingData(events); }};
		}
		else
		{
			logMsg("starting read thread");
			IG::makeDetachedThread(
				[this, &bta]()
				{
					if(Config::DEBUG_BUILD)
						logMsg("in read thread %d", gettid());
					auto &sock = static_cast<BluetoothSocket&>(*this);
					JNIEnv *env = ctx.thisThreadJniEnv();
					if(!env)
					{
						logErr("error attaching env to thread");
						// TODO: cleanup
						return;
					}

					auto looper = EventLoop::forThread().nativeObject();
					int dataPipe[2];
					{
						[[maybe_unused]] int ret = pipe(dataPipe);
						assert(ret == 0);
						ret = ALooper_addFd(looper, dataPipe[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
							[](int fd, int events, void* data)
							{
								if(events & pollEventError)
									return 0;
								auto &socket = *((BluetoothSocket*)data);
								while(fd_bytesReadable(fd))
								{
									uint16_t size;
									int ret = read(fd, &size, sizeof(size));
									if(ret != sizeof(size))
									{
										logErr("error reading BT socket data header in pipe, returned %d", ret);
										return 1;
									}
									char data[size];
									ret = read(fd, data, size);
									if(ret != size)
									{
										logErr("error reading BT socket data header in pipe, returned %d", ret);
										return 1;
									}
									socket.onData(&data[0], size);
								}
								return 1;
							}, this);
						assert(ret == 1);
					}

					jbyteArray jData = env->NewByteArray(48);
					jboolean usingArrayCopy;
					jbyte *data = env->GetByteArrayElements(jData, &usingArrayCopy);
					if(usingArrayCopy) // will call GetByteArrayElements each iteration
					{
						env->ReleaseByteArrayElements(jData, data, 0);
						logErr("couldn't get direct array pointer");
					}
					jobject jInput = jBtSocketInputStream(env, socket);
					for(;;)
					{
						int16_t len = jInRead(env, jInput, jData, 0, 48);
						//logMsg("read %d bytes", (int)len);
						if(len <= 0 || env->ExceptionCheck()) [[unlikely]]
						{
							if(isClosing)
								logMsg("input stream %p closing", jInput);
							else
								logMsg("error reading packet from input stream %p", jInput);
							env->ExceptionClear();
							if(!isClosing)
								bta.sendSocketStatusMessage({sock, BluetoothSocketState::ReadError});
							break;
						}
						if(::write(dataPipe[1], &len, sizeof(len)) != sizeof(len))
						{
							logErr("unable to write message header to pipe: %s", strerror(errno));
						}
						if(usingArrayCopy)
							data = env->GetByteArrayElements(jData, nullptr);
						if(::write(dataPipe[1], data, len) != len)
						{
							logErr("unable to write bt data to pipe: %s", strerror(errno));
						}
						if(usingArrayCopy)
							env->ReleaseByteArrayElements(jData, data, JNI_ABORT);
					}
					if(!usingArrayCopy)
						env->ReleaseByteArrayElements(jData, data, 0);
					ALooper_removeFd(looper, dataPipe[0]);
					::close(dataPipe[0]);
					::close(dataPipe[1]);
				}
			);
		}
	}
}

static int nativeFdForSocket(JNIEnv *env, jobject btSocket)
{
	if(jGetFd)
	{
		// ParcelFileDescriptor method
		auto pFd = env->GetObjectField(btSocket, fdDataId);
		if(!pFd)
		{
			logWarn("null ParcelFileDescriptor");
			return -1;
		}
		int fd = jGetFd(env, pFd);
		if(fd < 0)
		{
			logWarn("invalid FD");
			return -1;
		}
		return fd;
	}
	else if(Config::ENV_ANDROID_MIN_SDK < 17 && fdDataId)
	{
		// asocket method
		auto sockPtr = (asocket*)(uintptr_t)env->GetIntField(btSocket, fdDataId);
		if(!sockPtr)
		{
			logWarn("null asocket");
			return -1;
		}
		if(sockPtr->fd < 0)
		{
			logWarn("invalid FD");
			return -1;
		}
		return sockPtr->fd;
	}
	return -1;
}

void AndroidBluetoothSocket::openSocket(BluetoothAdapter &adapter, BluetoothAddr bdaddr, uint32_t channel, bool l2cap)
{
	addrStr = ba2str(bdaddr);
	this->channel = channel;
	isL2cap = l2cap;
	isConnecting = true;
	IG::makeDetachedThread(
		[this, &adapter]()
		{
			logMsg("in connect thread %d", gettid());
			auto &sock = static_cast<BluetoothSocket&>(*this);
			JNIEnv *env = ctx.thisThreadJniEnv();
			if(!env)
			{
				logErr("error attaching env to thread");
				connectSem.release();
				isConnecting = false;
				return;
			}
			socket = adapter.openSocket(env, addrStr.data(), this->channel, isL2cap);
			if(socket)
			{
				logMsg("opened Bluetooth socket %p", socket);
				socket = env->NewGlobalRef(socket);
				int fd = nativeFdForSocket(env, socket);
				if(fd != -1 && fd_isValid(fd))
				{
					logMsg("native FD %d", fd);
					nativeFd = fd;
				}
				else
				{
					outStream = jBtSocketOutputStream(env, socket);
					assert(outStream);
					logMsg("opened output stream %p", outStream);
					outStream = env->NewGlobalRef(outStream);
				}
				adapter.sendSocketStatusMessage({sock, BluetoothSocketState::Opened});
			}
			else
				adapter.sendSocketStatusMessage({sock, BluetoothSocketState::ConnectError});
			connectSem.release();
			isConnecting = false;
		}
	);
}

std::system_error BluetoothSocket::openRfcomm(BluetoothAdapter &adapter, BluetoothAddr bdaddr, uint32_t channel)
{
	logMsg("opening RFCOMM channel %d", channel);
	openSocket(adapter, bdaddr, channel, 0);
	return std::error_code{};
}

std::system_error BluetoothSocket::openL2cap(BluetoothAdapter &adapter, BluetoothAddr bdaddr, uint32_t psm)
{
	logMsg("opening L2CAP psm %d", psm);
	openSocket(adapter, bdaddr, psm, 1);
	return std::error_code{};
}

AndroidBluetoothSocket::~AndroidBluetoothSocket()
{
	close();
}

void AndroidBluetoothSocket::close()
{
	if(isConnecting)
	{
		logMsg("waiting for connect thread to complete before closing socket");
		connectSem.acquire();
	}
	if(socket)
	{
		logMsg("closing socket");
		if(nativeFd != -1)
		{
			fdSrc.detach();
			nativeFd = -1; // BluetoothSocket closes the FD
		}
		isClosing = 1;
		auto env = ctx.thisThreadJniEnv();
		env->DeleteGlobalRef(outStream);
		jBtSocketClose(env, socket);
		env->DeleteGlobalRef(socket);
		socket = nullptr;
	}
}

ssize_t BluetoothSocket::write(const void *data, size_t size)
{
	logMsg("writing %zd bytes", size);
	if(nativeFd != -1)
	{
		return fd_writeAll(nativeFd, data, size);
	}
	else
	{
		auto env = ctx.thisThreadJniEnv();
		jbyteArray jData = env->NewByteArray(size);
		env->SetByteArrayRegion(jData, 0, size, (jbyte *)data);
		jOutWrite(env, outStream, jData, 0, size);
		env->DeleteLocalRef(jData);
		return size;
	}
}

}
