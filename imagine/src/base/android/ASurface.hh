#pragma once

class ASurface
{
public:
	static constexpr int ROTATION_0 = 0, ROTATION_90 = 1, ROTATION_180 = 2, ROTATION_270 = 3;

	static bool isSidewaysOrientation(int o)
	{
		return o == ROTATION_90 || o == ROTATION_270;
	}

	static bool isStraightOrientation(int o)
	{
		return o == ROTATION_0 || o == ROTATION_180;
	}

	static bool orientationsAre90DegreesAway(int o1, int o2)
	{
		switch(o1)
		{
			default: bug_branch("%d", o1);
			case ROTATION_0:
			case ROTATION_180:
				return isSidewaysOrientation(o2);
			case ROTATION_90:
			case ROTATION_270:
				return isStraightOrientation(o2);
		}
	}
};
