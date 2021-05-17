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

#include <imagine/config/defs.hh>
#include <imagine/time/Time.hh>
#include <variant>

namespace Base
{

class FrameTimerI
{
public:
	virtual ~FrameTimerI();
	virtual void scheduleVSync() = 0;
	virtual void cancel();
	virtual void setFrameTime(IG::FloatSeconds rate);
};

template <class VariantBase>
class FrameTimerVariantWrapper : public VariantBase
{
public:
	using VariantBase::VariantBase;

	constexpr VariantBase &asVariant()
	{
		return static_cast<VariantBase&>(*this);
	}

	constexpr void scheduleVSync()
	{
		std::visit([](auto &&e){ e.scheduleVSync(); }, asVariant());
	}

	constexpr void cancel()
	{
		std::visit([](auto &&e){ e.cancel(); }, asVariant());
	}

	constexpr void setFrameTime(IG::FloatSeconds rate)
	{
		std::visit([&](auto &&e){ e.setFrameTime(rate); }, asVariant());
	}
};

}
