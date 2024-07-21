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

#include <imagine/util/bit.hh>
#include <imagine/util/math.hh>
#include <imagine/util/fd-utils.h>
#include <imagine/fs/FS.hh>
#include <imagine/input/evdev/EvdevInputDevice.hh>
#include <imagine/input/Event.hh>
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/time/Time.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <linux/input.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>

#define DEV_NODE_PATH "/dev/input"
static constexpr uint32_t MAX_STICK_AXES = 6; // 6 possible axes defined in key codes

namespace IG::Input
{

constexpr SystemLogger log{"Evdev"};

static Key toSysKey(Key key)
{
	#define EVDEV_TO_SYSKEY_CASE(keyIdent) case Evdev::keyIdent: return Keycode::keyIdent
	switch(key)
	{
		default: return 0;
		EVDEV_TO_SYSKEY_CASE(UP);
		EVDEV_TO_SYSKEY_CASE(RIGHT);
		EVDEV_TO_SYSKEY_CASE(DOWN);
		EVDEV_TO_SYSKEY_CASE(LEFT);
		EVDEV_TO_SYSKEY_CASE(GAME_A);
		EVDEV_TO_SYSKEY_CASE(GAME_B);
		EVDEV_TO_SYSKEY_CASE(GAME_C);
		EVDEV_TO_SYSKEY_CASE(GAME_X);
		EVDEV_TO_SYSKEY_CASE(GAME_Y);
		EVDEV_TO_SYSKEY_CASE(GAME_Z);
		EVDEV_TO_SYSKEY_CASE(GAME_L1);
		EVDEV_TO_SYSKEY_CASE(GAME_R1);
		EVDEV_TO_SYSKEY_CASE(GAME_L2);
		EVDEV_TO_SYSKEY_CASE(GAME_R2);
		EVDEV_TO_SYSKEY_CASE(GAME_LEFT_THUMB);
		EVDEV_TO_SYSKEY_CASE(GAME_RIGHT_THUMB);
		EVDEV_TO_SYSKEY_CASE(GAME_START);
		EVDEV_TO_SYSKEY_CASE(GAME_SELECT);
		EVDEV_TO_SYSKEY_CASE(GAME_MODE);
		EVDEV_TO_SYSKEY_CASE(GAME_1);
		EVDEV_TO_SYSKEY_CASE(GAME_2);
		EVDEV_TO_SYSKEY_CASE(GAME_3);
		EVDEV_TO_SYSKEY_CASE(GAME_4);
		EVDEV_TO_SYSKEY_CASE(GAME_5);
		EVDEV_TO_SYSKEY_CASE(GAME_6);
		EVDEV_TO_SYSKEY_CASE(GAME_7);
		EVDEV_TO_SYSKEY_CASE(GAME_8);
		EVDEV_TO_SYSKEY_CASE(GAME_9);
		EVDEV_TO_SYSKEY_CASE(GAME_10);
		EVDEV_TO_SYSKEY_CASE(GAME_11);
		EVDEV_TO_SYSKEY_CASE(GAME_12);
		EVDEV_TO_SYSKEY_CASE(GAME_13);
		EVDEV_TO_SYSKEY_CASE(GAME_14);
		EVDEV_TO_SYSKEY_CASE(GAME_15);
		EVDEV_TO_SYSKEY_CASE(GAME_16);

		EVDEV_TO_SYSKEY_CASE(JS1_XAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS1_XAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS1_YAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS1_YAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS2_XAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS2_XAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS2_YAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS2_YAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS3_XAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS3_XAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS3_YAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS3_YAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS_LTRIGGER_AXIS);
		EVDEV_TO_SYSKEY_CASE(JS_RTRIGGER_AXIS);
		EVDEV_TO_SYSKEY_CASE(JS_POV_XAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS_POV_XAXIS_NEG);
		EVDEV_TO_SYSKEY_CASE(JS_POV_YAXIS_POS);
		EVDEV_TO_SYSKEY_CASE(JS_POV_YAXIS_NEG);
	}
	#undef EVDEV_TO_SYSKEY_CASE
}

template <class T, size_t S>
static constexpr bool isBitSetInArray(const T (&arr)[S], unsigned int bit)
{
	auto bits = IG::bitSize<T>;
	return arr[bit / bits] & ((T)1 << (bit % bits));
}

EvdevInputDevice::EvdevInputDevice(int id, UniqueFileDescriptor fd, DeviceTypeFlags typeFlags, std::string name_, uint32_t vendorProductId):
	BaseDevice{id, Map::SYSTEM, typeFlags, std::move(name_)},
	fdSrc{std::move(fd), {.debugLabel = "EvdevInputDevice", .eventLoop = EventLoop::forThread()}, {}}
{
	subtype_ = DeviceSubtype::GENERIC_GAMEPAD;
	updateGamepadSubtype(name_, vendorProductId);
	if(setupJoystickBits())
		typeFlags_.joystick = true;
}

void EvdevInputDevice::processInputEvents(Device &dev, LinuxApplication &app, std::span<const input_event> events)
{
	for(auto &ev : events)
	{
		//log.debug("got event type {}, code {}, value {}", ev.type, ev.code, ev.value);
		auto time = SteadyClockTimePoint{Seconds{ev.time.tv_sec} + Microseconds{ev.time.tv_usec}};
		switch(ev.type)
		{
			case EV_KEY:
			{
				//log.debug("got key event code:{:X} value:{}", ev.code, ev.value);
				auto key = toSysKey(ev.code);
				KeyEvent event{Map::SYSTEM, key, ev.value ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &dev};
				app.dispatchRepeatableKeyInputEvent(event);
				break;
			}
			case EV_ABS:
			{
				auto &evDev = getAs<EvdevInputDevice>(dev);
				auto &axis = evDev.axis;
				auto axisIt = std::ranges::find_if(axis, [&](auto &axis){ return ev.code == (uint8_t)axis.id(); });
				if(axisIt == axis.end())
				{
					log.debug("event from unused axis:{}", ev.code);
					continue;
				}
				auto offset = evDev.axisRangeOffset[std::distance(axis.begin(), axisIt)];
				float val = (ev.value + offset) * axisIt->scale();
				//log.debug("got abs event code {:X}, value {} ({})", ev.code, ev.value, val);
				axisIt->dispatchInputEvent(val, Map::SYSTEM, time, dev, app.mainWindow());
			}
		}
	}
}

bool EvdevInputDevice::setupJoystickBits()
{
	ulong evBit[IG::divRoundUp(EV_MAX, IG::bitSize<ulong>)]{};
	bool isJoystick = (ioctl(fd(), EVIOCGBIT(0, sizeof(evBit)), evBit) >= 0)
		&& isBitSetInArray(evBit, EV_ABS);

	if(!isJoystick)
		return false;

	ulong absBit[IG::divRoundUp(ABS_MAX, IG::bitSize<ulong>)]{};
	if((ioctl(fd(), EVIOCGBIT(EV_ABS, sizeof(absBit)), absBit) < 0))
	{
		log.error("unable to check abs bits");
		return false;
	}

	// check joystick axes
	{
		static constexpr AxisId stickAxes[] { AxisId::X, AxisId::Y, AxisId::Z, AxisId::RX, AxisId::RY, AxisId::RZ,
			AxisId::HAT0X, AxisId::HAT0Y, AxisId::HAT1X, AxisId::HAT1Y, AxisId::HAT2X, AxisId::HAT2Y, AxisId::HAT3X, AxisId::HAT3Y,
			AxisId::LTRIGGER, AxisId::RTRIGGER, AxisId::RUDDER, AxisId::WHEEL };
		for(auto axisId : stickAxes)
		{
			if(!isBitSetInArray(absBit, (int)axisId))
				continue;
			log.info("joystick axis:{}", (int)axisId);
			struct input_absinfo info;
			if(ioctl(fd(), EVIOCGABS((int)axisId), &info) < 0)
			{
				logErr("error getting absinfo");
				continue;
			}
			auto rangeSize = info.maximum - info.minimum;
			float scale = 2.f / rangeSize;
			auto &currAxis = axis.emplace_back(axisId, scale);
			axisRangeOffset[axis.size() - 1] = currAxis.isTrigger() ? 0 : (std::abs(info.minimum) - std::abs(info.maximum)) / 2;
			log.info("min:{} max:{} fuzz:{} flat:{} range offset:{}", info.minimum, info.maximum, info.fuzz, info.flat,
				axisRangeOffset[axis.size() - 1]);
			if(axis.isFull())
			{
				log.info("reached maximum joystick axes");
				break;
			}
		}
	}
	// TODO: check trigger axes
	return true;
}

void EvdevInputDevice::addPollEvent(Device &dev, LinuxApplication &app)
{
	auto &evDev = getAs<EvdevInputDevice>(dev);
	assert(evDev.fd() >= 0);
	evDev.fdSrc.setCallback([&dev, &app](int fd, int pollEvents)
	{
		if(pollEvents & pollEventError) [[unlikely]]
		{
			log.error("error:{} in input fd:{} ({})", errno, fd, dev.name());
			app.removeInputDevice(ApplicationContext{static_cast<Application&>(app)}, dev, true);
			return false;
		}
		else
		{
			struct input_event event[64];
			int len;
			while((len = read(fd, event, sizeof event)) > 0)
			{
				uint32_t events = len / sizeof(struct input_event);
				//logMsg("read %d bytes from input fd %d, %d events", len, this->fd, events);
				processInputEvents(dev, app, {event, events});
			}
			if(len == -1 && errno != EAGAIN)
			{
				log.info("error:{} reading from input fd:{} ({})", errno, fd, dev.name());
				app.removeInputDevice(ApplicationContext{static_cast<Application&>(app)}, dev, true);
				return false;
			}
		}
		return true;
	});
}

static bool devIsGamepad(int fd)
{
	ulong keyBit[IG::divRoundUp(KEY_MAX, IG::bitSize<ulong>)] {0};
	if((ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBit)), keyBit) < 0))
	{
		log.error("unable to check key bits");
		return false;
	}
	for(uint32_t i = BTN_JOYSTICK; i < BTN_DIGI; i++)
	{
		if(isBitSetInArray(keyBit, i))
		{
			log.info("has joystick/gamepad button:{:X}", i);
			return true;
		}
	}
	return false;
}

static bool isEvdevInputDevice(Input::Device &d)
{
	return d.map() == Input::Map::SYSTEM && d.typeFlags().gamepad;
}

static bool processDevNode(LinuxApplication &app, CStringView path, int id, bool notify)
{
	if(access(path, R_OK) != 0)
	{
		logMsg("no access to %s", path.data());
		return false;
	}

	for(const auto &e : app.inputDevices())
	{
		if(isEvdevInputDevice(*e) && e->id() == id)
		{
			logMsg("id %d is already present", id);
			return false;
		}
	}

	auto fd = open(path, O_RDONLY, 0);
	if(fd == -1)
	{
		logMsg("error opening %s", path.data());
		return false;
	}

	logMsg("checking device @ %s", path.data());
	if(!devIsGamepad(fd))
	{
		logMsg("%s isn't a gamepad", path.data());
		close(fd);
		return false;
	}
	std::array<char, 80> nameStr{"Unknown"};
	if(ioctl(fd, EVIOCGNAME(sizeof(nameStr)), nameStr.data()) < 0)
	{
		logWarn("unable to get device name");
	}
	struct input_id devInfo{};
	if(ioctl(fd, EVIOCGID, &devInfo) < 0)
	{
		logWarn("unable to get device info");
	}
	auto vendorProductId = ((devInfo.vendor & 0xFFFF) << 16) | (devInfo.product & 0xFFFF);
	auto evDev = std::make_unique<Device>(std::in_place_type<EvdevInputDevice>, id, fd, DeviceTypeFlags{.gamepad = true}, nameStr.data(), vendorProductId);
	fd_setNonblock(fd, 1);
	EvdevInputDevice::addPollEvent(*evDev, app);
	app.addInputDevice(ApplicationContext{static_cast<Application&>(app)}, std::move(evDev), notify);
	return true;
}

static bool processDevNodeName(CStringView name, uint32_t &id)
{
	// extract id number from "event*" name and get the full path
	if(sscanf(name, "event%u", &id) != 1)
	{
		//logWarn("couldn't extract numeric part of node name: %s", name);
		return false;
	}
	return true;
}

}

namespace IG
{

void LinuxApplication::initEvdev(EventLoop loop)
{
	logMsg("setting up inotify for hotplug");
	{
		int inputDevNotifyFd = inotify_init();
		if(inputDevNotifyFd >= 0)
		{
			inotify_add_watch(inputDevNotifyFd, DEV_NODE_PATH, IN_CREATE | IN_ATTRIB);
			fd_setNonblock(inputDevNotifyFd, 1);
			evdevSrc = {inputDevNotifyFd, {.debugLabel = "Evdev Inotify", .eventLoop = loop},
				[this](int fd, int)
				{
					char event[sizeof(struct inotify_event) + 2048];
					int len;
					while((len = read(fd, event, sizeof event)) > 0)
					{
						//logMsg("read %d bytes from inotify fd %d", len, inputDevNotifyFd);
						auto inotifyEv = (struct inotify_event*)&event[0];
						uint32_t inotifyEvSize = sizeof(struct inotify_event) + inotifyEv->len;
						logMsg("inotify event @%p with size %u, mask 0x%X", inotifyEv, inotifyEvSize, inotifyEv->mask);
						do
						{
							if(inotifyEv->len > 1)
							{
								uint32_t id;
								if(Input::processDevNodeName(inotifyEv->name, id))
								{
									auto path = FS::pathString(DEV_NODE_PATH, inotifyEv->name);
									Input::processDevNode(*this, path, id, true);
								}
							}
							len -= inotifyEvSize;
							if(len)
							{
								inotifyEv = (struct inotify_event*)(((char*)inotifyEv) + inotifyEvSize);
								inotifyEvSize = sizeof(struct inotify_event) + inotifyEv->len;
								logMsg("next inotify event @%p with size %u, mask 0x%X", inotifyEv, inotifyEvSize, inotifyEv->mask);
							}
						} while(len);
					}
					return true;
				}
			};
		}
		else
		{
			logErr("can't create inotify instance, hotplug won't function");
		}
	}

	logMsg("checking device nodes");
	try
	{
		for(auto &entry : FS::directory_iterator{DEV_NODE_PATH})
		{
			auto filename = entry.name();
			if(entry.type() != FS::file_type::character || !filename.contains("event"))
				continue;
			uint32_t id;
			if(!Input::processDevNodeName(filename, id))
				continue;
			auto path = FS::pathString(DEV_NODE_PATH, filename);
			Input::processDevNode(*this, path, id, false);
		}
	}
	catch(...)
	{
		logErr("can't open " DEV_NODE_PATH);
		return;
	}
}

}
