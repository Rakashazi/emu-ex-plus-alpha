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

#if defined __APPLE__ && !defined __ARM_ARCH_6K__
#define CONFIG_GFX_MATH_GLKIT
#else
#define CONFIG_GFX_MATH_GLM
#endif

namespace Config
{
	namespace Gfx
	{
	#ifndef CONFIG_GFX_OPENGL_ES
		#if defined CONFIG_BASE_IOS || defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
		#define CONFIG_GFX_OPENGL_ES
		#endif
	#endif

	#if defined CONFIG_GFX_OPENGL_ES && !defined CONFIG_GFX_OPENGL_ES_MAJOR_VERSION
	#error "Configuration error, CONFIG_GFX_OPENGL_ES set but CONFIG_GFX_OPENGL_ES_MAJOR_VERSION unset"
	#endif

	#ifdef CONFIG_GFX_OPENGL_ES_MAJOR_VERSION
	static constexpr int OPENGL_ES = CONFIG_GFX_OPENGL_ES_MAJOR_VERSION;
	#else
	static constexpr int OPENGL_ES = 0;
	#define CONFIG_GFX_OPENGL_ES_MAJOR_VERSION 0
	#endif

	#if !defined CONFIG_BASE_MACOSX && \
	((defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1) || !defined CONFIG_GFX_OPENGL_ES)
	#define CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	static constexpr bool OPENGL_FIXED_FUNCTION_PIPELINE = true;
	#else
	static constexpr bool OPENGL_FIXED_FUNCTION_PIPELINE = false;
	#endif

	#if (defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 2) || !defined CONFIG_GFX_OPENGL_ES
	#define CONFIG_GFX_OPENGL_SHADER_PIPELINE
	static constexpr bool OPENGL_SHADER_PIPELINE = true;
	#else
	static constexpr bool OPENGL_SHADER_PIPELINE = false;
	#endif

	#if !defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE && !defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
	#error "Configuration error, OPENGL_FIXED_FUNCTION_PIPELINE & OPENGL_SHADER_PIPELINE both unset"
	#endif

	#ifdef CONFIG_GFX_ANDROID_SURFACE_TEXTURE
	#define CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
	static constexpr bool OPENGL_TEXTURE_TARGET_EXTERNAL = true;
	#else
	static constexpr bool OPENGL_TEXTURE_TARGET_EXTERNAL = false;
	#endif

	#if !defined NDEBUG && !defined __APPLE__
	#define CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	static constexpr bool OPENGL_DEBUG_CONTEXT = true;
	#else
	static constexpr bool OPENGL_DEBUG_CONTEXT = false;
	#endif

	#if defined CONFIG_BASE_IOS
	#define CONFIG_GFX_GLDRAWABLE_NEEDS_FRAMEBUFFER
	static constexpr bool GLDRAWABLE_NEEDS_FRAMEBUFFER = true;
	#else
	static constexpr bool GLDRAWABLE_NEEDS_FRAMEBUFFER = false;
	#endif
	}
}
