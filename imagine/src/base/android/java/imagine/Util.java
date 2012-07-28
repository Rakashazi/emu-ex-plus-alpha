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
