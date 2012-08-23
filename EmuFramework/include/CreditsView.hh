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

#pragma once
#include <gfx/GfxText.hh>
#include <gui/View.hh>
#include <util/gui/ViewAnimation.hh>
#include <config/version.h>
#include <meta.h>

#ifdef NDEBUG
	#define OTHER_SUFFIX_STR
#else
	#define OTHER_SUFFIX_STR "D"
#endif

#ifdef CONFIG_BASE_IOS
	#ifdef __ARM_ARCH_6K__
		#define ARCH_STR "armv6"
	#else
		#define ARCH_STR "armv7"
	#endif
#elif defined(CONFIG_ENV_WEBOS)
	#ifdef __ARM_ARCH_6J__
		#define ARCH_STR "armv6"
	#else
		#define ARCH_STR "armv7"
	#endif
#elif defined(CONFIG_BASE_ANDROID)
	#ifdef __ARM_ARCH_7A__
		#ifdef __ARM_NEON__
			#define ARCH_STR "armv7-neon"
		#else
			#define ARCH_STR "armv7"
		#endif
	#elif defined __i386__
		#define ARCH_STR "x86"
	#else
		#define ARCH_STR "armv6"
	#endif
#else
	#define ARCH_STR
#endif

#ifdef ENV_NOTE
	#define PLATFORM_INFO_STR ENV_NOTE " (" ARCH_STR ")"
#else
	#define PLATFORM_INFO_STR "(" ARCH_STR ")"
#endif
#define CREDITS_INFO_STRING "Built : " __DATE__ "\n" PLATFORM_INFO_STR "\n\n"

class CreditsView : public View
{
public:
	constexpr CreditsView(const char *str): View(CONFIG_APP_NAME " " IMAGINE_VERSION), str(str) { }
	GfxText text;
	FadeViewAnimation<10> fade;
	const char *str;
	Rect2<int> rect;
	Rect2<int> &viewRect() { return rect; }
	void draw ();
	void place ();
	void inputEvent(const InputEvent &e);
	void init ();
	void deinit ();
};
