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

#include <imagine/config/build.h>

#if !defined CONFIG_BLUETOOTH_SERVER
	#if defined CONFIG_BLUETOOTH_BLUEZ || defined CONFIG_BLUETOOTH_BTSTACK
	#define CONFIG_BLUETOOTH_SERVER
	#endif
#endif

#if defined CONFIG_BLUETOOTH_BLUEZ || defined CONFIG_BLUETOOTH_BTSTACK
#define CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
#define CONFIG_BLUETOOTH_SCAN_SECS
#endif
