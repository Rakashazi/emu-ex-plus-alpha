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
#include <imagine/util/concepts.hh>
#include <imagine/util/optional.hh>
#include <imagine/util/typeTraits.hh>
#include <imagine/logger/logger.h>
#include <array>
#include <cstring>
#include <string_view>
#include <compare>

namespace EmuEx
{

using namespace IG;

template <class T>
inline std::optional<T> readOptionValue(IO &io, size_t bytesToRead, IG::Predicate<const T&> auto &&isValid)
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
inline std::optional<T> readOptionValue(IO &io, size_t bytesToRead)
{
	return readOptionValue<T>(io, bytesToRead, [](const T&){ return true; });
}

template <class T>
inline std::optional<T> readOptionValue(IO &io, size_t bytesToRead, auto &&func)
{
	return IG::doOptionally(readOptionValue<T>(io, bytesToRead), std::forward<decltype(func)>(func));
}

template <IG::Container T>
inline std::optional<T> readStringOptionValue(IO &io, size_t bytesToRead)
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
inline void readStringOptionValue(IO &io, size_t bytesToRead, auto &&func)
{
	IG::doOptionally(readStringOptionValue<T>(io, bytesToRead), std::forward<decltype(func)>(func));
}

inline void writeOptionValueHeader(IO &io, uint16_t key, uint16_t optSize)
{
	optSize += sizeof key;
	logMsg("writing option key:%u with size:%u", key, optSize);
	io.write(optSize);
	io.write(key);
}

inline void writeOptionValue(IO &io, uint16_t key, const auto &val)
{
	writeOptionValueHeader(io, key, sizeof(decltype(val)));
	io.write(val);
}

template <class T>
inline void writeOptionValue(IO &io, uint16_t key, const std::optional<T> &val)
{
	if(!val)
		return;
	writeOptionValue(io, key, *val);
}

inline void writeStringOptionValue(IO &io, uint16_t key, std::string_view view)
{
	if(!view.size())
		return;
	writeOptionValueHeader(io, key, view.size());
	io.write(view.data(), view.size());
}

inline void writeStringOptionValue(IO &io, uint16_t key, const IG::Container auto &c)
{
	writeStringOptionValue(io, key, std::string_view(c.data()));
}

template <class T>
constexpr bool optionIsAlwaysValid(T)
{
	return true;
}

template <class T>
struct Option
{
public:
	bool (*validator)(T v);
	T val{};
	T defaultVal{};
	uint16_t KEY;
	bool isConst{};
	static constexpr size_t SIZE = sizeof(T);

	constexpr Option() = default;

	constexpr Option(uint16_t key, T defaultVal = {}, bool isConst = false,
		bool (*validator)(T v) = optionIsAlwaysValid):
		validator{validator},
		val{defaultVal},
		defaultVal{defaultVal},
		KEY{key},
		isConst{isConst} {}

	constexpr Option &operator=(T other)
	{
		if(!isConst)
			val = other;
		return *this;
	}

	constexpr auto operator<=>(auto rhs) const { return val <=> (T)rhs; }

	constexpr bool isDefault() const { return val == defaultVal; }

	constexpr void initDefault(T v)
	{
		val = defaultVal = v;
	}

	constexpr T reset()
	{
		val = defaultVal;
		return defaultVal;
	}

	constexpr void resetToConst()
	{
		reset();
		isConst = true;
	}

	constexpr operator T() const
	{
		return val;
	}

	bool writeToIO(IO &io) const
	{
		logMsg("writing option key %u after size %zu", KEY, ioSize());
		io.write(KEY);
		io.write(val);
		return true;
	}

	bool writeWithKeyIfNotDefault(IO &io) const
	{
		if(!isDefault())
		{
			io.write((uint16_t)ioSize());
			writeToIO(io);
		}
		return true;
	}

	bool readFromIO(IO &io, size_t readSize)
	{
		if(isConst || readSize != SIZE)
		{
			if(isConst)
				logMsg("skipping const option value");
			else
				logMsg("skipping %zu byte option value, expected %zu", readSize, SIZE);
			return false;
		}

		auto x = io.get<T>();
		if(isValidVal(x))
			val = x;
		else
			logMsg("skipped invalid option value");
		return true;
	}

	constexpr size_t ioSize() const
	{
		return sizeof(typeof(KEY)) + SIZE;
	}

	constexpr bool isValidVal(T v) const
	{
		return validator(v);
	}
};

using SByte1Option = Option<int8_t>;
using Byte1Option = Option<uint8_t>;
using Byte2Option = Option<uint16_t>;
using Byte4Option = Option<uint32_t>;
using DoubleOption = Option<double>;

template <class T>
inline void writeOptionValue(IO &io, const Option<T> &opt)
{
	if(opt.isDefault())
		return;
	io.write((uint16_t)opt.ioSize());
	opt.writeToIO(io);
}

inline void writeOptionValue(IO &io, Unused auto &opt) {}

template<int MAX, class T>
constexpr bool optionIsValidWithMax(T val)
{
	return val <= MAX;
}

template<int MIN, int MAX, class T>
constexpr bool optionIsValidWithMinMax(T val)
{
	return val >= MIN && val <= MAX;
}

}
