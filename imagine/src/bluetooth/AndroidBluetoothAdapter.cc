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

#define thisModuleName "android-bt"
#include "AndroidBluetoothAdapter.hh"
#include <util/fd-utils.h>
#include <errno.h>
#include <ctype.h>
#include <util/collection/DLList.hh>
#include <base/android/private.hh>
#include "utils.hh"

using namespace Base;

static AndroidBluetoothAdapter defaultAndroidAdapter;

static JavaInstMethod<jint> jStartScan, jInRead, jGetFd;
static JavaInstMethod<jobject> jDefaultAdapter, jOpenSocket, jBtSocketInputStream, jBtSocketOutputStream;
static JavaInstMethod<void> jBtSocketClose, jOutWrite, jCancelScan;
static jfieldID fdDataId = nullptr;

// runs in activity thread
static void JNICALL btScanStatus(JNIEnv* env, jobject thiz, jint res)
{
	logMsg("scan complete");
	if(defaultAndroidAdapter.scanCancelled)
		defaultAndroidAdapter.statusDelegate().invoke(BluetoothAdapter::SCAN_CANCELLED, 0);
	else
		defaultAndroidAdapter.statusDelegate().invoke(BluetoothAdapter::SCAN_COMPLETE, 0);
	defaultAndroidAdapter.inDetect = 0;
	postDrawWindowIfNeeded();
}

// runs in activity thread
static jboolean JNICALL scanDeviceClass(JNIEnv* env, jobject thiz, jint classInt)
{
	if(defaultAndroidAdapter.scanCancelled)
	{
		logMsg("scan cancelled with handling device class");
		return 0;
	}
	logMsg("got class %X", classInt);
	uchar classByte[3];
	classByte[2] = classInt >> 16;
	classByte[1] = (classInt >> 8) & 0xff;
	classByte[0] = classInt & 0xff;
	if(!defaultAndroidAdapter.scanDeviceClassDelegate().invoke(classByte))
	{
		logMsg("skipping device due to class %X:%X:%X", classByte[0], classByte[1], classByte[2]);
		postDrawWindowIfNeeded();
		return 0;
	}
	postDrawWindowIfNeeded();
	return 1;
}

// runs in activity thread
static void JNICALL scanDeviceName(JNIEnv* env, jobject thiz, jstring name, jstring addr)
{
	if(defaultAndroidAdapter.scanCancelled)
	{
		logMsg("scan cancelled with handling device name");
		return;
	}
	const char *nameStr = env->GetStringUTFChars(name, 0);
	BluetoothAddr addrByte;
	{
		const char *addrStr = env->GetStringUTFChars(addr, 0);
		str2ba(addrStr, &addrByte);
		env->ReleaseStringUTFChars(addr, addrStr);
	}
	defaultAndroidAdapter.scanDeviceNameDelegate().invoke(nameStr, addrByte);
	env->ReleaseStringUTFChars(name, nameStr);
	postDrawWindowIfNeeded();
}

bool AndroidBluetoothAdapter::openDefault()
{
	if(adapter)
		return 1;

	// setup JNI
	if(jDefaultAdapter.m == 0)
	{
		logMsg("JNI setup");
		jDefaultAdapter.setup(eEnv(), jBaseActivityCls, "btDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
		jStartScan.setup(eEnv(), jBaseActivityCls, "btStartScan", "(Landroid/bluetooth/BluetoothAdapter;)I");
		jCancelScan.setup(eEnv(), jBaseActivityCls, "btCancelScan", "(Landroid/bluetooth/BluetoothAdapter;)V");
		jOpenSocket.setup(eEnv(), jBaseActivityCls, "btOpenSocket", "(Landroid/bluetooth/BluetoothAdapter;Ljava/lang/String;IZ)Landroid/bluetooth/BluetoothSocket;");

		jclass jBluetoothSocketCls = eEnv()->FindClass("android/bluetooth/BluetoothSocket");
		assert(jBluetoothSocketCls);
		jBtSocketClose.setup(eEnv(), jBluetoothSocketCls, "close", "()V");
		jBtSocketInputStream.setup(eEnv(), jBluetoothSocketCls, "getInputStream", "()Ljava/io/InputStream;");
		jBtSocketOutputStream.setup(eEnv(), jBluetoothSocketCls, "getOutputStream", "()Ljava/io/OutputStream;");

		jclass jInputStreamCls = eEnv()->FindClass("java/io/InputStream");
		assert(jInputStreamCls);
		jInRead.setup(eEnv(), jInputStreamCls, "read", "([BII)I");

		jclass jOutputStreamCls = eEnv()->FindClass("java/io/OutputStream");
		assert(jOutputStreamCls);
		jOutWrite.setup(eEnv(), jOutputStreamCls, "write", "([BII)V");

		if(Base::androidSDK() < 17)
		{
			// pre-Android 4.2, int mSocketData is a pointer to an asocket C struct
			fdDataId = eEnv()->GetFieldID(jBluetoothSocketCls, "mSocketData", "I");
			if(!fdDataId)
			{
				logWarn("can't find mSocketData member of BluetoothSocket class, not using native FDs");
			}
		}
		else
		{
			// In Android 4.2, use ParcelFileDescriptor mPfd
			fdDataId = eEnv()->GetFieldID(jBluetoothSocketCls, "mPfd", "Landroid/os/ParcelFileDescriptor;");
			if(fdDataId)
			{
				auto parcelFileDescriptorCls = eEnv()->FindClass("android/os/ParcelFileDescriptor");
				assert(parcelFileDescriptorCls);
				jGetFd.setup(eEnv(), parcelFileDescriptorCls, "getFd", "()I");
			}
			else
			{
				logWarn("can't find mPfd member of BluetoothSocket class, not using native FDs");
			}
		}

		static JNINativeMethod activityMethods[] =
		{
		    {"onBTScanStatus", "(I)V", (void *)&btScanStatus},
		    {"onScanDeviceClass", "(I)Z", (void *)&scanDeviceClass},
		    {"onScanDeviceName", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)&scanDeviceName},
		};

		eEnv()->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
	}

	logMsg("opening default BT adapter");
	adapter = jDefaultAdapter(eEnv(), jBaseActivity);
	if(!adapter)
	{
		logErr("error opening adapter");
		return 0;
	}
	logMsg("success %p", adapter);
	adapter = eEnv()->NewGlobalRef(adapter);
	assert(adapter);
	return 1;
}

void AndroidBluetoothAdapter::cancelScan()
{
	scanCancelled = 1;
	jCancelScan(eEnv(), jBaseActivity, adapter);
}

void AndroidBluetoothAdapter::close()
{
	if(inDetect)
	{
		cancelScan();
		inDetect = 0;
	}
}

AndroidBluetoothAdapter *AndroidBluetoothAdapter::defaultAdapter()
{
	if(defaultAndroidAdapter.openDefault())
		return &defaultAndroidAdapter;
	else
		return nullptr;
}

bool AndroidBluetoothAdapter::startScan()
{
	if(!inDetect)
	{
		scanCancelled = 0;
		inDetect = 1;
		logMsg("starting scan");
		if(!jStartScan(eEnv(), jBaseActivity, adapter))
		{
			inDetect = 0;
			logMsg("failed to start scan");
			return 0;
		}
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

void AndroidBluetoothAdapter::constructSocket(void *mem)
{
	new(mem) AndroidBluetoothSocket();
}

int AndroidBluetoothSocket::readPendingData(int events)
{
	if(events & Base::POLLEV_ERR)
	{
		logMsg("socket %d disconnected", nativeFd);
		onStatus.invoke(*this, STATUS_ERROR);
		return 0;
	}
	else if(events & Base::POLLEV_IN)
	{
		uchar buff[48];
		//logMsg("at least %d bytes ready on socket %d", fd_bytesReadable(nativeFd), nativeFd);
		while(fd_bytesReadable(nativeFd))
		{
			auto len = read(nativeFd, buff, sizeof buff);
			if(unlikely(len <= 0))
			{
				logMsg("error %d reading packet from socket %d", len == -1 ? errno : 0, nativeFd);
				onStatus.invoke(*this, STATUS_ERROR);
				return 0;
			}
			//logMsg("read %d bytes from socket %d", len, nativeFd);
			if(!onDataEvent.invoke(buff, len))
				break; // socket was closed
		}
	}
	return 1;
}

ptrsize AndroidBluetoothSocket::readThreadFunc(ThreadPThread &thread)
{
	logMsg("in read thread %d", gettid());
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	JNIEnv *jEnv;
	if(Base::jVM->AttachCurrentThread(&jEnv, 0) != 0)
	{
		logErr("error attaching jEnv to thread");
		return 0;
	}
	#endif

	jbyteArray jData = jEnv->NewByteArray(48);
	jboolean jDataArrayIsCopy;
	jbyte *data = jEnv->GetByteArrayElements(jData, &jDataArrayIsCopy);
	if(unlikely(jDataArrayIsCopy)) // can't get direct pointer to memory
	{
		jEnv->ReleaseByteArrayElements(jData, data, 0);
		logErr("couldn't get direct array pointer");
		Base::sendBTSocketStatusDelegate(*this, STATUS_ERROR);
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
		Base::jVM->DetachCurrentThread();
		#endif
		return 0;
	}
	jobject jInput = jBtSocketInputStream(jEnv, socket);
	for(;;)
	{
		int len = jInRead(jEnv, jInput, jData, 0, 48);
		logMsg("read %d bytes", len);
		if(unlikely(len <= 0 || jEnv->ExceptionOccurred()))
		{
			if(isClosing)
				logMsg("input stream %p closing", jInput);
			else
				logMsg("error reading packet from input stream %p", jInput);
			jEnv->ExceptionClear();
			if(!isClosing)
				Base::sendBTSocketStatusDelegate(*this, STATUS_ERROR);
			break;
		}
		Base::sendBTSocketData(*this, len, data);
	}

	jEnv->ReleaseByteArrayElements(jData, data, 0);

	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	Base::jVM->DetachCurrentThread();
	#endif

	return 0;
}

void AndroidBluetoothSocket::onStatusDelegateMessage(int status)
{
	assert(status == STATUS_OPENED);
	if(onStatus.invoke(*this, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
	{
		if(nativeFd != -1)
		{
			Base::addPollEvent(nativeFd, pollEvDel, Base::POLLEV_IN);
		}
		else
		{
			logMsg("starting read thread");
			readThread.create(1, ThreadPThread::EntryDelegate::create<template_mfunc(AndroidBluetoothSocket, readThreadFunc)>(this));
		}
	}
}

struct asocket
{
	int fd;           /* primary socket fd */
	int abort_fd[2];  /* pipe used to abort */
};

static int nativeFdForSocket(JNIEnv *env, jobject btSocket)
{
	if(jGetFd.m != 0)
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
	else
	{
		// asocket method
		auto sockPtr = (asocket*)env->GetIntField(btSocket, fdDataId);
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
}

ptrsize AndroidBluetoothSocket::connectThreadFunc(ThreadPThread &thread)
{
	logMsg("in connect thread %d", gettid());
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	JNIEnv *jEnv;
	if(Base::jVM->AttachCurrentThread(&jEnv, 0) != 0)
	{
		logErr("error attaching jEnv to thread");
		return 0;
	}
	#endif

	socket = jOpenSocket(jEnv, Base::jBaseActivity,
			AndroidBluetoothAdapter::defaultAdapter()->adapter, jEnv->NewStringUTF(addrStr), channel, isL2cap ? 1 : 0);
	if(socket)
	{
		logMsg("opened Bluetooth socket %p", socket);
		socket = jEnv->NewGlobalRef(socket); // accessed by readThreadFunc thread
		int fd = nativeFdForSocket(jEnv, socket);
		if(fd != -1)
		{
			logMsg("native FD %d", fd);
			nativeFd = fd;
		}
		else
		{
			outStream = jBtSocketOutputStream(jEnv, socket);
			assert(outStream);
			logMsg("opened output stream %p", outStream);
			outStream = jEnv->NewGlobalRef(outStream);
		}
		Base::sendBTSocketStatusDelegate(*this, STATUS_OPENED);
	}
	else
		Base::sendBTScanStatusDelegate(BluetoothAdapter::SOCKET_OPEN_FAILED);

	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	Base::jVM->DetachCurrentThread();
	#endif
	return 0;
}

CallResult AndroidBluetoothSocket::openSocket(BluetoothAddr bdaddr, uint channel, bool l2cap)
{
	ba2str(&bdaddr, addrStr);
	var_selfs(channel);
	isL2cap = l2cap;
	connectThread.create(0, ThreadPThread::EntryDelegate::create<template_mfunc(AndroidBluetoothSocket, connectThreadFunc)>(this));
	return OK;
}

CallResult AndroidBluetoothSocket::openRfcomm(BluetoothAddr bdaddr, uint channel)
{
	logMsg("opening RFCOMM channel %d", channel);
	return openSocket(bdaddr, channel, 0);
}

CallResult AndroidBluetoothSocket::openL2cap(BluetoothAddr bdaddr, uint psm)
{
	logMsg("opening L2CAP psm %d", psm);
	return openSocket(bdaddr, psm, 1);
}

void AndroidBluetoothSocket::close()
{
	if(connectThread.running)
	{
		logMsg("waiting for connect thread to complete before closing socket");
		connectThread.join();
	}
	if(socket)
	{
		logMsg("closing socket");
		if(nativeFd != -1)
		{
			Base::removePollEvent(nativeFd);
			nativeFd = -1; // BluetoothSocket closes the FD
		}
		isClosing = 1;
		eEnv()->DeleteGlobalRef(outStream);
		jBtSocketClose(eEnv(), socket);
		eEnv()->DeleteGlobalRef(socket);
		socket = nullptr;
	}
}

CallResult AndroidBluetoothSocket::write(const void *data, size_t size)
{
	logMsg("writing %d bytes", size);
	if(nativeFd != -1)
	{
		if(fd_writeAll(nativeFd, data, size) != (ssize_t)size)
		{
			return IO_ERROR;
		}
	}
	else
	{
		jbyteArray jData = eEnv()->NewByteArray(size);
		eEnv()->SetByteArrayRegion(jData, 0, size, (jbyte *)data);
		jOutWrite(eEnv(), outStream, jData, 0, size);
		eEnv()->DeleteLocalRef(jData);
	}
	return OK;
}
