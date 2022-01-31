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
#include <imagine/util/concepts.hh>
#include <imagine/util/optional.hh>
#include <imagine/logger/logger.h>
#include <array>
#include <cstring>
#include <string_view>

namespace EmuEx
{

using namespace IG;

template <class T>
static std::optional<T> readOptionValue(IO &io, size_t bytesToRead, IG::Predicate<const T&> auto &&isValid)
{
	if(bytesToRead != sizeof(T))
	{
		logMsg("skipping %zu byte option value, expected %zu bytes", bytesToRead, sizeof(T));
		return {};
	}
	auto val = io.get<T>();
	if(!isValid(val))
		return {};
	return val;
}

template <class T>
static std::optional<T> readOptionValue(IO &io, size_t bytesToRead)
{
	return readOptionValue<T>(io, bytesToRead, [](const T&){ return true; });
}

template <class T>
static std::optional<T> readOptionValue(IO &io, size_t bytesToRead, auto &&func)
{
	return IG::doOptionally(readOptionValue<T>(io, bytesToRead), std::forward<decltype(func)>(func));
}

template <IG::Container T>
static std::optional<T> readStringOptionValue(IO &io, size_t bytesToRead)
{
	T val{};
	const auto destStringSize = val.max_size() - 1;
	if(bytesToRead > destStringSize)
	{
		logMsg("skipping %zu byte string option value, too large for %zu bytes", bytesToRead, destStringSize);
		return {};
	}
	auto size = io.readSized(val, bytesToRead);
	if(size == -1) [[unlikely]]
	{
		logErr("error reading %zu byte string option", bytesToRead);
		return {};
	}
	return val;
}

template <IG::Container T>
static void readStringOptionValue(IO &io, size_t bytesToRead, auto &&func)
{
	IG::doOptionally(readStringOptionValue<T>(io, bytesToRead), std::forward<decltype(func)>(func));
}

static void writeOptionValueHeader(IO &io, uint16_t key, uint16_t optSize)
{
	optSize += sizeof key;
	logMsg("writing option key:%u with size:%u", key, optSize);
	io.write(optSize);
	io.write(key);
}

static void writeOptionValue(IO &io, uint16_t key, const auto &val)
{
	writeOptionValueHeader(io, key, sizeof(decltype(val)));
	io.write(val);
}

template <class T>
static void writeOptionValue(IO &io, uint16_t key, const std::optional<T> &val)
{
	if(!val)
		return;
	writeOptionValue(io, key, *val);
}

static void writeStringOptionValue(IO &io, uint16_t key, std::string_view view)
{
	if(!view.size())
		return;
	writeOptionValueHeader(io, key, view.size());
	io.write(view.data(), view.size());
}

static void writeStringOptionValue(IO &io, uint16_t key, const IG::Container auto &c)
{
	writeStringOptionValue(io, key, std::string_view(c.data()));
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

		auto x = io.get<SERIALIZED_T>();
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

}
