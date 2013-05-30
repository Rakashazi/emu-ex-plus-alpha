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

#define thisModuleName "input:evdev"
#include <linux/input.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <util/cLang.h>
#include <util/bits.h>
#include <util/fd-utils.h>
#include <fs/sys.hh>
#include <base/Base.hh>
#include <input/Input.hh>
#include <input/AxisKeyEmu.hh>
#include <input/evdev/evdev.hh>
#include <util/collection/DLList.hh>

#define DEV_NODE_PATH "/dev/input"
static const uint MAX_STICK_AXES = 6; // 6 possible axes defined in key codes

static int dirFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_FILE && strstr(name, "event");
}

namespace Input
{

const char *evdevButtonName(Key b)
{
	switch(b)
	{
		case Evdev::UP: return "Up";
		case Evdev::RIGHT: return "Right";
		case Evdev::DOWN: return "Down";
		case Evdev::LEFT: return "Left";
		case Evdev::GAME_A: return "A";
		case Evdev::GAME_B: return "B";
		case Evdev::GAME_C: return "C";
		case Evdev::GAME_X: return "X";
		case Evdev::GAME_Y: return "Y";
		case Evdev::GAME_Z: return "Z";
		case Evdev::GAME_L1: return "L1";
		case Evdev::GAME_R1: return "R1";
		case Evdev::GAME_L2: return "L2";
		case Evdev::GAME_R2: return "R2";
		case Evdev::GAME_LEFT_THUMB: return "L-Thumb";
		case Evdev::GAME_RIGHT_THUMB: return "R-Thumb";
		case Evdev::GAME_START: return "Start";
		case Evdev::GAME_SELECT: return "Select";
		case Evdev::GAME_MODE: return "Mode";
		case Evdev::GAME_1: return "G1";
		case Evdev::GAME_2: return "G2";
		case Evdev::GAME_3: return "G3";
		case Evdev::GAME_4: return "G4";
		case Evdev::GAME_5: return "G5";
		case Evdev::GAME_6: return "G6";
		case Evdev::GAME_7: return "G7";
		case Evdev::GAME_8: return "G8";
		case Evdev::GAME_9: return "G9";
		case Evdev::GAME_10: return "G10";
		case Evdev::GAME_11: return "G11";
		case Evdev::GAME_12: return "G12";
		case Evdev::GAME_13: return "G13";
		case Evdev::GAME_14: return "G14";
		case Evdev::GAME_15: return "G15";
		case Evdev::GAME_16: return "G16";

		case Evdev::JS1_XAXIS_POS: return "X Axis+";
		case Evdev::JS1_XAXIS_NEG: return "X Axis-";
		case Evdev::JS1_YAXIS_POS: return "Y Axis+";
		case Evdev::JS1_YAXIS_NEG: return "Y Axis-";
		case Evdev::JS2_XAXIS_POS: return "X Axis+ 2";
		case Evdev::JS2_XAXIS_NEG: return "X Axis- 2";
		case Evdev::JS2_YAXIS_POS: return "Y Axis+ 2";
		case Evdev::JS2_YAXIS_NEG: return "Y Axis- 2";
		case Evdev::JS3_XAXIS_POS: return "X Axis+ 3";
		case Evdev::JS3_XAXIS_NEG: return "X Axis- 3";
		case Evdev::JS3_YAXIS_POS: return "Y Axis+ 3";
		case Evdev::JS3_YAXIS_NEG: return "Y Axis- 3";
		case Evdev::JS_LTRIGGER_AXIS: return "L Trigger";
		case Evdev::JS_RTRIGGER_AXIS: return "R Trigger";
	}
	return "Unknown";
}

/*static uint absAxisToKeycode(int axis)
{
	switch(axis)
	{
		case ABS_X: return Evdev::JS1_XAXIS_POS;
		case ABS_Y: return Evdev::JS1_YAXIS_POS;
		case ABS_Z: return Evdev::JS2_XAXIS_POS;
		case ABS_RZ: return Evdev::JS2_YAXIS_POS;
	}
	return Evdev::JS3_YAXIS_POS;
}*/

static void removeFromSystem(int fd);

struct EvdevInputDevice
{
	constexpr EvdevInputDevice() {}
	EvdevInputDevice(int id, int fd): id{id}, fd{fd},
		pollEvDel
		{
			[this](int pollEvents)
			{
				if(unlikely(pollEvents & Base::POLLEV_ERR))
				{
					logMsg("error %d in input fd %d (%s)", errno, this->fd, name);
					removeFromSystem(this->fd);
					return 0;
				}
				else
				{
					struct input_event event[64];
					int len;
					while((len = read(this->fd, event, sizeof event)) > 0)
					{
						uint events = len / sizeof(struct input_event);
						//logMsg("read %d bytes from input fd %d, %d events", len, this->fd, events);
						processInputEvents(event, events);
					}
					if(len == -1 && errno != EAGAIN)
					{
						logMsg("error %d reading from input fd %d (%s)", errno, this->fd, name);
						removeFromSystem(this->fd);
						return 0;
					}
				}
				return 1;
			}
		}
	{}
	Device *dev = nullptr;
	int id = 0;
	int fd = -1;
	struct Axis
	{
		AxisKeyEmu<int> keyEmu;
		bool active = 0;
	} axis[ABS_HAT3Y];
	Base::PollEventDelegate pollEvDel;
	char name[80] {0};

	void processInputEvents(input_event *event, uint events)
	{
		iterateTimes(events, i)
		{
			auto &ev = event[i];
			//logMsg("got event type %d, code %d, value %d", ev.type, ev.code, ev.value);
			switch(event[i].type)
			{
				bcase EV_KEY:
				{
					logMsg("got key event code 0x%X, value %d", ev.code, ev.value);
					Input::onInputEvent(Event(dev->devId, Event::MAP_EVDEV, ev.code, ev.value ? PUSHED : RELEASED, 0, dev));
				}
				bcase EV_ABS:
				{
					if(ev.code >= sizeofArray(axis) || !axis[ev.code].active)
					{
						continue; // out of range or inactive
					}
					//logMsg("got abs event code 0x%X, value %d", ev.code, ev.value);
					axis[ev.code].keyEmu.dispatch(ev.value, dev->devId, Event::MAP_EVDEV, *dev);
				}
			}
		}
	}

	bool setupJoystickBits()
	{
		ulong evBit[Bits::elemsToHold<ulong>(EV_MAX)] {0};
		bool isJoystick = (ioctl(fd, EVIOCGBIT(0, sizeof(evBit)), evBit) >= 0)
			&& Bits::isSetInArray(evBit, EV_ABS);

		if(!isJoystick)
			return false;

		ulong absBit[Bits::elemsToHold<ulong>(ABS_MAX)] {0};
		if((ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBit)), absBit) < 0))
		{
			logErr("unable to check abs bits");
			return false;
		}

		// check joystick axes
		{
			uint keycodeIdx = 0;
			Key axisKeycode[] =
			{
				Evdev::JS1_XAXIS_NEG, Evdev::JS1_XAXIS_POS,
				Evdev::JS1_YAXIS_NEG, Evdev::JS1_YAXIS_POS,
				Evdev::JS2_XAXIS_NEG, Evdev::JS2_XAXIS_POS,
				Evdev::JS2_YAXIS_NEG, Evdev::JS2_YAXIS_POS,
				Evdev::JS3_XAXIS_NEG, Evdev::JS3_XAXIS_POS,
				Evdev::JS3_YAXIS_NEG, Evdev::JS3_YAXIS_POS,
			};
			const uint8 stickAxes[] { ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ,
				ABS_HAT0X, ABS_HAT0Y, ABS_HAT1X, ABS_HAT1Y, ABS_HAT2X, ABS_HAT2Y, ABS_HAT3X, ABS_HAT3Y,
				ABS_RUDDER, ABS_WHEEL };
			int axes = 0;
			for(auto axisId : stickAxes)
			{
				if(!Bits::isSetInArray(absBit, axisId))
					continue;
				logMsg("joystick axis: %d", axisId);
				struct input_absinfo info;
				if(ioctl(fd, EVIOCGABS(axisId), &info) < 0)
				{
					logErr("error getting absinfo");
					continue;
				}
				logMsg("min: %d max: %d fuzz: %d flat: %d", info.minimum, info.maximum, info.fuzz, info.flat);
				int size = (info.maximum - info.minimum) + 1;
				axis[axisId].keyEmu = {(int)(info.minimum + size/4.), (int)(info.maximum - size/4.), axisKeycode[keycodeIdx], axisKeycode[keycodeIdx+1]};
				axis[axisId].active = 1;
				axes++;
				keycodeIdx += 2; // move to the next +/- axis keycode pair
				if(axes == sizeofArray(axisKeycode)/2)
				{
					logMsg("reached maximum joystick axes");
					break;
				}
			}
		}
		// TODO: check trigger axes
		return true;
	}

	void addPollEvent()
	{
		assert(fd >= 0);
		Base::addPollEvent(fd, pollEvDel, Base::POLLEV_IN);
	}

	void close()
	{
		Base::removePollEvent(fd);
		::close(fd);
		removeDevice(*dev);
		onInputDevChange((DeviceChange){ 0, Event::MAP_KEYBOARD, DeviceChange::REMOVED });
	}
};

static StaticDLList<EvdevInputDevice, 16> evDevice;

static void removeFromSystem(int fd)
{
	forEachInDLList(&evDevice, e)
	{
		if(e.fd == fd)
		{
			e.close();
			e_it.removeElem();
			return;
		}
	}
}

static bool devIsGamepad(int fd)
{
	ulong keyBit[Bits::elemsToHold<ulong>(KEY_MAX)] {0};

	if((ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBit)), keyBit) < 0))
	{
		logErr("unable to check key bits");
		return false;
	}

	bool isGamepad = false;
	for(uint i = BTN_JOYSTICK; i < BTN_DIGI; i++)
	{
		if(Bits::isSetInArray(keyBit, i))
		{
			logMsg("has joystick/gamepad button: 0x%X", i);
			//isGamepad = true;
			return true;
		}
	}

	return isGamepad;
}

static bool processDevNode(const char *path, int id, bool notify)
{
	if(access(path, R_OK) != 0)
	{
		logMsg("no access to %s", path);
		return false;
	}

	forEachInDLList(&evDevice, e)
	{
		if(e.id == id)
		{
			logMsg("id %d is already present", id);
			return false;
		}
	}

	auto fd = open(path, O_RDONLY, 0);
	if(fd == -1)
	{
		logMsg("error opening %s", path);
		return false;
	}

	logMsg("checking device @ %s", path);
	if(!devIsGamepad(fd))
	{
		logMsg("%s isn't a gamepad", path);
		close(fd);
		return false;
	}

	evDevice.emplace_back(id, fd);
	auto &evDev = evDevice.back();
	if(ioctl(fd, EVIOCGNAME(sizeof(evDev.name)), evDev.name) < 0)
	{
		logWarn("unable to get device name");
		string_copy(evDev.name, "Unknown");
	}
	bool isJoystick = evDev.setupJoystickBits();

	fd_setNonblock(fd, 1);
	evDev.addPollEvent();

	uint devId = 0;
	forEachInDLList(&devList, e)
	{
		if(e.map() != Event::MAP_EVDEV)
			continue;
		if(string_equal(e.name(), evDev.name) && e.devId == devId)
			devId++;
	}

	uint type = Device::TYPE_BIT_GAMEPAD;// | (isJoystick ? Device::TYPE_BIT_JOYSTICK : 0);
	addDevice(Device{devId, Event::MAP_EVDEV, type, evDev.name});
	evDev.dev = devList.last();
	if(notify)
		onInputDevChange((DeviceChange){ 0, Event::MAP_KEYBOARD, DeviceChange::ADDED });

	return true;
}

static bool processDevNodeName(const char *name, FsSys::cPath &path, uint &id)
{
	// extract id number from "event*" name and get the full path
	if(sscanf(name, "event%u", &id) != 1)
	{
		//logWarn("couldn't extract numeric part of node name: %s", name);
		return false;
	}
	string_printf(path, DEV_NODE_PATH "/%s", name);
	return true;
}

static int inputDevNotifyFd = -1;
static Base::PollEventDelegate evdevPoll =
	[](int)
	{
		uchar event[sizeof(struct inotify_event) + 2048];
		int len;
		while((len = read(inputDevNotifyFd, event, sizeof event)) > 0)
		{
			//logMsg("read %d bytes from inotify fd %d", len, inputDevNotifyFd);
			auto inotifyEv = (struct inotify_event*)&event[0];
			uint inotifyEvSize = sizeof(struct inotify_event) + inotifyEv->len;
			logMsg("inotify event @%p with size %u, mask 0x%X", inotifyEv, inotifyEvSize, inotifyEv->mask);
			do
			{
				if(inotifyEv->len > 1)
				{
					uint id;
					FsSys::cPath path;
					if(processDevNodeName(inotifyEv->name, path, id))
					{
						processDevNode(path, id, true);
					}
				}
				len -= inotifyEvSize;
				if(len)
				{
					inotifyEv = (struct inotify_event*)(((uchar*)inotifyEv) + inotifyEvSize);
					inotifyEvSize = sizeof(struct inotify_event) + inotifyEv->len;
					logMsg("next inotify event @%p with size %u, mask 0x%X", inotifyEv, inotifyEvSize, inotifyEv->mask);
				}
			} while(len);
		}
		return 1;
	};

void initEvdev()
{
	logMsg("setting up inotify for hotplug");
	{
		inputDevNotifyFd = inotify_init();
		if(inputDevNotifyFd >= 0)
		{
			auto watch = inotify_add_watch(inputDevNotifyFd, DEV_NODE_PATH, IN_CREATE | IN_ATTRIB);
			fd_setNonblock(inputDevNotifyFd, 1);
			Base::addPollEvent(inputDevNotifyFd, evdevPoll);
		}
		else
		{
			logErr("can't create inotify instance, hotplug won't function");
		}
	}

	logMsg("checking device nodes");
	FsSys f;
	if(f.openDir(DEV_NODE_PATH, 0, dirFsFilter) != OK)
	{
		logErr("can't open " DEV_NODE_PATH);
		return;
	}

	iterateTimes(f.numEntries(), i)
	{
		if(evDevice.isFull())
		{
			logMsg("device list is full");
			break;
		}
		auto filename = f.entryFilename(i);
		uint id;
		FsSys::cPath path;
		if(!processDevNodeName(filename, path, id))
			continue;
		processDevNode(path, id, false);
	}
}

}
