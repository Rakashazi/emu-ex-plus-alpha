#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/IO.hh>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <array>
#include <optional>
#include <cstring>

template<class T> struct isOptional : public std::false_type {};

template<class T>
struct isOptional<std::optional<T>> : public std::true_type {};

template <class T, class Validator = std::nullptr_t>
static std::optional<T> readOptionValue(IO &io, unsigned bytesToRead, Validator &&isValid = nullptr)
{
	if(bytesToRead != sizeof(T))
	{
		logMsg("skipping %u byte option value, expected %u bytes", bytesToRead, (unsigned)sizeof(T));
		return {};
	}
	auto [val, size] = io.read<T>();
	if(size == -1) [[unlikely]]
	{
		logErr("error reading %u byte option", bytesToRead);
		return {};
	}
	if constexpr(!std::is_null_pointer_v<Validator>)
	{
		if(!isValid(val))
			return {};
	}
	return val;
}

template <class T>
static std::optional<T> readStringOptionValue(IO &io, unsigned bytesToRead)
{
	static_assert(sizeof(T) != 0, "Destination type cannot have 0 size");
	constexpr auto destStringSize = sizeof(T) - 1;
	if(bytesToRead > destStringSize)
	{
		logMsg("skipping %u byte string option value, too large for %u bytes", bytesToRead, (unsigned)destStringSize);
		return {};
	}
	T val{};
	auto size = io.read(val.data(), bytesToRead);
	if(size == -1) [[unlikely]]
	{
		logErr("error reading %u byte string option", bytesToRead);
		return {};
	}
	return val;
}

template <class T>
static void writeOptionValue(IO &io, uint16_t key, T &&val)
{
	if constexpr(isOptional<T>::value)
	{
		if(!val)
			return;
		writeOptionValue(io, key, *val);
	}
	else
	{
		uint16_t ioSize = sizeof(typeof(key)) + sizeof(T);
		logMsg("writing option key:%u with size:%u", key, ioSize);
		io.write(ioSize);
		io.write(key);
		io.write(val);
	}
}

template <class T>
static void writeStringOptionValue(IO &io, uint16_t key, T &&val)
{
	if constexpr(isOptional<T>::value)
	{
		if(!val)
			return;
		writeStringOptionValue(io, key, *val);
	}
	else
	{
		uint16_t stringLen = strlen(val.data());
		uint16_t ioSize = sizeof(typeof(key)) + stringLen;
		logMsg("writing string option key:%u with size:%u", key, ioSize);
		io.write(ioSize);
		io.write(key);
		io.write(val.data(), stringLen);
	}
}

struct OptionBase
{
	bool isConst = false;

	constexpr OptionBase() {}
	constexpr OptionBase(bool isConst): isConst(isConst) {}
	virtual bool isDefault() const = 0;
	virtual unsigned ioSize() const = 0;
	virtual bool writeToIO(IO &io) = 0;
};

template <class T>
bool OptionMethodIsAlwaysValid(T)
{
	return 1;
}

template <class T>
struct OptionMethodBase
{
	constexpr OptionMethodBase(bool (&validator)(T v)): validator(validator) {}
	bool (&validator)(T v);
	bool isValidVal(T v) const
	{
		return validator(v);
	}
};

template <class T, T (&GET)(), void (&SET)(T)>
struct OptionMethodFunc : public OptionMethodBase<T>
{
	constexpr OptionMethodFunc(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodFunc(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	T get() const { return GET(); }
	void set(T v) { SET(v); }
};

template <class T, T &val>
struct OptionMethodRef : public OptionMethodBase<T>
{
	constexpr OptionMethodRef(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodRef(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	T get() const { return val; }
	void set(T v) { val = v; }
};

template <class T>
struct OptionMethodVar : public OptionMethodBase<T>
{
	constexpr OptionMethodVar(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodVar(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator), val(init) {}
	T val{};
	T get() const { return val; }
	void set(T v) { val = v; }
};

template <class V, class SERIALIZED_T = typeof(V().get())>
struct Option : public OptionBase, public V
{
private:
	const uint16_t KEY;
public:
	typedef typeof(V().get()) T;
	T defaultVal;

	constexpr Option(uint16_t key, T defaultVal = 0, bool isConst = 0, bool (&validator)(T v) = OptionMethodIsAlwaysValid):
		OptionBase(isConst), V(defaultVal, validator), KEY(key), defaultVal(defaultVal)
	{}

	Option & operator = (T other)
	{
		if(!isConst)
			V::set(other);
		return *this;
	}

	bool operator ==(T const& rhs) const { return V::get() == rhs; }
	bool operator !=(T const& rhs) const { return V::get() != rhs; }

	bool isDefault() const override { return V::get() == defaultVal; }
	void initDefault(T val) { defaultVal = val; V::set(val); }
	T reset()
	{
		V::set(defaultVal);
		return defaultVal;
	}

	void resetToConst()
	{
		reset();
		isConst = true;
	}

	operator T() const
	{
		return V::get();
	}

	bool writeToIO(IO &io) override
	{
		logMsg("writing option key %u after size %u", KEY, ioSize());
		io.write(KEY);
		io.write((SERIALIZED_T)V::get());
		return true;
	}

	bool writeWithKeyIfNotDefault(IO &io)
	{
		if(!isDefault())
		{
			io.write((uint16_t)ioSize());
			writeToIO(io);
		}
		return true;
	}

	bool readFromIO(IO &io, unsigned readSize)
	{
		if(isConst || readSize != sizeof(SERIALIZED_T))
		{
			if(isConst)
				logMsg("skipping const option value");
			else
				logMsg("skipping %d byte option value, expected %d", readSize, (int)sizeof(SERIALIZED_T));
			return false;
		}

		auto [x, size] = io.read<SERIALIZED_T>();
		if(size == -1)
		{
			logErr("error reading option from io");
			return false;
		}
		if(V::isValidVal(x))
			V::set(x);
		else
			logMsg("skipped invalid option value");
		return true;
	}

	unsigned ioSize() const override
	{
		return sizeof(typeof(KEY)) + sizeof(SERIALIZED_T);
	}
};

struct PathOption : public OptionBase
{
	char *val;
	unsigned strSize;
	const char *defaultVal;
	const uint16_t KEY;

	constexpr PathOption(uint16_t key, char *val, unsigned size, const char *defaultVal): val(val), strSize(size), defaultVal(defaultVal), KEY(key) {}
	template <size_t S>
	constexpr PathOption(uint16_t key, char (&val)[S], const char *defaultVal): PathOption(key, val, S, defaultVal) {}
	template <size_t S>
	constexpr PathOption(uint16_t key, std::array<char, S> &val, const char *defaultVal): PathOption(key, val.data(), S, defaultVal) {}

	bool isDefault() const override { return string_equal(val, defaultVal); }

	operator char *() const
	{
		return val;
	}

	bool writeToIO(IO &io) override;
	bool readFromIO(IO &io, unsigned readSize);
	unsigned ioSize() const override;
};

using SByte1Option = Option<OptionMethodVar<int8_t>, int8_t>;
using Byte1Option = Option<OptionMethodVar<uint8_t>, uint8_t>;
using Byte2Option = Option<OptionMethodVar<uint16_t>, uint16_t>;
using Byte4s2Option = Option<OptionMethodVar<uint32_t>, uint16_t>;
using Byte4Option = Option<OptionMethodVar<uint32_t>, uint32_t>;
using Byte4s1Option = Option<OptionMethodVar<uint32_t>, uint8_t>;
using DoubleOption = Option<OptionMethodVar<double>, double>;

template<int MAX, class T>
static bool optionIsValidWithMax(T val)
{
	return val <= MAX;
}

template<int MIN, int MAX, class T>
static bool optionIsValidWithMinMax(T val)
{
	return val >= MIN && val <= MAX;
}
