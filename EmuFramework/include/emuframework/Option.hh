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

#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/optional.hh>
#include <imagine/util/used.hh>
#include <imagine/util/Property.hh>
#include <imagine/logger/logger.h>
#include <array>
#include <cstring>
#include <string_view>

namespace EmuEx
{

using namespace IG;

// Stateless option API

template <class T>
constexpr bool isAlwaysValid(const T &) { return true; }

template <class T>
concept PropertyOption =
	requires(T &&p)
	{
		T::uid;
		p.defaultValue();
	};

template <class T>
inline std::optional<T> readOptionValue(Readable auto &io, std::predicate<T> auto &&isValid)
{
	size_t bytesToRead = io.size();
	if(bytesToRead != sizeof(T))
	{
		logMsg("skipping %zu byte option value, expected %zu bytes", bytesToRead, sizeof(T));
		return {};
	}
	auto val = io.template get<T>();
	if(!isValid(val))
		return {};
	return val;
}

template <class T>
inline std::optional<T> readOptionValue(Readable auto &io)
{
	return readOptionValue<T>(io, isAlwaysValid<T>);
}

template <class T>
inline bool readOptionValue(Readable auto &io, Callable<void, T> auto &&func, std::predicate<T> auto &&isValid)
{
	return doOptionally(readOptionValue<T>(io, IG_forward(isValid)), IG_forward(func));
}

template <class T>
inline bool readOptionValue(Readable auto &io, Callable<void, T> auto &&func)
{
	return readOptionValue<T>(io, IG_forward(func), isAlwaysValid<T>);
}

template <class T>
inline bool readOptionValue(Readable auto &io, T &output, std::predicate<T> auto &&isValid)
{
	if(!used(output))
		return false;
	return readOptionValue<T>(io,
		[&](auto &&val){ output = IG_forward(val); }, IG_forward(isValid));
}

template <class T>
inline bool readOptionValue(Readable auto &io, T &output)
{
	if(!used(output))
		return false;
	return readOptionValue<T>(io, output, isAlwaysValid<T>);
}

template<PropertyOption Prop>
inline bool readOptionValue(Readable auto &io, Prop &output)
{
	using T = Prop::SerializedType;
	auto bytesToRead = io.size();
	if(bytesToRead != sizeof(T))
	{
		logMsg("skipping %zu byte option value, expected %zu bytes", bytesToRead, sizeof(T));
		return false;
	}
	return output.unserialize(io.template get<T>());
}

template <Container T>
inline std::optional<T> readStringOptionValue(Readable auto &io)
{
	T val{};
	size_t bytesToRead = io.size();
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

template <Container T>
inline bool readStringOptionValue(Readable auto &io, Callable<void, T> auto &&func)
{
	return doOptionally(readStringOptionValue<T>(io), IG_forward(func));
}

template <Container T>
inline bool readStringOptionValue(Readable auto &io, T &output)
{
	return readStringOptionValue<T>(io, [&](auto &&val){ output = IG_forward(val); });
}

inline void writeOptionValueHeader(Writable auto &io, uint16_t key, uint16_t optSize)
{
	optSize += sizeof key;
	logMsg("writing option key:%u with size:%u", key, optSize);
	io.put(optSize);
	io.put(key);
}

inline void writeOptionValue(Writable auto &io, uint16_t key, const auto &val)
{
	if(!used(val))
		return;
	writeOptionValueHeader(io, key, sizeof(decltype(val)));
	io.put(val);
}

template <class T>
inline void writeOptionValue(Writable auto &io, uint16_t key, const std::optional<T> &val)
{
	if(!val)
		return;
	writeOptionValue(io, key, *val);
}

inline void writeOptionValueIfNotDefault(Writable auto &io, uint16_t key, const auto &val, const auto &defaultVal)
{
	if(val == defaultVal)
		return;
	writeOptionValue(io, key, val);
}

inline void writeOptionValueIfNotDefault(Writable auto&, Unused auto const&) {}

inline void writeOptionValueIfNotDefault(Writable auto &io, PropertyOption auto const &p)
{
	if(p.isDefault())
		return;
	writeOptionValue(io, p.uid, p.serialize());
}

inline void writeStringOptionValueAllowEmpty(Writable auto &io, uint16_t key, std::string_view s)
{
	writeOptionValueHeader(io, key, s.size());
	io.write(s.data(), s.size());
}

inline void writeStringOptionValue(Writable auto &io, uint16_t key, std::string_view s)
{
	if(s.empty())
		return;
	writeStringOptionValueAllowEmpty(io, key, s);
}

inline void writeStringOptionValue(Writable auto &io, uint16_t key, const Container auto &c)
{
	writeStringOptionValue(io, key, std::string_view(c.data()));
}

inline void writeStringOptionValueIfNotDefault(Writable auto &io, uint16_t key, std::string_view s, std::string_view defaultStr)
{
	if(s == defaultStr)
		return;
	writeStringOptionValueAllowEmpty(io, key, s);
}

template<class Size>
inline size_t sizedDataBytes(const ResizableContainer auto& c)
{
	size_t bytes = sizeof(Size); // store array length
	Size size = std::min(c.size(), size_t(std::numeric_limits<Size>::max()));
	bytes += size * sizeof(c[0]);
	return bytes;
}

template<class Size>
inline void writeSizedData(Writable auto& io, const ResizableContainer auto& c)
{
	Size size = std::min(c.size(), size_t(std::numeric_limits<Size>::max()));
	io.put(size);
	if(size)
		io.write(c.data(), size);
}

template<class Size>
inline ssize_t readSizedData(Readable auto& io, ResizableContainer auto& c)
{
	auto size = io.template get<Size>();
	return io.readSized(c, size);
}


}
