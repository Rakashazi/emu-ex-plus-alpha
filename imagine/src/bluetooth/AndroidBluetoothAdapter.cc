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

static JavaInstMethod<jint> jStartScan, jInRead;
static JavaInstMethod<jobject> jDefaultAdapter, jOpenSocket, jBtSocketInputStream, jBtSocketOutputStream;
static JavaInstMethod<void> jBtSocketClose, jOutWrite, jCancelScan;

// runs in activity thread
static void JNICALL btScanStatus(JNIEnv* env, jobject thiz, jint res)
{
	logMsg("scan complete");
	if(defaultAndroidAdapter.scanCancelled)
		Base::sendBTScanStatusDelegate(AndroidBluetoothAdapter::SCAN_CANCELLED);
	else
		Base::sendBTScanStatusDelegate(AndroidBluetoothAdapter::SCAN_COMPLETE);
	defaultAndroidAdapter.inDetect = 0;
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
		return 0;
	}
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

		static JNINativeMethod activityMethods[] =
		{
		    {"onBTScanStatus", "(I)V", (void *)&btScanStatus},
		    {"onScanDeviceClass", "(I)Z", (void *)&scanDeviceClass},
		    {"onScanDeviceName", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)&scanDeviceName},
		};

		eEnv()->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
	}

	logMsg("opening default BT adapter");
	auto localAdapter = jDefaultAdapter(eEnv(), jBaseActivity);
	if(!localAdapter)
	{
		logErr("error opening adapter");
		return 0;
	}
	logMsg("success %p", localAdapter);
	adapter = eEnv()->NewGlobalRef(localAdapter); // may be accessed by activity thread
	assert(adapter);
	eEnv()->DeleteLocalRef(localAdapter);
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

void* AndroidBluetoothSocket::readThreadFunc(void *arg)
{
	logMsg("in read thread");
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	JNIEnv *jEnv;
	if(Base::jVM->AttachCurrentThread(&jEnv, 0) != 0)
	{
		logErr("error attaching jEnv to thread");
		return 0;
	}
	#endif

	auto &s = *((AndroidBluetoothSocket*)arg);
	jbyteArray jData = jEnv->NewByteArray(48);
	jboolean jDataArrayIsCopy;
	jbyte *data = jEnv->GetByteArrayElements(jData, &jDataArrayIsCopy);
	if(unlikely(jDataArrayIsCopy)) // can't get direct pointer to memory
	{
		jEnv->ReleaseByteArrayElements(jData, data, 0);
		logErr("couldn't get direct array pointer");
		Base::sendBTSocketStatusDelegate(s, STATUS_ERROR);
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
		Base::jVM->DetachCurrentThread();
		#endif
		return nullptr;
	}
	jobject jInput = jBtSocketInputStream(jEnv, s.socket);
	for(;;)
	{
		int len = jInRead(jEnv, jInput, jData, 0, 48);
		//logMsg("read %d bytes", len);
		if(unlikely(len <= 0 || jEnv->ExceptionOccurred()))
		{
			if(s.isClosing)
				logMsg("input stream %p closing", jInput);
			else
				logMsg("error reading packet from input stream %p", jInput);
			jEnv->ExceptionClear();
			if(!s.isClosing)
				Base::sendBTSocketStatusDelegate(s, STATUS_ERROR);
			break;
		}
		Base::sendBTSocketData(s, len, data);
	}

	jEnv->ReleaseByteArrayElements(jData, data, 0);

	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	Base::jVM->DetachCurrentThread();
	#endif

	return nullptr;
}

CallResult AndroidBluetoothSocket::openSocket(BluetoothAddr bdaddr, uint channel, bool l2cap)
{
	char addrStr[18];
	ba2str(&bdaddr, addrStr);
	//jbyteArray jAddr = jEnv->NewByteArray(sizeof(BluetoothAddr));
	//jEnv->SetByteArrayRegion(jAddr, 0, sizeof(BluetoothAddr), (jbyte *)bdaddr.b);
	auto localSocket = jOpenSocket(eEnv(), Base::jBaseActivity,
		AndroidBluetoothAdapter::defaultAdapter()->adapter, eEnv()->NewStringUTF(addrStr), channel, l2cap ? 1 : 0);
	if(localSocket)
	{
		logMsg("opened Bluetooth socket %p", localSocket);
		socket = eEnv()->NewGlobalRef(localSocket); // accessed by readThreadFunc thread
		assert(socket);
		eEnv()->DeleteLocalRef(localSocket);
		outStream = jBtSocketOutputStream(eEnv(), socket);
		logMsg("opened output stream %p", outStream);
		outStream = Base::eNewGlobalRef(outStream);
		if(onStatus.invoke(*this, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		{
			logMsg("starting read thread");
			readThread.create(1, readThreadFunc, this);
		}
		return OK;
	}
	Base::sendBTScanStatusDelegate(BluetoothAdapter::SOCKET_OPEN_FAILED);
	return IO_ERROR;
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
	if(socket)
	{
		logMsg("closing socket");
		isClosing = 1;
		Base::eDeleteGlobalRef(outStream);
		jBtSocketClose(eEnv(), socket);
		eEnv()->DeleteGlobalRef(socket);
		socket = nullptr;
	}
}

CallResult AndroidBluetoothSocket::write(const void *data, size_t size)
{
	logMsg("writing %d bytes", size);
	jbyteArray jData = eEnv()->NewByteArray(size);
	eEnv()->SetByteArrayRegion(jData, 0, size, (jbyte *)data);
	jOutWrite(eEnv(), outStream, jData, 0, size);
	eEnv()->DeleteLocalRef(jData);
	return OK;
}
