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

#ifdef IG_GL_API_OGL
#include <imagine/util/opengl/glDefs.h>
#elif defined IG_GL_API_OGL_ES
#include <imagine/util/opengl/glesDefs.h>
#else
	#if defined CONFIG_OS_IOS || defined __ANDROID__
	#include <imagine/util/opengl/glesDefs.h>
	#else
	#include <imagine/util/opengl/glDefs.h>
	#endif
#endif

#include <imagine/util/ctype.hh>
#include <imagine/util/ranges.hh>
#include <cstdio>
#include <string_view>

namespace IG
{

inline int glVersionFromStr(const char* versionStr)
{
	// skip to version number
	while(!isDigit(*versionStr) && *versionStr != '\0')
		versionStr++;
	int major = 1, minor = 0;
	if(sscanf(versionStr, "%d.%d", &major, &minor) != 2)
	{
		return 10 * major;
	}
	return 10 * major + minor;
}

inline void forEachOpenGLExtension(auto &&func)
{
	#ifdef IG_GL_API_OGL
	GLint numExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	for(auto i : iotaCount(numExtensions))
	{
		auto extStr = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		func(std::string_view{extStr});
	}
	#else
	auto extStr = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	for(auto s: std::string_view{extStr} | std::views::split(' '))
	{
		func(std::string_view{s});
	}
	#endif
}

}
