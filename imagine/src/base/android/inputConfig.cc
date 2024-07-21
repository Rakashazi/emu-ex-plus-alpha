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

#include <imagine/base/Timer.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/bit.hh>
#include <imagine/base/android/AndroidInputDevice.hh>
#include <android/configuration.h>
#include <android/input.h>
#include <sys/inotify.h>
#include <optional>

[[maybe_unused]] static float (*AMotionEvent_getAxisValueFunc)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index){};
[[maybe_unused]] static float (*AMotionEvent_getButtonStateFunc)(const AInputEvent *motion_event){};

#if ANDROID_MIN_API == 9
CLINK float AMotionEvent_getAxisValue(const AInputEvent* motion_event, int32_t axis, size_t pointer_index)
{
	assumeExpr(AMotionEvent_getAxisValueFunc);
	return AMotionEvent_getAxisValueFunc(motion_event, axis, pointer_index);
}

CLINK int32_t AMotionEvent_getButtonState(const AInputEvent *motion_event)
{
	if(!AMotionEvent_getButtonStateFunc)
		return IG::Input::Pointer::LBUTTON;
	return AMotionEvent_getButtonStateFunc(motion_event);
}
#endif

namespace IG::Input
{

constexpr SystemLogger log{"InputConfig"};

// Note: values must remain in sync with Java code
constexpr inline int DEVICE_ADDED = 0;
constexpr inline int DEVICE_CHANGED = 1;
constexpr inline int DEVICE_REMOVED = 2;

static std::unique_ptr<Input::Device> makeGenericKeyDevice()
{
	return std::make_unique<Input::Device>(std::in_place_type<Input::AndroidInputDevice>, -1, virtualDeviceFlags, "Key Input (All Devices)");
}

// NAVHIDDEN_* mirrors KEYSHIDDEN_*

static const char *hardKeyboardNavStateToStr(int state)
{
	switch(state)
	{
		case ACONFIGURATION_KEYSHIDDEN_ANY: return "undefined";
		case ACONFIGURATION_KEYSHIDDEN_NO: return "no";
		case ACONFIGURATION_KEYSHIDDEN_YES: return "yes";
		case ACONFIGURATION_KEYSHIDDEN_SOFT:  return "soft";
		default: return "unknown";
	}
}

static const char *inputDeviceKeyboardTypeToStr(int type)
{
	switch(type)
	{
		case AINPUT_KEYBOARD_TYPE_NONE: return "None";
		case AINPUT_KEYBOARD_TYPE_NON_ALPHABETIC: return "Non-Alphabetic";
		case AINPUT_KEYBOARD_TYPE_ALPHABETIC: return "Alphabetic";
	}
	return "Unknown";
}

AndroidInputDevice::AndroidInputDevice(int osId, DeviceTypeFlags typeFlags, std::string name):
	BaseDevice{osId, Map::SYSTEM, typeFlags, std::move(name)} {}

AndroidInputDevice::AndroidInputDevice(int osId, int src, std::string devName,
	int kbType, AxisFlags jsAxisFlags, uint32_t vendorProductId, bool isPowerButton):
	BaseDevice{osId, Map::SYSTEM, {.miscKeys = true}, std::move(devName)}
{
	typeFlags_.virtualInput = osId == -1;
	typeFlags_.powerButton = isPowerButton;
	if(src & AINPUT_SOURCE_CLASS_POINTER)
	{
		typeFlags_ .touchscreen = isBitMaskSet(src, AINPUT_SOURCE_TOUCHSCREEN);
		typeFlags_ .mouse = isBitMaskSet(src, AINPUT_SOURCE_MOUSE);
	}
	auto &name = name_;
	if(isBitMaskSet(src, AINPUT_SOURCE_GAMEPAD))
	{
		bool isGamepad = true;
		if(Config::MACHINE_IS_GENERIC_ARMV7 && name.contains("-zeus"))
		{
			log.info("detected Xperia Play gamepad");
			subtype_ = Subtype::XPERIA_PLAY;
		}
		else if((Config::MACHINE_IS_GENERIC_ARMV7 && name == "sii9234_rcp")
			|| name.contains("MHLRCP" ) || name.contains("Button Jack")
			|| (Config::MACHINE_IS_GENERIC_AARCH64 && name.starts_with("uinput-")))
		{
			// sii9234_rcp on Samsung devices like Galaxy S2, may claim to be a gamepad & full keyboard
			// but has only special function keys
			// uinput-* devices like uinput-fpc and uinput-fortsense are actually finger-print sensors
			// that incorrectly claim to be gamepads
			log.info("ignoring extra device bits");
			src = 0;
			isGamepad = false;
		}
		updateGamepadSubtype(name, vendorProductId);
		typeFlags_ .gamepad = isGamepad;
	}
	if(kbType)
	{
		// Classify full alphabetic keyboards, and also devices with other keyboard
		// types, as long as they are exclusively SOURCE_KEYBOARD
		// (needed for the iCade 8-bitty since it reports a non-alphabetic keyboard type)
		if(kbType == AINPUT_KEYBOARD_TYPE_ALPHABETIC
			|| src == AINPUT_SOURCE_KEYBOARD)
		{
			typeFlags_ .keyboard = true;
			log.info("has keyboard type:{}", inputDeviceKeyboardTypeToStr(kbType));
		}
	}
	if(IG::isBitMaskSet(src, (int)AINPUT_SOURCE_JOYSTICK))
	{
		typeFlags_.joystick = true;
		log.info("detected a joystick");
		if(subtype_ == Subtype::NONE
			&& typeFlags_.gamepad
			&& !typeFlags_.keyboard
			&& !typeFlags_.virtualInput)
		{
			log.info("device looks like a generic gamepad");
			subtype_ = Subtype::GENERIC_GAMEPAD;
		}
		// check joystick axes
		static constexpr AxisId stickAxes[]{AxisId::X, AxisId::Y, AxisId::Z, AxisId::RX, AxisId::RY, AxisId::RZ,
				AxisId::HAT0X, AxisId::HAT0Y, AxisId::RUDDER, AxisId::WHEEL};
		static constexpr AxisFlags stickAxesBits[]{{.x = true}, {.y = true}, {.z = true}, {.rx = true}, {.ry = true}, {.rz = true},
				{.hatX = true}, {.hatY = true}, {.rudder = true}, {.wheel = true}};
		for(auto &axisId : stickAxes)
		{
			bool hasAxis = asInt(jsAxisFlags & stickAxesBits[std::distance(stickAxes, &axisId)]);
			if(!hasAxis)
			{
				continue;
			}
			log.info("joystick axis:{}", (int)axisId);
			axis.emplace_back(axisId);
			assert(!axis.isFull());
		}
		// check trigger axes
		static constexpr AxisId triggerAxes[]{AxisId::LTRIGGER, AxisId::RTRIGGER, AxisId::GAS, AxisId::BRAKE};
		static constexpr AxisFlags triggerAxesBits[]{{.lTrigger = true}, {.rTrigger = true}, {.gas = true}, {.brake = true}};
		AxisFlags addedTriggers{};
		for(auto &axisId : triggerAxes)
		{
			auto triggerAxisBits = triggerAxesBits[std::distance(triggerAxes, &axisId)];
			bool hasAxis = asInt(jsAxisFlags & triggerAxisBits);
			if(!hasAxis)
			{
				continue;
			}
			log.info("trigger axis:{}", (int)axisId);
			axis.emplace_back(axisId);
			assert(!axis.isFull());
			addedTriggers |= triggerAxisBits;
			if(addedTriggers == AxisFlags{.lTrigger = true, .rTrigger = true})
				break; // don't parse Gas/Brake if triggers are present to avoid duplicate events
		}
	}
}

bool AndroidInputDevice::operator ==(AndroidInputDevice const& rhs) const
{
	return id_ == rhs.id_ && name_ == rhs.name_;
}

void AndroidInputDevice::update(const AndroidInputDevice &other)
{
	name_ = other.name_;
	typeFlags_ = other.typeFlags_;
	subtype_ = other.subtype_;
	axis = other.axis;
}

bool Device::anyTypeFlagsPresent(ApplicationContext ctx, DeviceTypeFlags typeFlags)
{
	auto &app = ctx.application();
	if(typeFlags.keyboard)
	{
		if(app.keyboardType() == ACONFIGURATION_KEYBOARD_QWERTY)
		{
			if(app.hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_YES || app.hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_SOFT)
			{
				log.debug("keyboard present, but not in use");
			}
			else
			{
				//logDMsg("keyboard present");
				return true;
			}
		}
		typeFlags.keyboard = false; // ignore keyboards in device list
	}

	if(Config::MACHINE_IS_GENERIC_ARMV7 && app.hasXperiaPlayGamepad() &&
		typeFlags.gamepad && app.hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		log.debug("Xperia-play gamepad in use");
		return true;
	}

	for(auto &devPtr : app.inputDevices())
	{
		auto &e = *devPtr;
		if((e.isVirtual() && (typeFlags.miscKeys && e.typeFlags().miscKeys)) // virtual devices count as miscKeys flag only
			|| (!e.isVirtual() && asInt(e.typeFlags() & typeFlags)))
		{
			log.debug("device:{} has bits:{:X}", e.name(), std::bit_cast<uint8_t>(typeFlags));
			return true;
		}
	}
	return false;
}

bool hasGetAxisValue()
{
	if constexpr(Config::ENV_ANDROID_MIN_SDK == 9)
		return AMotionEvent_getAxisValueFunc;
	else
		return true;
}

}

namespace IG
{

constexpr SystemLogger log{"InputConfig"};

int32_t (*AMotionEvent_getActionButton_)(const AInputEvent* motion_event){};

static bool isXperiaPlayDeviceStr(std::string_view str)
{
	return str.contains("R800") || str == "zeus";
}

bool AndroidApplication::hasMultipleInputDeviceSupport() const
{
	return Config::ENV_ANDROID_MIN_SDK >= 12 || processInput_ == &AndroidApplication::processInputWithGetEvent;
}

void AndroidApplication::initInput(ApplicationContext ctx, JNIEnv *env, jobject baseActivity, jclass baseActivityClass, int32_t androidSDK)
{
	if(androidSDK >= 12)
	{
		processInput_ = &AndroidApplication::processInputWithGetEvent;
		if constexpr(Config::ENV_ANDROID_MIN_SDK == 9)
		{
			// load AMotionEvent_getAxisValue dynamically
			if(!loadSymbol(AMotionEvent_getAxisValueFunc, {}, "AMotionEvent_getAxisValue"))
			{
				bug_unreachable("AMotionEvent_getAxisValue not found even though using SDK level >= 12");
			}
			if(androidSDK >= 14)
			{
				if(!loadSymbol(AMotionEvent_getButtonStateFunc, {}, "AMotionEvent_getButtonState"))
				{
					bug_unreachable("AMotionEvent_getButtonState not found even though using SDK level >= 14");
				}
			}
		}
		if(androidSDK >= 33)
		{
			if(!loadSymbol(AMotionEvent_getActionButton_, {}, "AMotionEvent_getActionButton"))
			{
				bug_unreachable("AMotionEvent_getActionButton not found even though using SDK level >= 33");
			}
		}

		jEnumInputDevices = {env, baseActivityClass, "enumInputDevices", "(J)V"};

		// device change notifications
		if(androidSDK >= 16)
		{
			log.info("setting up input notifications");
			auto &impl = inputDeviceChangeImpl.emplace<InputDeviceListenerImpl>();
			JNI::InstMethod<jobject(jlong)> jInputDeviceListenerHelper{env, baseActivityClass, "inputDeviceListenerHelper", "(J)Lcom/imagine/InputDeviceListenerHelper;"};
			impl.listenerHelper = {env, jInputDeviceListenerHelper(env, baseActivity, jlong(ctx.aNativeActivityPtr()))};
			auto inputDeviceListenerHelperCls = env->GetObjectClass(impl.listenerHelper);
			impl.jRegister = {env, inputDeviceListenerHelperCls, "register", "()V"};
			impl.jUnregister = {env, inputDeviceListenerHelperCls, "unregister", "()V"};
			JNINativeMethod method[]
			{
				{
					"deviceChanged", "(JIILandroid/view/InputDevice;Ljava/lang/String;IIII)V",
					(void*)
					+[](JNIEnv* env, jobject, jlong nUserData, jint change, jint devID, [[maybe_unused]] jobject jDev,
						jstring jName, jint src, jint kbType, jint jsAxisFlags, jint vendorProductId)
					{
						ApplicationContext ctx{reinterpret_cast<ANativeActivity*>(nUserData)};
						auto &app = ctx.application();
						if(change == Input::DEVICE_REMOVED)
						{
							app.removeInputDevice(ctx, Input::Map::SYSTEM, devID, true);
						}
						else // add or update existing
						{
							const char *name = env->GetStringUTFChars(jName, nullptr);
							auto sysDev = std::make_unique<Input::Device>(std::in_place_type<Input::AndroidInputDevice>, devID,
								src, name, kbType, std::bit_cast<Input::AxisFlags>(jsAxisFlags), (uint32_t)vendorProductId, false);
							env->ReleaseStringUTFChars(jName, name);
							app.updateAndroidInputDevice(ctx, std::move(sysDev), true);
						}
					}
				}
			};
			env->RegisterNatives(inputDeviceListenerHelperCls, method, std::size(method));
			addOnResume([this, env](ApplicationContext ctx, bool)
			{
				enumInputDevices(ctx, env, ctx.baseActivityObject(), true);
				log.info("registering input device listener");
				auto &impl = *std::get_if<InputDeviceListenerImpl>(&inputDeviceChangeImpl);
				impl.jRegister(env, impl.listenerHelper);
				return true;
			}, INPUT_DEVICE_ON_RESUME_PRIORITY);
			addOnExit([this, env](ApplicationContext, [[maybe_unused]] bool backgrounded)
			{
				log.info("unregistering input device listener");
				auto &impl = *std::get_if<InputDeviceListenerImpl>(&inputDeviceChangeImpl);
				impl.jUnregister(env, impl.listenerHelper);
				return true;
			}, INPUT_DEVICE_ON_EXIT_PRIORITY);
		}
		else
		{
			log.info("setting up input notifications with inotify");
			auto &impl = inputDeviceChangeImpl.emplace<INotifyImpl>(Timer{
				{.debugLabel = "inputRescanCallback"}, [ctx]{ ctx.enumInputDevices(); }});
			impl.fd = inotify_init();
			if(impl.fd == -1)
			{
				log.error("couldn't create inotify instance");
			}
			else
			{
				int ret = ALooper_addFd(EventLoop::forThread().nativeObject(), impl.fd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
					[](int fd, int events, void* data)
					{
						log.info("got inotify event");
						auto &app = *((AndroidApplication*)data);
						auto &impl = *std::get_if<INotifyImpl>(&app.inputDeviceChangeImpl);
						if(events == ALOOPER_EVENT_INPUT)
						{
							char buffer[2048];
							[[maybe_unused]] auto size = read(fd, buffer, sizeof(buffer));
							if(app.isRunning())
							{
								impl.rescanTimer.runIn(IG::Milliseconds(250));
							}
						}
						return 1;
					}, this);
				if(ret != 1)
				{
					log.error("couldn't add inotify fd to looper");
				}
				addOnResume([this, env](ApplicationContext ctx, bool)
				{
					auto &impl = *std::get_if<INotifyImpl>(&inputDeviceChangeImpl);
					enumInputDevices(ctx, env, ctx.baseActivityObject(), true);
					if(impl.fd != -1 && impl.watch == -1)
					{
						log.info("registering inotify input device listener");
						impl.watch = inotify_add_watch(impl.fd, "/dev/input", IN_CREATE | IN_DELETE);
						if(impl.watch == -1)
						{
							log.error("error setting inotify watch");
						}
					}
					return true;
				}, INPUT_DEVICE_ON_RESUME_PRIORITY);
				addOnExit([this](ApplicationContext, [[maybe_unused]] bool backgrounded)
				{
					auto &impl = *std::get_if<INotifyImpl>(&inputDeviceChangeImpl);
					if(impl.watch != -1)
					{
						log.info("unregistering inotify input device listener");
						inotify_rm_watch(impl.fd, impl.watch);
						impl.watch = -1;
						impl.rescanTimer.cancel();
					}
					return true;
				}, INPUT_DEVICE_ON_EXIT_PRIORITY);
			}
		}
	}
	else
	{
		// no multi-input device support
		auto genericKeyDev = Input::makeGenericKeyDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			auto buildDevice = androidBuildDevice(env, baseActivityClass);
			if(isXperiaPlayDeviceStr(buildDevice))
			{
				log.info("detected Xperia Play gamepad");
				genericKeyDev->setSubtype(Input::DeviceSubtype::XPERIA_PLAY);
			}
			else if(buildDevice == "sholes")
			{
				log.info("detected Droid/Milestone keyboard");
			}
		}
		builtinKeyboardDev = addAndroidInputDevice(ctx, std::move(genericKeyDev), false);
	}
}

void AndroidApplication::initInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navigationState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	if(trackballNav)
		log.info("detected trackball");

	aHardKeyboardState = hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	log.info("keyboard/nav hidden:{}", Input::hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		log.info("keyboard type:{}", aKeyboardType);
}

void AndroidApplication::updateInputConfig(ApplicationContext ctx, AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	log.info("config change, keyboard:{}, navigation:{}", Input::hardKeyboardNavStateToStr(hardKeyboardState), Input::hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(ctx, hasXperiaPlayGamepad() ? navState : hardKeyboardState);
}

void ApplicationContext::enumInputDevices() const
{
	application().enumInputDevices(*this, mainThreadJniEnv(), baseActivityObject(), true);
}

bool ApplicationContext::hasInputDeviceHotSwap() const { return androidSDK() >= 12; }

void AndroidApplication::enumInputDevices(ApplicationContext ctx, JNIEnv* env, jobject baseActivity, bool notify)
{
	log.info("doing input device scan");
	removeInputDevices(ctx, Input::Map::SYSTEM);
	jEnumInputDevices(env, baseActivity, jlong(ctx.aNativeActivityPtr()));
	if(!virtualDev)
	{
		log.info("no \"Virtual\" device id found, adding one");
		virtualDev = addAndroidInputDevice(ctx,
			std::make_unique<Input::Device>(std::in_place_type<Input::AndroidInputDevice>, -1, Input::virtualDeviceFlags, "Virtual"),
			false);
	}
	if(notify) { onEvent(ctx, Input::DevicesEnumeratedEvent{}); }
}

bool AndroidApplication::hasTrackball() const
{
	return trackballNav;
}

bool AndroidApplicationContext::hasTrackball() const
{
	return application().hasTrackball();
}

int AndroidApplication::hardKeyboardState() const { return aHardKeyboardState; }

void AndroidApplication::setHardKeyboardState(ApplicationContext ctx, int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		log.info("hard keyboard hidden:{}", Input::hardKeyboardNavStateToStr(aHardKeyboardState));
		if(builtinKeyboardDev)
		{
			bool shown = aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO;
			auto change = shown ? Input::DeviceChange::shown : Input::DeviceChange::hidden;
			onEvent(ctx, Input::DeviceChangeEvent{*builtinKeyboardDev, change});
		}
	}
}

int AndroidApplication::keyboardType() const { return aKeyboardType; }

bool AndroidApplication::hasXperiaPlayGamepad() const
{
	return builtinKeyboardDev && builtinKeyboardDev->subtype() == Input::DeviceSubtype::XPERIA_PLAY;
}

Input::Device *AndroidApplication::addAndroidInputDevice(ApplicationContext ctx, std::unique_ptr<Input::Device> devPtr, bool notify)
{
	if(Config::DEBUG_BUILD)
	{
		if(inputDeviceForId(devPtr->id()))
			log.warn("adding duplicate device ID:{}", devPtr->id());
	}
	log.info("added device id:{} ({}) to list", devPtr->id(), devPtr->name());
	return &addInputDevice(ctx, std::move(devPtr), notify);
}

Input::Device *AndroidApplication::updateAndroidInputDevice(ApplicationContext ctx, std::unique_ptr<Input::Device> devPtr, bool notify)
{
	auto existingDevPtr = inputDeviceForId(devPtr->id());
	if(!existingDevPtr)
	{
		return addAndroidInputDevice(ctx, std::move(devPtr), notify);
	}
	else
	{
		auto &existingDev = getAs<Input::AndroidInputDevice>(*existingDevPtr);
		log.info("device id:{} ({}) updated", devPtr->id(), devPtr->name());
		existingDev.update(getAs<Input::AndroidInputDevice>(*devPtr));
		if(notify)
			dispatchInputDeviceChange(ctx, existingDev, Input::DeviceChange::updated);
		return existingDevPtr;
	}
}

}
