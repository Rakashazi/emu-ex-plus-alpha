#pragma once

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

#include <imagine/util/used.hh>
#include <imagine/util/utility.h>
#include <type_traits>
#include <concepts>

namespace IG
{

template<class T>
constexpr bool isValidProperty(const T&) { return true; }

template<class T, class SerializedT = T, std::predicate<const T&> Validator = bool(*)(const T&)>
struct PropertyDesc
{
	using SerializedType = SerializedT;
	T defaultValue{};
	bool mutableDefault{};
	Validator isValid = [](const T &v){ return isValidProperty(v); };
};

template<class T, auto uid_, PropertyDesc desc = PropertyDesc<T>{}>
class Property
{
public:
	using Type = T;
	using SerializedType = decltype(desc)::SerializedType;
	static constexpr bool sameSerializedType = std::is_same_v<T, SerializedType>;
	static constexpr auto uid{uid_};

	static_assert(desc.isValid(desc.defaultValue));

	constexpr Property() = default;
	constexpr Property(const Property&) = default;
	constexpr Property& operator=(const Property&) = default;

	constexpr Property& operator=(auto &&v)
	{
		set(IG_forward(v));
		return *this;
	}

	constexpr void setUnchecked(auto &&v)
	{
		assert(isValid(v));
		value_ = IG_forward(v);
	}

	constexpr bool set(auto &&v)
	{
		if(!isValid(v))
			return false;
		value_ = IG_forward(v);
		return true;
	}

	constexpr bool setDefaultValue(auto &&v)
	{
		if(!isValid(v))
			return false;
		defaultValue_ = IG_forward(v);
		return true;
	}

	constexpr operator const T&() const { return value_; }
	constexpr const T& value() const { return value_; }
	constexpr const T& defaultValue() const { return defaultValue_; }
	constexpr bool isDefault() const { return value() == defaultValue(); }
	constexpr const T& reset() { return value_ = defaultValue(); }
	static constexpr bool isValid(const auto &v) { return desc.isValid(v); }

	constexpr const T& resetDefault(auto &&v)
	{
		setDefaultValue(IG_forward(v));
		return reset();
	}

	// If serialized types match, no need to copy/convert values
	constexpr const T& serialize() const requires (sameSerializedType) { return value(); }
	constexpr bool unserialize(auto &&v) requires (sameSerializedType) { return set(IG_forward(v)); }
	constexpr auto serialize() const requires (!sameSerializedType) { return SerializedType(value()); }
	constexpr bool unserialize(auto &&v) requires (!sameSerializedType) { return set(T(IG_forward(v))); }

private:
	T value_{desc.defaultValue};
	[[no_unique_address]] std::conditional_t<desc.mutableDefault, T,
		ConstantType<T, desc.defaultValue, uid>> defaultValue_{desc.defaultValue};
};

template<bool condition, class T, auto uid, PropertyDesc<T> desc = PropertyDesc<T>{}>
using ConditionalPropertyImpl = std::conditional_t<condition, Property<T, uid, desc>, ConstantType<T, desc.defaultValue, uid>>;

#define ConditionalProperty [[no_unique_address]] ConditionalPropertyImpl

}
