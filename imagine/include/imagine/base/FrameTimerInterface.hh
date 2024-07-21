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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/variant.hh>

namespace IG
{

template <class VariantBase>
class FrameTimerInterface : public VariantBase, public AddVisit
{
public:
	using VariantBase::VariantBase;
	using AddVisit::visit;

	void scheduleVSync() { visit([](auto &e){ e.scheduleVSync(); }); }
	void cancel() { visit([](auto &e){ e.cancel(); }); }
	void setFrameRate(FrameRate rate) { visit([&](auto &e){ e.setFrameRate(rate); }); }
	void setEventsOnThisThread(ApplicationContext ctx) { visit([&](auto &e){ e.setEventsOnThisThread(ctx); }); }
};

}
