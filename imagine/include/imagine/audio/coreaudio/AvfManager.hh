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

namespace IG
{
class ApplicationContext;
}

namespace IG::Audio
{

class AvfManager
{
public:
	static constexpr bool HAS_SOLO_MIX = true;
	static constexpr bool SOLO_MIX_DEFAULT = true;

	AvfManager(ApplicationContext);

protected:
	#ifdef __OBJC__
	static_assert(sizeof(id) == sizeof(void*));
	using Id = id;
	#else
	using Id = void*;
	#endif
	#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000
	Id sessionInterruptionObserver{};
	#endif
	bool soloMix_{SOLO_MIX_DEFAULT};
	bool sessionActive{};
};

using ManagerImpl = AvfManager;

}
