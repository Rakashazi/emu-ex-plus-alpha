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

#include <TouchConfigView.hh>

//static const char *ctrlPosStr[] =
//{
//	"Top-Left", "Mid-Left", "Bottom-Left", "Top-Right", "Mid-Right", "Bottom-Right", "Top", "Bottom", "Off"
//};

static uint alphaToMenuOption(int alpha)
{
	if(alpha == 0)
		return 0;
	else if(alpha == int(255 * .1))
		return 1;
	else if(alpha == int(255 * .25))
		return 2;
	else if(alpha == int(255 * .50))
		return 3;
	else if(alpha == int(255 * .65))
		return 4;
	else
		return 5;
}

void alphaSet(MultiChoiceMenuItem &, int val)
{
	if(val == 0)
	{
		optionTouchCtrlAlpha = 0;
	}
	else if(val == 1)
	{
		optionTouchCtrlAlpha = 255 * .1;
	}
	else if(val == 2)
	{
		optionTouchCtrlAlpha = 255 * .25;
	}
	else if(val == 3)
	{
		optionTouchCtrlAlpha = 255 * .5;
	}
	else if(val == 4)
	{
		optionTouchCtrlAlpha = 255 * .65;
	}
	else
	{
		optionTouchCtrlAlpha = 255 * .75;
	}
	vController.alpha = (int)optionTouchCtrlAlpha / 255.0;
}

void TouchConfigView::alphaInit()
{
	static const char *str[] =
	{
		"0%", "10%", "25%", "50%", "65%", "75%"
	};
	alpha.init("Blend Amount", str, alphaToMenuOption(optionTouchCtrlAlpha), sizeofArray(str), 0);
	alpha.valueDelegate().bind<&alphaSet>();
}

void sizeSet(MultiChoiceMenuItem &, int val)
{
	if(val == 0)
		optionTouchCtrlSize.val = 650;
	else if(val == 1)
		optionTouchCtrlSize.val = 700;
	else if(val == 2)
		optionTouchCtrlSize.val = 750;
	else if(val == 3)
		optionTouchCtrlSize.val = 800;
	else if(val == 4)
		optionTouchCtrlSize.val = 850;
	else if(val == 5)
		optionTouchCtrlSize.val = 900;
	else if(val == 6)
		optionTouchCtrlSize.val = 1000;
	else if(val == 7)
		optionTouchCtrlSize.val = 1200;
	else if(val == 8)
		optionTouchCtrlSize.val = 1400;
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::sizeInit()
{
	static const char *str[] =
	{
		"6.5mm", "7mm", "7.5mm", "8mm", "8.5mm", "9mm", "10mm", "12mm", "14mm"
	};
	int init = 0;
	if(optionTouchCtrlSize == 650U)
		init = 0;
	else if(optionTouchCtrlSize == 700U)
		init = 1;
	else if(optionTouchCtrlSize == 750U)
		init = 2;
	else if(optionTouchCtrlSize == 800U)
		init = 3;
	else if(optionTouchCtrlSize == 850U)
		init = 4;
	else if(optionTouchCtrlSize == 900U)
		init = 5;
	else if(optionTouchCtrlSize == 1000U)
		init = 6;
	else if(optionTouchCtrlSize == 1200U)
		init = 7;
	else if(optionTouchCtrlSize == 1400U)
		init = 8;
	size.init("Button Size", str, init, sizeofArray(str), 0);
	size.valueDelegate().bind<&sizeSet>();
}

void deadzoneSet(MultiChoiceMenuItem &, int val)
{
	if(val == 0)
		optionTouchDpadDeadzone.val = 100;
	else if(val == 1)
		optionTouchDpadDeadzone.val = 135;
	else if(val == 2)
		optionTouchDpadDeadzone.val = 160;
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::deadzoneInit()
{
	static const char *str[] =
	{
		"1mm", "1.35mm", "1.6mm"
	};
	int init = 0;
	if(optionTouchDpadDeadzone == 100U)
		init = 0;
	else if(optionTouchDpadDeadzone == 135U)
		init = 1;
	else if(optionTouchDpadDeadzone == 160U)
		init = 2;
	deadzone.init("D-Pad Deadzone", str, init, sizeofArray(str), 0);
	deadzone.valueDelegate().bind<&deadzoneSet>();
}

void diagonalSensitivitySet(MultiChoiceMenuItem &, int val)
{
	if(val == 0)
		optionTouchDpadDiagonalSensitivity.val = 1000;
	else if(val == 1)
		optionTouchDpadDiagonalSensitivity.val = 1500;
	else if(val == 2)
		optionTouchDpadDiagonalSensitivity.val = 1750;
	else if(val == 3)
		optionTouchDpadDiagonalSensitivity.val = 2000;
	else if(val == 4)
		optionTouchDpadDiagonalSensitivity.val = 2500;
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::diagonalSensitivityInit()
{
	static const char *str[] =
	{
		"None", "Low", "M-Low","Med.", "High"
	};
	int init = 0;
	if(optionTouchDpadDiagonalSensitivity == 1500U)
		init = 1;
	else if(optionTouchDpadDiagonalSensitivity == 1750U)
		init = 2;
	else if(optionTouchDpadDiagonalSensitivity == 2000U)
		init = 3;
	else if(optionTouchDpadDiagonalSensitivity == 2500U)
		init = 4;
	diagonalSensitivity.init("Diagonal Sensitivity", str, init, sizeofArray(str), 0);
	diagonalSensitivity.valueDelegate().bind<&diagonalSensitivitySet>();
}

void btnSpaceSet(MultiChoiceMenuItem &, int val)
{
	if(val == 0)
		optionTouchCtrlBtnSpace.val = 100;
	else if(val == 1)
		optionTouchCtrlBtnSpace.val = 200;
	else if(val == 2)
		optionTouchCtrlBtnSpace.val = 300;
	else if(val == 3)
		optionTouchCtrlBtnSpace.val = 400;
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::btnSpaceInit()
{
	static const char *str[] =
	{
		"1mm", "2mm", "3mm", "4mm"
	};
	int init = 0;
	if(optionTouchCtrlBtnSpace == 100U)
		init = 0;
	else if(optionTouchCtrlBtnSpace == 200U)
		init = 1;
	else if(optionTouchCtrlBtnSpace == 300U)
		init = 2;
	else if(optionTouchCtrlBtnSpace == 400U)
		init = 3;
	btnSpace.init("Button Spacing", str, init, sizeofArray(str), 0);
	btnSpace.valueDelegate().bind<&btnSpaceSet>();
}

void btnExtraXSizeSet(MultiChoiceMenuItem &, int val)
{
	switch(val)
	{
		bdefault: optionTouchCtrlExtraXBtnSize = 0;
		bcase 1: optionTouchCtrlExtraXBtnSize = 1;
		bcase 2: optionTouchCtrlExtraXBtnSize = 200;
		bcase 3: optionTouchCtrlExtraXBtnSize = 500;
	}
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::btnExtraXSizeInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "10%", "25%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraXBtnSize)
	{
		bcase 1: init = 1;
		bcase 200: init = 2;
		bcase 500: init = 3;
	}
	btnExtraXSize.init("H Overlap", str, init, sizeofArray(str), 0);
	btnExtraXSize.valueDelegate().bind<&btnExtraXSizeSet>();
}

void btnExtraYSizeMultiRowSet(MultiChoiceMenuItem &, int val)
{
	switch(val)
	{
		bdefault: optionTouchCtrlExtraYBtnSizeMultiRow = 0;
		bcase 1: optionTouchCtrlExtraYBtnSizeMultiRow = 1;
		bcase 2: optionTouchCtrlExtraYBtnSizeMultiRow = 200;
		bcase 3: optionTouchCtrlExtraYBtnSizeMultiRow = 500;
	}
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::btnExtraYSizeMultiRowInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "10%", "25%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraYBtnSizeMultiRow)
	{
		bcase 1: init = 1;
		bcase 200: init = 2;
		bcase 500: init = 3;
	}
	btnExtraYSizeMultiRow.init((systemFaceBtns == 4 || (systemFaceBtns >= 6 && systemHasTriggerBtns)) ? "V Overlap" : "V Overlap (2 rows)", str, init, sizeofArray(str), 0);
	btnExtraYSizeMultiRow.valueDelegate().bind<&btnExtraYSizeMultiRowSet>();
}

void btnExtraYSizeSet(MultiChoiceMenuItem &, int val)
{
	switch(val)
	{
		bdefault: optionTouchCtrlExtraYBtnSize = 0;
		bcase 1: optionTouchCtrlExtraYBtnSize = 1;
		bcase 2: optionTouchCtrlExtraYBtnSize = 500;
		bcase 3: optionTouchCtrlExtraYBtnSize = 1000;
	}
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::btnExtraYSizeInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "25%", "50%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraYBtnSize)
	{
		bcase 1: init = 1;
		bcase 500: init = 2;
		bcase 1000: init = 3;
	}
	btnExtraYSize.init("V Overlap", str, init, sizeofArray(str), 0);
	btnExtraYSize.valueDelegate().bind<&btnExtraYSizeSet>();
}

void btnStaggerSet(MultiChoiceMenuItem &, int val)
{
	optionTouchCtrlBtnStagger.val = val;
	setupVControllerVars();
	vController.place();
}

void TouchConfigView::btnStaggerInit()
{
	static const char *str[] =
	{
			"-0.75x V", "-0.5x V", "0", "0.5x V", "0.75x V", "1x H&V"
	};
	assert(optionTouchCtrlBtnStagger < sizeofArray(str));
	int init = optionTouchCtrlBtnStagger;
	btnStagger.init("Button Stagger", str, init, sizeofArray(str), 0);
	btnStagger.valueDelegate().bind<&btnStaggerSet>();
}

void imageResolutionHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	uint newRes = item.on ? 128 : 64;
	if(optionTouchCtrlImgRes != newRes)
	{
		optionTouchCtrlImgRes = newRes;
		updateVControlImg();
		vController.place();
	}
}

BoolMenuItem boundingBoxes;

void boundingBoxesHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	optionTouchCtrlBoundingBoxes = item.on;
	setupVControllerVars();
	Base::displayNeedsUpdate();
}

void vibrateHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	optionVibrateOnPush = item.on;
}

void showMenuIconHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	optionShowMenuIcon = item.on;
}

void TouchConfigView::init(bool highlightFirst)
{
	assert(faceBtnName);
	assert(centerBtnName);

	uint i = 0;
	//timeout.init(); text[i++] = &timeout;
	alphaInit(); text[i++] = &alpha;
	sizeInit(); text[i++] = &size;
	deadzoneInit(); text[i++] = &deadzone;
	diagonalSensitivityInit(); text[i++] = &diagonalSensitivity;
	menuPos.init(); text[i++] = &menuPos;
	showMenuIcon.init(optionShowMenuIcon); text[i++] = &showMenuIcon;
	showMenuIcon.selectDelegate().bind<&showMenuIconHandler>();
	ffPos.init(); text[i++] = &ffPos;
	dPadPos.init(); text[i++] = &dPadPos;
	faceBtnPos.init(faceBtnName); text[i++] = &faceBtnPos;
	centerBtnPos.init(centerBtnName); text[i++] = &centerBtnPos;
	if(vController.hasTriggers())
	{
		triggerPos.init(); text[i++] = &triggerPos;
	}
	btnSpaceInit(); text[i++] = &btnSpace;
	btnStaggerInit(); text[i++] = &btnStagger;
	btnExtraXSizeInit(); text[i++] = &btnExtraXSize;
	if(systemFaceBtns < 4 || (systemFaceBtns == 6 && !systemHasTriggerBtns))
	{
		btnExtraYSizeInit(); text[i++] = &btnExtraYSize;
	}
	if(systemFaceBtns >= 4)
	{
		btnExtraYSizeMultiRowInit(); text[i++] = &btnExtraYSizeMultiRow;
	}
	boundingBoxes.init(optionTouchCtrlBoundingBoxes); text[i++] = &boundingBoxes;
	boundingBoxes.selectDelegate().bind<&boundingBoxesHandler>();
	if(!optionVibrateOnPush.isConst)
	{
		vibrate.init(optionVibrateOnPush); text[i++] = &vibrate;
		vibrate.selectDelegate().bind<&vibrateHandler>();
	}
	if(!optionTouchCtrlImgRes.isConst)
	{
		imageResolution.init(optionTouchCtrlImgRes == 128U ? 1 : 0); text[i++] = &imageResolution;
		imageResolution.selectDelegate().bind<&imageResolutionHandler>();
	}
	assert(i <= sizeofArray(text));
	BaseMenuView::init(text, i, highlightFirst);
}

void TouchConfigView::draw()
{
	using namespace Gfx;
	resetTransforms();
	vController.draw(.5);
	BaseMenuView::draw();
}

void TouchConfigView::updatePositionVals()
{
	dPadPos.updateVal();
	faceBtnPos.updateVal();
	centerBtnPos.updateVal();
	ffPos.updateVal();
	menuPos.updateVal();
	if(vController.hasTriggers())
	{
		triggerPos.updateVal();
	}
}
