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

static_assert(__has_feature(objc_arc), "This file requires ARC");
#import <Foundation/NSString.h>
#include <imagine/util/string/apple.h>
#include <cstring>

void precomposeUnicodeString(const char *src, char *dest, unsigned int destSize)
{
	auto decomp = [[NSString alloc] initWithBytesNoCopy:(void*)src length:strlen(src) encoding:NSUTF8StringEncoding freeWhenDone:false];
	@autoreleasepool
	{
		auto precomp = [decomp precomposedStringWithCanonicalMapping];
		strncpy(dest, [precomp UTF8String], destSize);
	}
}
