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

#define thisModuleName "vController"
#include <VController.hh>
#include <algorithm>

void VControllerDPad::init() {}

void VControllerDPad::setImg(Gfx::BufferImage *dpadR, GC texHeight)
{
	spr.init(-.5, -.5, .5, .5, dpadR);
	spr.setImg(dpadR, 0., 0., 1., 64./texHeight);
}

void VControllerDPad::updateBoundingAreaGfx()
{
	if(visualizeBounds && padArea.xSize())
	{
		mapPix.init(padArea.xSize(), padArea.ySize());
		mapImg.init(mapPix, 0, Gfx::BufferImage::LINEAR, 0);
		mapSpr.init(&mapImg);
		mapSpr.setPos(padArea);

		iterateTimes(mapPix.y, y)
			iterateTimes(mapPix.x, x)
			{
				int input = getInput(padArea.xPos(LT2DO) + x, padArea.yPos(LT2DO) + y);
				//logMsg("got input %d", input);
				*((uint16*)mapPix.getPixel(x,y)) = input == -1 ? PixelFormatRGB565.build(1., 0., 0., 1.)
										: IG::isOdd(input) ? PixelFormatRGB565.build(1., 1., 1., 1.)
										: PixelFormatRGB565.build(0., 1., 0., 1.);
			}
		mapImg.write(mapPix);
	}
}

void VControllerDPad::setDeadzone(int newDeadzone)
{
	if(deadzone != newDeadzone)
	{
		deadzone = newDeadzone;
		updateBoundingAreaGfx();
	}
}

void VControllerDPad::setDiagonalSensitivity(float newDiagonalSensitivity)
{
	if(diagonalSensitivity != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity = newDiagonalSensitivity;
		updateBoundingAreaGfx();
	}
}

IG::Rect2<int> VControllerDPad::bounds() const
{
	return padBaseArea;
}

void VControllerDPad::setSize(uint sizeInPixels)
{
	btnSizePixels = sizeInPixels;
	auto rect = IG::makeRectRel<int>(0, 0, btnSizePixels, btnSizePixels);
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {0, 0, int(padBaseArea.xSize()*1.5), int(padBaseArea.xSize()*1.5)};
	if(visualizeBounds)
	{
		if(changedSize)
			updateBoundingAreaGfx();
	}
}

void VControllerDPad::setPos(IG::Point2D<int> pos)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(Base::mainWindow().viewBounds());
	padBase = Gfx::unProjectRect(padBaseArea);
	spr.setPos(padBase);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea);
	}
}

void VControllerDPad::setBoundingAreaVisible(bool on)
{
	if(visualizeBounds == on)
		return;
	visualizeBounds = on;
	if(!on)
	{
		if(mapSpr.image())
		{
			logMsg("deallocating bounding box display resources");
			mapSpr.deinit();
			mapImg.deinit();
			mapPix.deinitManaged();
		}
	}
	else
	{
		updateBoundingAreaGfx();
	}
}

void VControllerDPad::draw()
{
	//{ gfx_resetTransforms(); GeomRect::draw(padArea); }
	spr.draw();

	if(visualizeBounds)
	{
		mapSpr.draw();
	}
}

int VControllerDPad::getInput(int cx, int cy)
{
	if(padArea.overlaps({cx, cy}))
	{
		int x = cx - padArea.xCenter(), y = cy - padArea.yCenter();
		int xDeadzone = deadzone, yDeadzone = deadzone;
		if(std::abs(x) > deadzone)
			yDeadzone += (std::abs(x) - deadzone)/diagonalSensitivity;
		if(std::abs(y) > deadzone)
			xDeadzone += (std::abs(y) - deadzone)/diagonalSensitivity;
		//logMsg("dpad offset %d,%d, deadzone %d,%d", x, y, xDeadzone, yDeadzone);
		int pad = 4; // init to center
		if(std::abs(x) > xDeadzone)
		{
			if(x > 0)
				pad = 5; // right
			else
				pad = 3; // left
		}
		if(std::abs(y) > yDeadzone)
		{
			if(y > 0)
				pad += 3; // shift to top row
			else
				pad -= 3; // shift to bottom row
		}
		return pad == 4 ? -1 : pad; // don't send center dpad push
	}
	return -1;
}

void VControllerKeyboard::init()
{
	mode = 0;
}

void VControllerKeyboard::updateImg()
{
	if(mode)
		spr.setImg(spr.image(), 0., .5, spr.image()->textureDesc().xEnd/*384./512.*/, 1.);
	else
		spr.setImg(spr.image(), 0., 0., spr.image()->textureDesc().xEnd/*384./512.*/, .5);
}

void VControllerKeyboard::setImg(Gfx::BufferImage *img)
{
	spr.init(-.5, -.5, .5, .5, img);
	updateImg();
}

void VControllerKeyboard::place(GC btnSize, GC yOffset)
{
	GC xSize, ySize;
	IG::setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, Gfx::proj.w));
	GC vArea = Gfx::proj.h - yOffset*2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	IG::Rect2<GC> boundGC {0., 0., xSize, ySize};
	boundGC.setPos({0., Gfx::proj.rect.y2 + yOffset}, CT2DO);
	spr.setPos(boundGC);
	bound = Gfx::projectRect(boundGC);
	keyXSize = (bound.xSize() / cols) + (bound.xSize() * (1./256.));
	keyYSize = bound.ySize() / 4;
	logMsg("key size %dx%d", keyXSize, keyYSize);
}

void VControllerKeyboard::draw()
{
	spr.draw();
}

int VControllerKeyboard::getInput(int cx, int cy)
{
	if(bound.overlaps({cx, cy}))
	{
		int relX = cx - bound.x, relY = cy - bound.y;
		uint row = relY/keyYSize;
		uint col;
		if((mode == 0 && row == 1) || row == 2)
		{
			relX -= keyXSize/2;
			col = relX/keyXSize;
			if(relX < 0)
				col = 0;
			else if(col > 8)
				col = 8;
		}
		else
		{
			if(row > 3)
				row = 3;
			col = relX/keyXSize;
			if(col > 9)
				col = 9;
		}
		uint idx = col + (row*cols);
		logMsg("pointer %d,%d key @ %d,%d, idx %d", relX, relY, row, col, idx);
		return idx;
	}
	return -1;
}

void VControllerGamepad::init(float alpha)
{
	dp.init();
}

void VControllerGamepad::setBoundingAreaVisible(bool on)
{
	showBoundingArea = on;
	dp.setBoundingAreaVisible(on);
}

bool VControllerGamepad::boundingAreaVisible()
{
	return showBoundingArea;
}

void VControllerGamepad::setImg(Gfx::BufferImage *pics)
{
	GC h = systemFaceBtns == 2 ? 128. : 256.;
	dp.setImg(pics, h);
	forEachInArray(centerBtnSpr, e)
	{
		e->init(pics);
	}
	centerBtnSpr[0].setImg(pics, 0., 65./h, 32./64., 81./h);
	if(systemCenterBtns == 2)
	{
		centerBtnSpr[1].setImg(pics, 33./64., 65./h, 1., 81./h);
	}

	forEachInArray(circleBtnSpr, e)
	{
		e->init(pics);
	}
	if(systemFaceBtns == 2)
	{
		circleBtnSpr[0].setImg(pics, 0., 82./h, 32./64., 114./h);
		circleBtnSpr[1].setImg(pics, 33./64., 83./h, 1., 114./h);
	}
	else // for tall overlay image
	{
		circleBtnSpr[0].setImg(pics, 0., 82./h, 32./64., 114./h);
		circleBtnSpr[1].setImg(pics, 33./64., 83./h, 1., 114./h);
		circleBtnSpr[2].setImg(pics, 0., 115./h, 32./64., 147./h);
		circleBtnSpr[3].setImg(pics, 33./64., 116./h, 1., 147./h);
		if(systemFaceBtns >= 6)
		{
			circleBtnSpr[4].setImg(pics, 0., 148./h, 32./64., 180./h);
			circleBtnSpr[5].setImg(pics, 33./64., 149./h, 1., 180./h);
		}
		if(systemFaceBtns == 8)
		{
			circleBtnSpr[6].setImg(pics, 0., 181./h, 32./64., 213./h);
			circleBtnSpr[7].setImg(pics, 33./64., 182./h, 1., 213./h);
		}
	}
}

IG::Rect2<int> VControllerGamepad::centerBtnBounds() const
{
	return centerBtnsBound;
}

void VControllerGamepad::setCenterBtnPos(IG::Point2D<int> pos)
{
	centerBtnsBound.setPos(pos, C2DO);
	centerBtnsBound.fitIn(Base::mainWindow().viewBounds());
	int buttonXSpace = btnSpacePixels;//btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
	int extraXSize = buttonXSpace + btnSizePixels * btnExtraXSize;
	int spriteYPos = centerBtnsBound.yCenter() - centerBtnsBound.ySize()/6;
	if(systemCenterBtns == 2)
	{
		centerBtnBound[0] = IG::makeRectRel<int>(centerBtnsBound.x - extraXSize/2, centerBtnsBound.y, btnSizePixels + extraXSize, centerBtnsBound.ySize());
		centerBtnBound[1] = IG::makeRectRel<int>((centerBtnsBound.x2 - btnSizePixels) - extraXSize/2, centerBtnsBound.y, btnSizePixels + extraXSize, centerBtnsBound.ySize());
		centerBtnSpr[0].setPos(Gfx::unProjectRect(IG::makeRectRel<int>(centerBtnsBound.x, spriteYPos, btnSizePixels, btnSizePixels/2)));
		centerBtnSpr[1].setPos(Gfx::unProjectRect(IG::makeRectRel<int>(centerBtnsBound.x2 - btnSizePixels, spriteYPos, btnSizePixels, btnSizePixels/2)));
	}
	else
	{
		centerBtnBound[0] = IG::makeRectRel<int>(centerBtnsBound.x - extraXSize/2, centerBtnsBound.y, btnSizePixels + extraXSize, centerBtnsBound.ySize());
		centerBtnSpr[0].setPos(Gfx::unProjectRect(IG::makeRectRel<int>(centerBtnsBound.x, spriteYPos, centerBtnsBound.xSize(), btnSizePixels/2)));
	}
}

static uint lTriggerIdx()
{
	return systemFaceBtns-2;
}

static uint rTriggerIdx()
{
	return systemFaceBtns-1;
}

IG::Rect2<int> VControllerGamepad::lTriggerBounds() const
{
	return lTriggerBound;
}

void VControllerGamepad::setLTriggerPos(IG::Point2D<int> pos)
{
	uint idx = lTriggerIdx();
	lTriggerBound.setPos(pos, C2DO);
	lTriggerBound.fitIn(Base::mainWindow().viewBounds());
	auto lTriggerAreaGC = Gfx::unProjectRect(lTriggerBound);
	faceBtnBound[idx] = lTriggerBound;
	circleBtnSpr[idx].setPos(lTriggerBound);
}

IG::Rect2<int> VControllerGamepad::rTriggerBounds() const
{
	return rTriggerBound;
}

void VControllerGamepad::setRTriggerPos(IG::Point2D<int> pos)
{
	uint idx = rTriggerIdx();
	rTriggerBound.setPos(pos, C2DO);
	rTriggerBound.fitIn(Base::mainWindow().viewBounds());
	auto rTriggerAreaGC = Gfx::unProjectRect(rTriggerBound);
	faceBtnBound[idx] = rTriggerBound;
	circleBtnSpr[idx].setPos(rTriggerBound);
}

void VControllerGamepad::layoutBtnRows(uint a[], uint btns, uint rows, IG::Point2D<int> pos)
{
	int btnsPerRow = btns/rows;
	//logMsg("laying out buttons with size %d, space %d, row shift %d, stagger %d", btnSizePixels, btnSpacePixels, btnRowShiftPixels, btnStaggerPixels);
	faceBtnsBound.setPos(pos, C2DO);
	faceBtnsBound.fitIn(Base::mainWindow().viewBounds());
	auto btnArea = Gfx::unProjectRect(faceBtnsBound);

	int row = 0, btnPos = 0;
	GC yOffset = (btnStagger < 0) ? -btnStagger*(btnsPerRow-1) : 0,
		xOffset = -btnRowShift*(rows-1),
		staggerOffset = 0;
	iterateTimes(btns, i)
	{
		auto faceBtn = IG::makeRectRel<GC>(
			(btnArea.xPos(LT2DO) + xOffset), (btnArea.yPos(LT2DO) + yOffset + staggerOffset), btnSize, btnSize);
		xOffset += btnSize + btnSpace;
		staggerOffset += btnStagger;
		if(++btnPos == btnsPerRow)
		{
			row++;
			yOffset += btnSize + btnSpace;
			staggerOffset = 0;
			xOffset = -btnRowShift*((rows-1)-row);
			btnPos = 0;
		}

		circleBtnSpr[a[i]].setPos(faceBtn);

		GC btnExtraYSizeVal = (rows == 1) ? btnExtraYSize : btnExtraYSizeMultiRow;
		uint buttonXSpace = btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
		uint buttonYSpace = btnExtraYSizeVal ? btnSpacePixels * 2 : btnSpacePixels;
		uint extraXSize = buttonXSpace + (GC)btnSizePixels * btnExtraXSize;
		uint extraYSize = buttonYSpace + (GC)btnSizePixels * btnExtraYSizeVal;
		auto &bound = faceBtnBound[a[i]];
		bound = Gfx::projectRect(faceBtn);
		bound = IG::makeRectRel<int>(bound.pos(LT2DO).x - extraXSize/2,
			bound.pos(LT2DO).y - extraYSize/2,
			bound.xSize() + extraXSize, bound.ySize() + extraYSize);
	}
}

IG::Rect2<int> VControllerGamepad::faceBtnBounds() const
{
	return faceBtnsBound;
}

uint VControllerGamepad::rowsForButtons(uint activeButtons)
{
	if(!systemHasTriggerBtns)
	{
		return (activeButtons > 3) ? 2 : 1;
	}
	else
	{
		return triggersInline ? 2 : (systemFaceBtns < 6) ? 1 : 2;
	}
}

void VControllerGamepad::setFaceBtnPos(IG::Point2D<int> pos)
{
	using namespace IG;

	if(!systemHasTriggerBtns)
	{
		uint btnMap[] 		{1, 0};
		uint btnMap2Rev[] {0, 1};
		uint btnMap3[] 		{0, 1, 2};
		uint btnMap4[] 		{0, 1, 2, 3};
		uint btnMap6Rev[] {0, 1, 2, 3, 4, 5};
		uint btnMap6[] 		{2, 1, 0, 3, 4, 5};
		uint rows = rowsForButtons(activeFaceBtns);
		if(activeFaceBtns == 6)
			layoutBtnRows(systemHasRevBtnLayout ? btnMap6Rev : btnMap6, sizeofArray(btnMap6), rows, pos);
		else if(activeFaceBtns == 4)
			layoutBtnRows(btnMap4, sizeofArray(btnMap4), rows, pos);
		else if(activeFaceBtns == 3)
			layoutBtnRows(btnMap3, sizeofArray(btnMap3), rows, pos);
		else
			layoutBtnRows(systemHasRevBtnLayout ? btnMap2Rev : btnMap, sizeofArray(btnMap), rows, pos);
	}
	else
	{
		if(triggersInline)
		{
			uint btnMap8[] {0, 1, 2, 6, 3, 4, 5, 7};
			uint btnMap6[] {1, 0, 5, 3, 2, 4};
			uint btnMap4[] {1, 0, 2, 3};
			if(systemFaceBtns == 8)
				layoutBtnRows(btnMap8, sizeofArray(btnMap8), 2, pos);
			else if(systemFaceBtns == 6)
				layoutBtnRows(btnMap6, sizeofArray(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, sizeofArray(btnMap4), 2, pos);
		}
		else
		{
			uint btnMap8[] {0, 1, 2, 3, 4, 5};
			uint btnMap6[] {1, 0, 3, 2};
			uint btnMap4[] {1, 0};
			if(systemFaceBtns == 8)
				layoutBtnRows(btnMap8, sizeofArray(btnMap8), 2, pos);
			else if(systemFaceBtns == 6)
				layoutBtnRows(btnMap6, sizeofArray(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, sizeofArray(btnMap4), 1, pos);
		}
	}
}

void VControllerGamepad::setBaseBtnSize(uint sizeInPixels)
{
	btnSizePixels = sizeInPixels;
	btnSize = Gfx::iXSize(sizeInPixels);
	dp.setSize(IG::makeEvenRoundedUp(int(sizeInPixels*2.5)));

	// face buttons
	uint btns = (systemHasTriggerBtns && !triggersInline) ? systemFaceBtns-2 : activeFaceBtns;
	uint rows = rowsForButtons(activeFaceBtns);
	int btnsPerRow = btns/rows;
	int xSizePixel = btnSizePixels*btnsPerRow + btnSpacePixels*(btnsPerRow-1) + std::abs(btnRowShiftPixels*((int)rows-1));
	int ySizePixel = btnSizePixels*rows + btnSpacePixels*(rows-1) + std::abs(btnStaggerPixels*((int)btnsPerRow-1));
	faceBtnsBound = IG::makeRectRel<int>(0, 0, xSizePixel, ySizePixel);

	// center buttons
	int cenBtnFullXSize = (systemCenterBtns == 2) ? (btnSizePixels*2) + btnSpacePixels : btnSizePixels;
	int cenBtnFullYSize = IG::makeEvenRoundedUp(int(btnSizePixels*1.25));
	centerBtnsBound = IG::makeRectRel<int>(0, 0, cenBtnFullXSize, cenBtnFullYSize);

	// triggers
	lTriggerBound = IG::makeRectRel<int>(0, 0, btnSizePixels, btnSizePixels);
	rTriggerBound = IG::makeRectRel<int>(0, 0, btnSizePixels, btnSizePixels);
}

void VControllerGamepad::getCenterBtnInput(int x, int y, int btnOut[2])
{
	uint count = 0;
	forEachInArray(centerBtnBound, e)
	{
		if(e->overlaps({x, y}))
		{
			//logMsg("overlaps %d", (int)e_i);
			btnOut[count] = e_i;
			count++;
			if(count == 2)
				return;
		}
	}
}

void VControllerGamepad::getBtnInput(int x, int y, int btnOut[2])
{
	uint count = 0;
	bool doSeparateTriggers = systemHasTriggerBtns && !triggersInline;
	if(faceBtnsState)
	{
		iterateTimes(doSeparateTriggers ? systemFaceBtns-2 : activeFaceBtns, i)
		{
			if(faceBtnBound[i].overlaps({x, y}))
			{
				//logMsg("overlaps %d", (int)e_i);
				btnOut[count] = i;
				count++;
				if(count == 2)
					return;
			}
		}
	}

	if(doSeparateTriggers)
	{
		if(lTriggerState)
		{
			if(faceBtnBound[lTriggerIdx()].overlaps({x, y}))
			{
				btnOut[count] = lTriggerIdx();
				count++;
				if(count == 2)
					return;
			}
		}
		if(rTriggerState)
		{
			if(faceBtnBound[rTriggerIdx()].overlaps({x, y}))
			{
				btnOut[count] = rTriggerIdx();
				count++;
				if(count == 2)
					return;
			}
		}
	}
}

void VControllerGamepad::draw(bool showHidden)
{
	using namespace Gfx;
	if(dp.state == 1 || (showHidden && dp.state))
	{
		dp.draw();
	}

	if(faceBtnsState == 1 || (showHidden && faceBtnsState))
	{
		//GeomRect::draw(faceBtnsBound);
		iterateTimes((systemHasTriggerBtns && !triggersInline) ? systemFaceBtns-2 : activeFaceBtns, i)
		{
			if(showBoundingArea)
				{ GeomRect::draw(faceBtnBound[i]); }
			circleBtnSpr[i].draw();
		}
	}

	if(systemHasTriggerBtns && !triggersInline)
	{
		if(lTriggerState == 1 || (showHidden && lTriggerState))
		{
			if(showBoundingArea)
				{ GeomRect::draw(faceBtnBound[lTriggerIdx()]); }
			circleBtnSpr[lTriggerIdx()].draw();
		}
		if(rTriggerState == 1 || (showHidden && rTriggerState))
		{
			if(showBoundingArea)
				{ GeomRect::draw(faceBtnBound[rTriggerIdx()]); }
			circleBtnSpr[rTriggerIdx()].draw();
		}
	}

	if(centerBtnsState == 1 || (showHidden && centerBtnsState))
	{
		forEachInArray(centerBtnSpr, e)
		{
			if(showBoundingArea)
				{ GeomRect::draw(centerBtnBound[e_i]); }
			e->draw();
		}
	}
}

GC VController::xMMSize(GC mm)
{
	return useScaledCoordinates ? Gfx::xSMMSize(mm) : Gfx::xMMSize(mm);
}

GC VController::yMMSize(GC mm)
{
	return useScaledCoordinates ? Gfx::ySMMSize(mm) : Gfx::yMMSize(mm);
}

int VController::xMMSizeToPixel(const Base::Window &win, GC mm)
{
	return useScaledCoordinates ? win.xSMMSizeToPixel(mm) : win.xMMSizeToPixel(mm);
}

int VController::yMMSizeToPixel(const Base::Window &win, GC mm)
{
	return useScaledCoordinates ? win.ySMMSizeToPixel(mm) : win.yMMSizeToPixel(mm);
}

bool VController::hasTriggers() const
{
	return systemHasTriggerBtns;
}

void VController::setImg(Gfx::BufferImage *pics)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setImg(pics);
	#endif
}

void VController::setBoundingAreaVisible(bool on)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBoundingAreaVisible(on);
	#endif
}

bool VController::boundingAreaVisible()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	return gp.boundingAreaVisible();
	#else
	return false;
	#endif
}

void VController::setMenuBtnPos(IG::Point2D<int> pos)
{
	menuBound.setPos(pos, C2DO);
	menuBound.fitIn(Base::mainWindow().viewBounds());
	menuBtnSpr.setPos(menuBound);
}

void VController::setFFBtnPos(IG::Point2D<int> pos)
{
	ffBound.setPos(pos, C2DO);
	ffBound.fitIn(Base::mainWindow().viewBounds());
	ffBtnSpr.setPos(ffBound);
}

void VController::setBaseBtnSize(uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels)
{
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	kb.place(Gfx::iYSize(gamepadBtnSizeInPixels), Gfx::iYSize(gamepadBtnSizeInPixels * .75));
	#endif
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBaseBtnSize(gamepadBtnSizeInPixels);
	#endif
	auto size = uiBtnSizeInPixels;
	if(menuBound.xSize() != (int)size)
		logMsg("set UI button size: %d", size);
	menuBound = IG::makeRectRel<int>(0, 0, size, size);
	ffBound = IG::makeRectRel<int>(0, 0, size, size);
}

void VController::inputAction(uint action, uint vBtn)
{
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	if(kbMode)
	{
		assert(vBtn < sizeofArray(kbMap));
		EmuSystem::handleInputAction(action, kbMap[vBtn]);
	}
	else
	#endif
	{
		assert(vBtn < sizeofArray(map));
		auto turbo = map[vBtn] & TURBO_BIT;
		auto keyCode = map[vBtn] & ACTION_MASK;
		if(turbo)
		{
			if(action == Input::PUSHED)
			{
				turboActions.addEvent(keyCode);
			}
			else
			{
				turboActions.removeEvent(keyCode);
			}
		}
		EmuSystem::handleInputAction(action, keyCode);
	}
}

void VController::resetInput(bool init)
{
	iterateTimes(Input::maxCursors, i)
	{
		iterateTimes(2, j)
		{
			if(!init && ptrElem[i][j] != -1) // release old key, if any
				inputAction(Input::RELEASED, ptrElem[i][j]);
			ptrElem[i][j] = -1;
			prevPtrElem[i][j] = -1;
		}
	}
}

void VController::init(float alpha, uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels)
{
	var_selfs(alpha);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.init(alpha);
	#endif
	setBaseBtnSize(gamepadBtnSizeInPixels, uiBtnSizeInPixels);
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	kb.init();
	kbMode = 0;
	#endif
	resetInput(1);
}

void VController::place()
{
	resetInput();
}

#ifdef CONFIG_VCONTROLLER_KEYBOARD
void VController::toggleKeyboard()
{
	logMsg("toggling keyboard");
	resetInput();
	toggle(kbMode);
}
#endif

void VController::findElementUnderPos(const Input::Event &e, int elemOut[2])
{
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	if(kbMode)
	{
		int kbChar = kb.getInput(e.x, e.y);
		if(kbChar == -1)
			return;
		if(kbChar == 30 && e.pushed())
		{
			logMsg("dismiss kb");
			toggleKeyboard();
		}
		else if((kbChar == 31 || kbChar == 32) && e.pushed())
		{
			logMsg("switch kb mode");
			toggle(kb.mode);
			kb.updateImg();
			resetInput();
			updateKeyboardMapping();
		}
		else
			elemOut[0] = kbChar;
		return;
	}
	#endif

	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(gp.centerBtnsState != 0)
	{
		int elem[2]= { -1, -1 };
		gp.getCenterBtnInput(e.x, e.y, elem);
		if(elem[0] != -1)
		{
			elemOut[0] = C_ELEM + elem[0];
			if(elem[1] != -1)
				elemOut[1] = C_ELEM + elem[1];
			return;
		}
	}

	{
		int elem[2]= { -1, -1 };
		gp.getBtnInput(e.x, e.y, elem);
		if(elem[0] != -1)
		{
			elemOut[0] = F_ELEM + elem[0];
			if(elem[1] != -1)
				elemOut[1] = F_ELEM + elem[1];
			return;
		}
	}

	if(gp.dp.state != 0)
	{
		int elem = gp.dp.getInput(e.x, e.y);
		if(elem != -1)
		{
			elemOut[0] = D_ELEM + elem;
			return;
		}
	}
	#endif
}

void VController::applyInput(const Input::Event &e)
{
	using namespace IG;
	assert(e.isPointer());
	auto drag = Input::dragState(e.devId);

	int elem[2] = { -1, -1 };
	if(drag->pushed) // make sure the cursor isn't hovering
		findElementUnderPos(e, elem);

	//logMsg("under %d %d", elem[0], elem[1]);

	// release old buttons
	iterateTimes(2, i)
	{
		auto vBtn = ptrElem[e.devId][i];
		if(vBtn != -1 && !mem_findFirstValue(elem, vBtn))
		{
			//logMsg("releasing %d", vBtn);
			inputAction(Input::RELEASED, vBtn);
		}
	}

	// push new buttons
	iterateTimes(2, i)
	{
		auto vBtn = elem[i];
		if(vBtn != -1 && !mem_findFirstValue(ptrElem[e.devId], vBtn))
		{
			//logMsg("pushing %d", vBtn);
			inputAction(Input::PUSHED, vBtn);
			if(optionVibrateOnPush)
			{
				Base::vibrate(32);
			}
		}
	}

	memcpy(ptrElem[e.devId], elem, sizeof(elem));
}

void VController::draw(bool emuSystemControls, bool activeFF, bool showHidden)
{
	draw(emuSystemControls, activeFF, showHidden, alpha);
}

void VController::draw(bool emuSystemControls, bool activeFF, bool showHidden, float alpha)
{
	using namespace Gfx;
	if(unlikely(alpha == 0.))
		return;
	//gfx_setBlendMode(GFX_BLEND_MODE_INTENSITY);
	setImgMode(IMG_MODE_MODULATE);
	setBlendMode(BLEND_MODE_ALPHA);
	setColor(1., 1., 1., alpha);

	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	if(kbMode)
		kb.draw();
	else
	#endif
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(emuSystemControls)
		gp.draw(showHidden);
	#endif
	//GeomRect::draw(menuBound);
	//GeomRect::draw(ffBound);
	if(menuBtnState == 1 || (showHidden && menuBtnState))
		menuBtnSpr.draw();
	if(ffBtnState == 1 || (showHidden && ffBtnState))
	{
		if(activeFF)
			setColor(1., 0., 0., alpha);
		ffBtnSpr.draw();
	}
}

int VController::numElements() const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	return (systemHasTriggerBtns && !gp.triggersInline) ? 7 : 5;
	#else
	return 5;
	#endif
}

IG::Rect2<int> VController::bounds(int elemIdx) const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dp.bounds();
		case 1: return gp.centerBtnBounds();
		case 2: return gp.faceBtnBounds();
		case 3: return menuBound;
		case 4: return ffBound;
		case 5: return gp.lTriggerBounds();
		case 6: return gp.rTriggerBounds();
		default: bug_branch("%d", elemIdx); return {0,0,0,0};
	}
	#else
	switch(elemIdx)
	{
		case 0: return {0,0,0,0};
		case 1: return {0,0,0,0};
		case 2: return {0,0,0,0};
		case 3: return menuBound;
		case 4: return ffBound;
		default: bug_branch("%d", elemIdx); return {0,0,0,0};
	}
	#endif
}

void VController::setPos(int elemIdx, IG::Point2D<int> pos)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dp.setPos(pos);
		bcase 1: gp.setCenterBtnPos(pos);
		bcase 2: gp.setFaceBtnPos(pos);
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bcase 5: gp.setLTriggerPos(pos);
		bcase 6: gp.setRTriggerPos(pos);
		bdefault: bug_branch("%d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bdefault: bug_branch("%d", elemIdx);
	}
	#endif
}

void VController::setState(int elemIdx, uint state)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dp.state = state;
		bcase 1: gp.centerBtnsState = state;
		bcase 2: gp.faceBtnsState = state;
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bcase 5: gp.lTriggerState = state;
		bcase 6: gp.rTriggerState = state;
		bdefault: bug_branch("%d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bdefault: bug_branch("%d", elemIdx);
	}
	#endif
}

uint VController::state(int elemIdx)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dp.state;
		case 1: return gp.centerBtnsState;
		case 2: return gp.faceBtnsState;
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		case 5: return gp.lTriggerState;
		case 6: return gp.rTriggerState;
		default: bug_branch("%d", elemIdx); return 0;
	}
	#else
	switch(elemIdx)
	{
		case 0: return 0;
		case 1: return 0;
		case 2: return 0;
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		default: bug_branch("%d", elemIdx); return 0;
	}
	#endif
}

void VController::updateMapping(uint player)
{
	updateVControllerMapping(player, map);
}

#ifdef CONFIG_VCONTROLLER_KEYBOARD
void VController::updateKeyboardMapping()
{
	updateVControllerKeyboardMapping(kb.mode, kbMap);
}
#endif
