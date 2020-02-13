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

#include <array>
#include <imagine/io/IO.hh>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>

struct OptionBase
{
	bool isConst = false;

	constexpr OptionBase() {}
	constexpr OptionBase(bool isConst): isConst(isConst) {}
	virtual bool isDefault() const = 0;
	virtual uint ioSize() = 0;
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
	bool isValidVal(T v)
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

	bool isDefault() const { return V::get() == defaultVal; }
	void initDefault(T val) { defaultVal = val; V::set(val); }
	void reset() { V::set(defaultVal); }

	operator T() const
	{
		return V::get();
	}

	bool writeToIO(IO &io)
	{
		logMsg("writing option key %u after size %u", KEY, ioSize());
		std::error_code ec{};
		io.writeVal(KEY, &ec);
		io.writeVal((SERIALIZED_T)V::get(), &ec);
		return true;
	}

	bool writeWithKeyIfNotDefault(IO &io)
	{
		if(!isDefault())
		{
			std::error_code ec{};
			io.writeVal((uint16_t)ioSize(), &ec);
			writeToIO(io);
		}
		return true;
	}

	bool readFromIO(IO &io, uint readSize)
	{
		if(isConst || readSize != sizeof(SERIALIZED_T))
		{
			if(isConst)
				logMsg("skipping const option value");
			else
				logMsg("skipping %d byte option value, expected %d", readSize, (int)sizeof(SERIALIZED_T));
			return false;
		}

		std::error_code ec{};
		auto x = io.readVal<SERIALIZED_T>(&ec);
		if(ec)
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

	uint ioSize()
	{
		return sizeof(typeof(KEY)) + sizeof(SERIALIZED_T);
	}
};

struct PathOption : public OptionBase
{
	char *val;
	uint strSize;
	const char *defaultVal;
	const uint16_t KEY;

	constexpr PathOption(uint16_t key, char *val, uint size, const char *defaultVal): val(val), strSize(size), defaultVal(defaultVal), KEY(key) {}
	template <size_t S>
	constexpr PathOption(uint16_t key, char (&val)[S], const char *defaultVal): PathOption(key, val, S, defaultVal) {}
	template <size_t S>
	constexpr PathOption(uint16_t key, std::array<char, S> &val, const char *defaultVal): PathOption(key, val.data(), S, defaultVal) {}

	bool isDefault() const { return string_equal(val, defaultVal); }

	operator char *() const
	{
		return val;
	}

	bool writeToIO(IO &io);
	bool readFromIO(IO &io, uint readSize);
	uint ioSize();
};

using SByte1Option = Option<OptionMethodVar<int8_t>, int8_t>;
using Byte1Option = Option<OptionMethodVar<uint8_t>, uint8_t>;
using Byte2Option = Option<OptionMethodVar<uint16_t>, uint16_t>;
using Byte4s2Option = Option<OptionMethodVar<uint32_t>, uint16_t>;
using Byte4Option = Option<OptionMethodVar<uint32_t>, uint32_t>;
using Byte4s1Option = Option<OptionMethodVar<uint32_t>, uint8_t>;
using DoubleOption = Option<OptionMethodVar<double>, double>;

template<int MAX, class T>
bool optionIsValidWithMax(T val)
{
	return val <= MAX;
}

template<int MIN, int MAX, class T>
bool optionIsValidWithMinMax(T val)
{
	return val >= MIN && val <= MAX;
}
