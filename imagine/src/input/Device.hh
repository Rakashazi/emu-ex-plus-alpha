#pragma once

#include <input/config.hh>
#include <util/bits.h>

namespace Input
{

class Device
{
public:
	constexpr Device() {}
	constexpr Device(uint devId, uint map, uint type, const char *name_): map_{map}, type_{type}, devId{devId}, name_{name_} {}
	virtual ~Device() {}

	static constexpr uint SUBTYPE_NONE = 0,
		SUBTYPE_XPERIA_PLAY = 1,
		SUBTYPE_PS3_CONTROLLER = 2,
		SUBTYPE_MOTO_DROID_KEYBOARD = 3,
		SUBTYPE_OUYA_CONTROLLER = 4,
		SUBTYPE_PANDORA_HANDHELD = 5,
		SUBTYPE_XBOX_360_CONTROLLER = 6,
		SUBTYPE_NVIDIA_SHIELD = 7
		;

	static constexpr uint
		TYPE_BIT_KEY_MISC = IG::bit(0),
		TYPE_BIT_KEYBOARD = IG::bit(1),
		TYPE_BIT_GAMEPAD = IG::bit(2),
		TYPE_BIT_JOYSTICK = IG::bit(3),
		TYPE_BIT_VIRTUAL = IG::bit(4)
		;

	struct Change
	{
		uint state;
		enum { ADDED, REMOVED, SHOWN, HIDDEN };

		constexpr Change(uint state): state(state) {}
		bool added() const { return state == ADDED; }
		bool removed() const { return state == REMOVED; }
		bool shown() const { return state == SHOWN; }
		bool hidden() const { return state == HIDDEN; }
	};

	bool hasKeyboard() const
	{
		return typeBits() & TYPE_BIT_KEYBOARD;
	}

	bool hasGamepad() const
	{
		return typeBits() & TYPE_BIT_GAMEPAD;
	}

	bool hasJoystick() const
	{
		return typeBits() & TYPE_BIT_JOYSTICK;
	}

	bool isVirtual() const
	{
		return typeBits() & TYPE_BIT_VIRTUAL;
	}

	uint enumId() const { return devId; }
	const char *name() const { return name_; }
	uint map() const;
	uint typeBits() const
	{
		return
		#ifdef CONFIG_INPUT_ICADE
		iCadeMode() ? TYPE_BIT_GAMEPAD :
		#endif
		type_;
	}

	uint subtype() const { return subtype_; }

	bool operator ==(Device const& rhs) const
	{
		return enumId() == rhs.enumId() && map_ == rhs.map_ && string_equal(name(), rhs.name());
	}

	#ifdef CONFIG_INPUT_ICADE
	virtual void setICadeMode(bool on) { logWarn("setICadeMode called but unimplemented"); }
	virtual bool iCadeMode() const { return false; }
	#endif

	virtual void setJoystickAxis1AsDpad(bool on) {}
	virtual bool joystickAxis1AsDpad() { return false; }

	const char *keyName(Key b) const;

	// TODO
	//bool isDisconnectable() { return 0; }
	//void disconnect() { }

	#if defined CONFIG_ENV_WEBOS
	bool anyTypeBitsPresent(uint typeBits) { bug_exit("TODO"); return 0; }
	#else
	static bool anyTypeBitsPresent(uint typeBits);
	#endif

protected:
	uint map_ = 0;
	uint type_ = 0;
	uint devId = 0;
public:
	uint subtype_ = 0;
	uint idx = 0;
protected:
	const char *name_ = nullptr;
};

}
