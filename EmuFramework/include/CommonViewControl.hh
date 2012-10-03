#pragma once

void refreshTouchConfigMenu()
{
#ifndef CONFIG_BASE_PS3
	if(viewStack.top() == &tcMenu)
		tcMenu.updatePositionVals();
#endif
}
