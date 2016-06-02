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

package com.imagine;

import java.lang.reflect.*;

final class Util
{
	static Method getMethod(Class<?> cls, String name, Class<?>[] args)
	{
		try
		{
			return cls.getMethod(name, args);
		}
		catch (Exception ex)
		{
			return null;
		}
	}
	
	static Constructor<?> getConstructor(Class<?> cls, Class<?>[] args)
	{
		try
		{
			Constructor<?> c = cls.getDeclaredConstructor(args);
			if (!c.isAccessible())
				c.setAccessible(true);
			return c;
		}
		catch (Exception ex)
		{
			return null;
		}
	}
}
