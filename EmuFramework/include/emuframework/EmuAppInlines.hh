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

#include <meta.h>
#include <imagine/config/version.h>

const char *const Base::ApplicationContext::applicationName{CONFIG_APP_NAME};
const char *const Base::ApplicationContext::applicationId{CONFIG_APP_ID};

const char *appViewTitle()
{
	return CONFIG_APP_NAME " " IMAGINE_VERSION;
}

bool hasGooglePlayStoreFeatures()
{
	#if defined __ANDROID__ && defined CONFIG_GOOGLE_PLAY_STORE
	return true;
	#else
	return false;
	#endif
}
