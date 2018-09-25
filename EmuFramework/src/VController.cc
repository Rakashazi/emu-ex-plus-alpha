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

#define LOGTAG "VController"
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include "EmuOptions.hh"
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/math/space.hh>
#include "private.hh"
#include "privateInput.hh"

static constexpr uint TOGGLE_KEYBOARD = 65536;
static constexpr uint CHANGE_KEYBOARD_MODE = 65537;

void VControllerDPad::setImg(Gfx::Renderer &r, Gfx::PixmapTexture &dpadR, Gfx::GTexC texHeight)
{
	using namespace Gfx;
	spr.init({-.5, -.5, .5, .5,});
	spr.setImg(&dpadR, {0., 0., 1., 64._gtexc/texHeight});
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r)
{
	if(visualizeBounds && padArea.xSize())
	{
		IG::MemPixmap mapPix{{padArea.size(), IG::PIXEL_FMT_RGB565}};
		iterateTimes(mapPix.h(), y)
			iterateTimes(mapPix.w(), x)
			{
				int input = getInput({padArea.xPos(LT2DO) + (int)x, padArea.yPos(LT2DO) + (int)y});
				//logMsg("got input %d", input);
				*((uint16*)mapPix.pixel({(int)x, (int)y})) = input == -1 ? IG::PIXEL_DESC_RGB565.build(1., 0., 0., 1.)
										: IG::isOdd(input) ? IG::PIXEL_DESC_RGB565.build(1., 1., 1., 1.)
										: IG::PIXEL_DESC_RGB565.build(0., 1., 0., 1.);
			}
		mapImg = r.makePixmapTexture({mapPix});
		mapImg.write(0, mapPix, {});
		mapSpr.init({}, mapImg);
		mapSpr.setPos(padArea, mainWin.projectionPlane);
	}
}

void VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone)
{
	if(deadzone != newDeadzone)
	{
		deadzone = newDeadzone;
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity)
{
	if(diagonalSensitivity != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity = newDiagonalSensitivity;
		updateBoundingAreaGfx(r);
	}
}

IG::WindowRect VControllerDPad::bounds() const
{
	return padBaseArea;
}

void VControllerDPad::setSize(Gfx::Renderer &r, uint sizeInPixels)
{
	//logMsg("set dpad pixel size: %d", sizeInPixels);
	btnSizePixels = sizeInPixels;
	auto rect = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {0, 0, int(padBaseArea.xSize()*1.5), int(padBaseArea.xSize()*1.5)};
	if(visualizeBounds)
	{
		if(changedSize)
			updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setPos(IG::Point2D<int> pos)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(mainWin.viewport().bounds());
	padBase = mainWin.projectionPlane.unProjectRect(padBaseArea);
	spr.setPos(padBase);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea, mainWin.projectionPlane);
	}
}

void VControllerDPad::setBoundingAreaVisible(Gfx::Renderer &r, bool on)
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
		}
	}
	else
	{
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::draw(Gfx::RendererCommands &cmds) const
{
	cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
	spr.setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
	spr.draw(cmds);

	if(visualizeBounds)
	{
		mapSpr.setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
		mapSpr.draw(cmds);
	}
}

int VControllerDPad::getInput(IG::WP c) const
{
	if(padArea.overlaps(c))
	{
		int x = c.x - padArea.xCenter(), y = c.y - padArea.yCenter();
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

void VControllerKeyboard::updateImg(Gfx::Renderer &r)
{
	if(mode_)
		spr.setUVBounds({0., .5, texXEnd, 1.});
	else
		spr.setUVBounds({0., 0., texXEnd, .5});
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerKeyboard::setImg(Gfx::Renderer &r, Gfx::PixmapTexture *img)
{
	spr.init({-.5, -.5, .5, .5}, *img);
	texXEnd = img->uvBounds().x2;
	updateImg(r);
}

void VControllerKeyboard::place(Gfx::GC btnSize, Gfx::GC yOffset)
{
	Gfx::GC xSize, ySize;
	IG::setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, mainWin.projectionPlane.w));
	Gfx::GC vArea = mainWin.projectionPlane.h - yOffset*2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	Gfx::GCRect boundGC {0., 0., xSize, ySize};
	boundGC.setPos({0., mainWin.projectionPlane.bounds().y + yOffset}, CB2DO);
	spr.setPos(boundGC);
	bound = mainWin.projectionPlane.projectRect(boundGC);
	keyXSize = std::max(bound.xSize() / VKEY_COLS, 1u);
	keyYSize = std::max(bound.ySize() / KEY_ROWS, 1u);
	logMsg("key size %dx%d", keyXSize, keyYSize);
}

void VControllerKeyboard::draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP) const
{
	if(spr.image()->levels() > 1)
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
	else
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
	spr.setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
	spr.draw(cmds);
	if(selected.x != -1)
	{
		cmds.setColor(.2, .71, .9, 1./3.);
		cmds.setCommonProgram(Gfx::CommonProgram::NO_TEX, projP.makeTranslate());
		IG::WindowRect rect{};
		rect.x = bound.x + (selected.x * keyXSize);
		rect.x2 = bound.x + ((selected.x2 + 1) * keyXSize);
		rect.y = bound.y + (selected.y * keyYSize);
		rect.y2 = rect.y + keyYSize;
		Gfx::GeomRect::draw(cmds, rect, projP);
	}
}

int VControllerKeyboard::getInput(IG::WP c) const
{
	if(!bound.overlaps(c))
		return -1;
	int relX = c.x - bound.x, relY = c.y - bound.y;
	uint row = std::min(relY/keyYSize, 3u);
	uint col = std::min(relX/keyXSize, 19u);
	uint idx = col + (row * VKEY_COLS);
	logMsg("pointer %d,%d key @ %d,%d, idx %d", relX, relY, row, col, idx);
	return idx;
}

int VControllerKeyboard::translateInput(uint idx) const
{
	assumeExpr(idx < VKEY_COLS * KEY_ROWS);
	return table[0][idx];
}

bool VControllerKeyboard::keyInput(VController &v, Gfx::Renderer &r, Input::Event e)
{
	if(selected.x == -1)
	{
		if(e.pushed() && (e.isDefaultConfirmButton() || e.isDefaultDirectionButton()))
		{
			selectKey(0, 3);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if(e.isDefaultConfirmButton())
	{
		if(currentKey() == TOGGLE_KEYBOARD)
		{
			if(!e.pushed() || e.repeated())
				return false;
			logMsg("dismiss kb");
			unselectKey();
			v.toggleKeyboard();
		}
		else if(currentKey() == CHANGE_KEYBOARD_MODE)
		{
			if(!e.pushed() || e.repeated())
				return false;
			logMsg("switch kb mode");
			setMode(r, mode() ^ true);
			v.resetInput();
		}
		else if(e.pushed())
		{
			EmuSystem::handleInputAction(Input::PUSHED, currentKey());
		}
		else
		{
			EmuSystem::handleInputAction(Input::RELEASED, currentKey());
		}
		return true;
	}
	else if(!e.pushed())
	{
		return false;
	}
	else if(e.isDefaultLeftButton())
	{
		selectKeyRel(-1, 0);
		return true;
	}
	else if(e.isDefaultRightButton())
	{
		selectKeyRel(1, 0);
		return true;
	}
	else if(e.isDefaultUpButton())
	{
		selectKeyRel(0, -1);
		return true;
	}
	else if(e.isDefaultDownButton())
	{
		selectKeyRel(0, 1);
		return true;
	}
	return false;
}

void VControllerKeyboard::selectKey(uint x, uint y)
{
	if(x >= VKEY_COLS || y >= KEY_ROWS)
	{
		logErr("selected key:%dx%d out of range", x, y);
	}
	selected = {(int)x, (int)y, (int)x, (int)y};
	extendKeySelection();
}

void VControllerKeyboard::selectKeyRel(int x, int y)
{
	if(x > 0)
	{
		selected.x2 = IG::wrapMinMax(selected.x2 + x, 0, (int)VKEY_COLS);
		selected.x = selected.x2;
	}
	else if(x < 0)
	{
		selected.x = IG::wrapMinMax(selected.x + x, 0, (int)VKEY_COLS);
		selected.x2 = selected.x;
	}
	if(y != 0)
	{
		selected.y = selected.y2 = IG::wrapMinMax(selected.y2 + y, 0, (int)KEY_ROWS);
		selected.x2 = selected.x;
	}
	extendKeySelection();
	if(!currentKey())
	{
		logMsg("skipping blank key index");
		selectKeyRel(x, y);
	}
}

void VControllerKeyboard::unselectKey()
{
	selected = {-1, -1, -1, -1};
}

void VControllerKeyboard::extendKeySelection()
{
	auto key = currentKey();
	iterateTimes(selected.x, i)
	{
		if(table[selected.y][selected.x - 1] == key)
			selected.x--;
		else
			break;
	}
	iterateTimes((VKEY_COLS - 1) - selected.x2, i)
	{
		if(table[selected.y][selected.x2 + 1] == key)
			selected.x2++;
		else
			break;
	}
	logMsg("extended selection to:%d:%d", selected.x, selected.x2);
}

uint VControllerKeyboard::currentKey() const
{
	return table[selected.y][selected.x];
}

void VControllerKeyboard::setMode(Gfx::Renderer &r, int mode)
{
	mode_ = mode;
	updateImg(r);
	updateKeyboardMapping();
}

void VControllerKeyboard::applyMap(KbMap map)
{
	table = {};
	// 1st row
	auto *__restrict tablePtr = &table[0][0];
	auto *__restrict mapPtr = &map[0];
	iterateTimes(10, i)
	{
		tablePtr[0] = *mapPtr;
		tablePtr[1] = *mapPtr;
		tablePtr += 2;
		mapPtr++;
	}
	// 2nd row
	mapPtr = &map[10];
	if(mode_ == 0)
	{
		tablePtr = &table[1][1];
		iterateTimes(9, i)
		{
			tablePtr[0] = *mapPtr;
			tablePtr[1] = *mapPtr;
			tablePtr += 2;
			mapPtr++;
		}
	}
	else
	{
		tablePtr = &table[1][0];
		iterateTimes(10, i)
		{
			tablePtr[0] = *mapPtr;
			tablePtr[1] = *mapPtr;
			tablePtr += 2;
			mapPtr++;
		}
	}
	// 3rd row
	mapPtr = &map[20];
	table[2][0] = table[2][1] = table[2][2] = *mapPtr;
	mapPtr++;
	tablePtr = &table[2][3];
	iterateTimes(7, i)
	{
		tablePtr[0] = *mapPtr;
		tablePtr[1] = *mapPtr;
		tablePtr += 2;
		mapPtr++;
	}
	table[2][17] = table[2][18] = table[2][19] = *mapPtr;
	// 4th row
	table[3][0] = table[3][1] = table[3][2] = TOGGLE_KEYBOARD;
	table[3][3] = table[3][4] = table[3][5] = CHANGE_KEYBOARD_MODE;
	tablePtr = &table[3][6];
	mapPtr = &map[33];
	iterateTimes(8, i)
	{
		*tablePtr++ = *mapPtr;
	}
	mapPtr += 4;
	table[3][14] = table[3][15] = table[3][16] = *mapPtr;
	mapPtr += 2;
	table[3][17] = table[3][18] = table[3][19] = *mapPtr;

	/*iterateTimes(table.size(), i)
	{
		logMsg("row:%d", i);
		iterateTimes(table[0].size(), j)
		{
			logMsg("col:%d = %d", j, table[i][j]);
		}
	}*/
}

void VControllerKeyboard::updateKeyboardMapping()
{
	auto map = updateVControllerKeyboardMapping(mode());
	applyMap(map);
}

void VControllerGamepad::setBoundingAreaVisible(Gfx::Renderer &r, bool on)
{
	showBoundingArea = on;
	dp.setBoundingAreaVisible(r, on);
}

bool VControllerGamepad::boundingAreaVisible() const
{
	return showBoundingArea;
}

void VControllerGamepad::setImg(Gfx::Renderer &r, Gfx::PixmapTexture &pics)
{
	using namespace Gfx;
	pics.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
	Gfx::GTexC h = EmuSystem::inputFaceBtns == 2 ? 128. : 256.;
	dp.setImg(r, pics, h);
	iterateTimes(EmuSystem::inputCenterBtns, i)
	{
		centerBtnSpr[i].init({});
	}
	centerBtnSpr[0].setImg(&pics, {0., 65._gtexc/h, 32./64., 81._gtexc/h});
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnSpr[1].setImg(&pics, {33./64., 65._gtexc/h, 1., 81._gtexc/h});
	}

	iterateTimes(EmuSystem::inputFaceBtns, i)
	{
		circleBtnSpr[i].init({});
	}
	if(EmuSystem::inputFaceBtns == 2)
	{
		circleBtnSpr[0].setImg(&pics, {0., 82._gtexc/h, 32./64., 114._gtexc/h});
		circleBtnSpr[1].setImg(&pics, {33./64., 83._gtexc/h, 1., 114._gtexc/h});
	}
	else // for tall overlay image
	{
		circleBtnSpr[0].setImg(&pics, {0., 82._gtexc/h, 32./64., 114._gtexc/h});
		circleBtnSpr[1].setImg(&pics, {33./64., 83._gtexc/h, 1., 114._gtexc/h});
		circleBtnSpr[2].setImg(&pics, {0., 115._gtexc/h, 32./64., 147._gtexc/h});
		circleBtnSpr[3].setImg(&pics, {33./64., 116._gtexc/h, 1., 147._gtexc/h});
		if(EmuSystem::inputFaceBtns >= 6)
		{
			circleBtnSpr[4].setImg(&pics, {0., 148._gtexc/h, 32./64., 180._gtexc/h});
			circleBtnSpr[5].setImg(&pics, {33./64., 149._gtexc/h, 1., 180._gtexc/h});
		}
		if(EmuSystem::inputFaceBtns == 8)
		{
			circleBtnSpr[6].setImg(&pics, {0., 181._gtexc/h, 32./64., 213._gtexc/h});
			circleBtnSpr[7].setImg(&pics, {33./64., 182._gtexc/h, 1., 213._gtexc/h});
		}
	}
}

IG::WindowRect VControllerGamepad::centerBtnBounds() const
{
	return centerBtnsBound;
}

void VControllerGamepad::setCenterBtnPos(IG::Point2D<int> pos)
{
	centerBtnsBound.setPos(pos, C2DO);
	centerBtnsBound.fitIn(mainWin.viewport().bounds());
	int buttonXSpace = btnSpacePixels;//btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
	int extraXSize = buttonXSpace + btnSizePixels * btnExtraXSize;
	int spriteYPos = centerBtnsBound.yCenter() - centerBtnsBound.ySize()/6;
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnBound[1] = IG::makeWindowRectRel({(centerBtnsBound.x2 - btnSizePixels) - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
		centerBtnSpr[1].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x2 - btnSizePixels, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
	}
	else
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {centerBtnsBound.xSize(), btnSizePixels/2})));
	}
}

static uint lTriggerIdx()
{
	return EmuSystem::inputFaceBtns-2;
}

static uint rTriggerIdx()
{
	return EmuSystem::inputFaceBtns-1;
}

IG::WindowRect VControllerGamepad::lTriggerBounds() const
{
	return lTriggerBound;
}

void VControllerGamepad::setLTriggerPos(IG::Point2D<int> pos)
{
	uint idx = lTriggerIdx();
	lTriggerBound.setPos(pos, C2DO);
	lTriggerBound.fitIn(mainWin.viewport().bounds());
	auto lTriggerAreaGC = mainWin.projectionPlane.unProjectRect(lTriggerBound);
	faceBtnBound[idx] = lTriggerBound;
	circleBtnSpr[idx].setPos(lTriggerBound, mainWin.projectionPlane);
}

IG::WindowRect VControllerGamepad::rTriggerBounds() const
{
	return rTriggerBound;
}

void VControllerGamepad::setRTriggerPos(IG::Point2D<int> pos)
{
	uint idx = rTriggerIdx();
	rTriggerBound.setPos(pos, C2DO);
	rTriggerBound.fitIn(mainWin.viewport().bounds());
	auto rTriggerAreaGC = mainWin.projectionPlane.unProjectRect(rTriggerBound);
	faceBtnBound[idx] = rTriggerBound;
	circleBtnSpr[idx].setPos(rTriggerBound, mainWin.projectionPlane);
}

void VControllerGamepad::layoutBtnRows(uint a[], uint btns, uint rows, IG::Point2D<int> pos)
{
	int btnsPerRow = btns/rows;
	//logMsg("laying out buttons with size %d, space %d, row shift %d, stagger %d", btnSizePixels, btnSpacePixels, btnRowShiftPixels, btnStaggerPixels);
	faceBtnsBound.setPos(pos, C2DO);
	faceBtnsBound.fitIn(mainWin.viewport().bounds());
	auto btnArea = mainWin.projectionPlane.unProjectRect(faceBtnsBound);

	int row = 0, btnPos = 0;
	Gfx::GC yOffset = (btnStagger < 0) ? -btnStagger*(btnsPerRow-1) : 0,
		xOffset = -btnRowShift*(rows-1),
		staggerOffset = 0;
	iterateTimes(btns, i)
	{
		auto faceBtn = Gfx::makeGCRectRel(
			btnArea.pos(LB2DO) + Gfx::GP{xOffset, yOffset + staggerOffset}, {btnSize, btnSize});
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

		Gfx::GC btnExtraYSizeVal = (rows == 1) ? btnExtraYSize : btnExtraYSizeMultiRow;
		uint buttonXSpace = btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
		uint buttonYSpace = btnExtraYSizeVal ? btnSpacePixels * 2 : btnSpacePixels;
		int extraXSize = buttonXSpace + (Gfx::GC)btnSizePixels * btnExtraXSize;
		int extraYSize = buttonYSpace + (Gfx::GC)btnSizePixels * btnExtraYSizeVal;
		auto &bound = faceBtnBound[a[i]];
		bound = mainWin.projectionPlane.projectRect(faceBtn);
		bound = IG::makeWindowRectRel(bound.pos(LT2DO) - IG::WP{extraXSize/2, extraYSize/2},
			bound.size() + IG::WP{extraXSize, extraYSize});
	}
}

IG::WindowRect VControllerGamepad::faceBtnBounds() const
{
	return faceBtnsBound;
}

uint VControllerGamepad::rowsForButtons(uint activeButtons)
{
	if(!EmuSystem::inputHasTriggerBtns)
	{
		return (activeButtons > 3) ? 2 : 1;
	}
	else
	{
		return triggersInline_ ? 2 : (EmuSystem::inputFaceBtns < 6) ? 1 : 2;
	}
}

void VControllerGamepad::setFaceBtnPos(IG::Point2D<int> pos)
{
	using namespace IG;

	if(!EmuSystem::inputHasTriggerBtns)
	{
		uint btnMap[] 		{1, 0};
		uint btnMap2Rev[] {0, 1};
		uint btnMap3[] 		{0, 1, 2};
		uint btnMap4[] 		{0, 1, 2, 3};
		uint btnMap6Rev[] {0, 1, 2, 3, 4, 5};
		uint btnMap6[] 		{2, 1, 0, 3, 4, 5};
		uint rows = rowsForButtons(activeFaceBtns);
		if(activeFaceBtns == 6)
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap6Rev : btnMap6, IG::size(btnMap6), rows, pos);
		else if(activeFaceBtns == 4)
			layoutBtnRows(btnMap4, IG::size(btnMap4), rows, pos);
		else if(activeFaceBtns == 3)
			layoutBtnRows(btnMap3, IG::size(btnMap3), rows, pos);
		else
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap2Rev : btnMap, IG::size(btnMap), rows, pos);
	}
	else
	{
		if(triggersInline_)
		{
			uint btnMap8[] {0, 1, 2, 6, 3, 4, 5, 7};
			uint btnMap6[] {1, 0, 5, 3, 2, 4};
			uint btnMap4[] {1, 0, 2, 3};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, IG::size(btnMap8), 2, pos);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, IG::size(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, IG::size(btnMap4), 2, pos);
		}
		else
		{
			uint btnMap8[] {0, 1, 2, 3, 4, 5};
			uint btnMap6[] {1, 0, 3, 2};
			uint btnMap4[] {1, 0};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, IG::size(btnMap8), 2, pos);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, IG::size(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, IG::size(btnMap4), 1, pos);
		}
	}
}

void VControllerGamepad::setBaseBtnSize(Gfx::Renderer &r, uint sizeInPixels)
{
	btnSizePixels = sizeInPixels;
	btnSize = mainWin.projectionPlane.unprojectXSize(sizeInPixels);
	dp.setSize(r, IG::makeEvenRoundedUp(int(sizeInPixels*(double)2.5)));

	// face buttons
	uint btns = (EmuSystem::inputHasTriggerBtns && !triggersInline_) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns;
	uint rows = rowsForButtons(activeFaceBtns);
	int btnsPerRow = btns/rows;
	int xSizePixel = btnSizePixels*btnsPerRow + btnSpacePixels*(btnsPerRow-1) + std::abs(btnRowShiftPixels*((int)rows-1));
	int ySizePixel = btnSizePixels*rows + btnSpacePixels*(rows-1) + std::abs(btnStaggerPixels*((int)btnsPerRow-1));
	faceBtnsBound = IG::makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});

	// center buttons
	int cenBtnFullXSize = (EmuSystem::inputCenterBtns == 2) ? (btnSizePixels*2) + btnSpacePixels : btnSizePixels;
	int cenBtnFullYSize = IG::makeEvenRoundedUp(int(btnSizePixels*(double)1.25));
	centerBtnsBound = IG::makeWindowRectRel({0, 0}, {cenBtnFullXSize, cenBtnFullYSize});

	// triggers
	lTriggerBound = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	rTriggerBound = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
}

std::array<int, 2> VControllerGamepad::getCenterBtnInput(IG::WP pos) const
{
	std::array<int, 2> btnOut{-1, -1};
	uint count = 0;
	iterateTimes(EmuSystem::inputCenterBtns, i)
	{
		if(centerBtnBound[i].overlaps(pos))
		{
			//logMsg("overlaps %d", (int)i);
			btnOut[count] = i;
			count++;
			if(count == 2)
				return btnOut;
		}
	}
	return btnOut;
}

std::array<int, 2> VControllerGamepad::getBtnInput(IG::WP pos) const
{
	std::array<int, 2> btnOut{-1, -1};
	uint count = 0;
	bool doSeparateTriggers = EmuSystem::inputHasTriggerBtns && !triggersInline_;
	if(faceBtnsState)
	{
		iterateTimes(doSeparateTriggers ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
		{
			if(faceBtnBound[i].overlaps(pos))
			{
				//logMsg("overlaps %d", (int)i);
				btnOut[count] = i;
				count++;
				if(count == 2)
					return btnOut;
			}
		}
	}

	if(doSeparateTriggers)
	{
		if(lTriggerState_)
		{
			if(faceBtnBound[lTriggerIdx()].overlaps(pos))
			{
				btnOut[count] = lTriggerIdx();
				count++;
				if(count == 2)
					return btnOut;
			}
		}
		if(rTriggerState_)
		{
			if(faceBtnBound[rTriggerIdx()].overlaps(pos))
			{
				btnOut[count] = rTriggerIdx();
				count++;
				if(count == 2)
					return btnOut;
			}
		}
	}

	return btnOut;
}

void VControllerGamepad::draw(Gfx::RendererCommands &cmds, bool showHidden) const
{
	using namespace Gfx;
	if(dp.state() == 1 || (showHidden && dp.state()))
	{
		dp.draw(cmds);
	}

	if(faceBtnsState == 1 || (showHidden && faceBtnsState))
	{
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
		if(showBoundingArea)
		{
			cmds.setCommonProgram(CommonProgram::NO_TEX);
			iterateTimes((EmuSystem::inputHasTriggerBtns && !triggersInline_) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
			{
				GeomRect::draw(cmds, faceBtnBound[i], mainWin.projectionPlane);
			}
		}
		circleBtnSpr[0].setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
		//GeomRect::draw(faceBtnsBound);
		iterateTimes((EmuSystem::inputHasTriggerBtns && !triggersInline_) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
		{
			circleBtnSpr[i].draw(cmds);
		}
	}

	if(EmuSystem::inputHasTriggerBtns && !triggersInline_)
	{
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
		if(showBoundingArea)
		{
			if(lTriggerState_ == 1 || (showHidden && lTriggerState_))
			{
				cmds.setCommonProgram(CommonProgram::NO_TEX);
				GeomRect::draw(cmds, faceBtnBound[lTriggerIdx()], mainWin.projectionPlane);
			}
			if(rTriggerState_ == 1 || (showHidden && rTriggerState_))
			{
				cmds.setCommonProgram(CommonProgram::NO_TEX);
				GeomRect::draw(cmds, faceBtnBound[rTriggerIdx()], mainWin.projectionPlane);
			}
		}
		if(lTriggerState_ == 1 || (showHidden && lTriggerState_))
		{
			circleBtnSpr[lTriggerIdx()].setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
			circleBtnSpr[lTriggerIdx()].draw(cmds);
		}
		if(rTriggerState_ == 1 || (showHidden && rTriggerState_))
		{
			circleBtnSpr[rTriggerIdx()].setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
			circleBtnSpr[rTriggerIdx()].draw(cmds);
		}
	}

	if(centerBtnsState == 1 || (showHidden && centerBtnsState))
	{
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
		if(showBoundingArea)
		{
			cmds.setCommonProgram(CommonProgram::NO_TEX);
			iterateTimes(EmuSystem::inputCenterBtns, i)
			{
				GeomRect::draw(cmds, centerBtnBound[i], mainWin.projectionPlane);
			}
		}
		centerBtnSpr[0].setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
		iterateTimes(EmuSystem::inputCenterBtns, i)
		{
			centerBtnSpr[i].draw(cmds);
		}
	}
}

Gfx::GC VController::xMMSize(Gfx::GC mm) const
{
	return useScaledCoordinates ? mainWin.projectionPlane.xSMMSize(mm) : mainWin.projectionPlane.xMMSize(mm);
}

Gfx::GC VController::yMMSize(Gfx::GC mm) const
{
	return useScaledCoordinates ? mainWin.projectionPlane.ySMMSize(mm) : mainWin.projectionPlane.yMMSize(mm);
}

int VController::xMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const
{
	return useScaledCoordinates ? win.widthSMMInPixels(mm) : win.widthMMInPixels(mm);
}

int VController::yMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const
{
	return useScaledCoordinates ? win.heightSMMInPixels(mm) : win.heightMMInPixels(mm);
}

bool VController::hasTriggers() const
{
	return EmuSystem::inputHasTriggerBtns;
}

void VController::setImg(Gfx::PixmapTexture &pics)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setImg(renderer_, pics);
	#endif
}

void VController::setBoundingAreaVisible(bool on)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBoundingAreaVisible(renderer_, on);
	#endif
}

bool VController::boundingAreaVisible() const
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
	menuBound.fitIn(mainWin.viewport().bounds());
	menuBtnSpr.setPos(menuBound, mainWin.projectionPlane);
}

void VController::setFFBtnPos(IG::Point2D<int> pos)
{
	ffBound.setPos(pos, C2DO);
	ffBound.fitIn(mainWin.viewport().bounds());
	ffBtnSpr.setPos(ffBound, mainWin.projectionPlane);
}

void VController::setBaseBtnSize(uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(projP.unprojectYSize(gamepadBtnSizeInPixels), projP.unprojectYSize(gamepadBtnSizeInPixels * .75));
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBaseBtnSize(renderer_, gamepadBtnSizeInPixels);
	#endif
	int size = uiBtnSizeInPixels;
	if(menuBound.xSize() != size)
		logMsg("set UI button size: %d", size);
	menuBound = IG::makeWindowRectRel({0, 0}, {size, size});
	ffBound = IG::makeWindowRectRel({0, 0}, {size, size});
}

void VController::inputAction(uint action, uint vBtn)
{
	if(isInKeyboardMode())
	{
		EmuSystem::handleInputAction(action, kb.translateInput(vBtn));
	}
	else
	{
		assert(vBtn < IG::size(map));
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
	for(auto &e : ptrElem)
	{
		for(auto &vBtn : e)
		{
			if(!init && vBtn != -1) // release old key, if any
				inputAction(Input::RELEASED, vBtn);
			vBtn = -1;
		}
	}
}

void VController::init(float alpha, uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
{
	this->alpha = alpha;
	setBaseBtnSize(gamepadBtnSizeInPixels, uiBtnSizeInPixels, projP);
	if(EmuSystem::inputHasKeyboard)
	{
		kbMode = 0;
	}
	resetInput(1);
}

void VController::place()
{
	resetInput();
}

void VController::toggleKeyboard()
{
	logMsg("toggling keyboard");
	resetInput();
	kbMode ^= true;
}

std::array<int, 2> VController::findElementUnderPos(Input::Event e)
{
	if(isInKeyboardMode())
	{
		if(e.pushed())
		{
			kb.unselectKey();
		}
		int kbIdx = kb.getInput(e.pos());
		if(kbIdx == -1)
			return {-1, -1};
		if(kb.translateInput(kbIdx) == TOGGLE_KEYBOARD)
		{
			if(!e.pushed())
				return {-1, -1};
			logMsg("dismiss kb");
			toggleKeyboard();
		}
		else if(kb.translateInput(kbIdx) == CHANGE_KEYBOARD_MODE)
		{
			if(!e.pushed())
				return {-1, -1};
			logMsg("switch kb mode");
			kb.setMode(renderer_, kb.mode() ^ true);
			resetInput();
		}
		else
			 return {kbIdx, -1};
		return {-1, -1};
	}

	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(gp.centerButtonsState() != 0)
	{
		auto elem = gp.getCenterBtnInput(e.pos());
		if(elem[0] != -1)
		{
			return {C_ELEM + elem[0], elem[1] != -1 ? C_ELEM + elem[1] : -1};
		}
	}

	{
		auto elem = gp.getBtnInput(e.pos());
		if(elem[0] != -1)
		{
			return {F_ELEM + elem[0], elem[1] != -1 ? F_ELEM + elem[1] : -1};
		}
	}

	if(gp.dPad().state() != 0)
	{
		int elem = gp.dPad().getInput(e.pos());
		if(elem != -1)
		{
			return {D_ELEM + elem, -1};
		}
	}
	#endif
	return {-1, -1};
}

void VController::applyInput(Input::Event e)
{
	using namespace IG;
	assert(e.isPointer());
	auto &currElem = ptrElem[e.deviceID()];
	std::array<int, 2> elem{-1, -1};
	if(e.isPointerPushed(Input::Pointer::LBUTTON)) // make sure the cursor isn't hovering
		elem = findElementUnderPos(e);

	//logMsg("under %d %d", elem[0], elem[1]);

	// release old buttons
	for(auto vBtn : currElem)
	{
		if(vBtn != -1 && !IG::contains(elem, vBtn))
		{
			//logMsg("releasing %d", vBtn);
			inputAction(Input::RELEASED, vBtn);
		}
	}

	// push new buttons
	for(auto vBtn : elem)
	{
		if(vBtn != -1 && !IG::contains(currElem, vBtn))
		{
			//logMsg("pushing %d", vBtn);
			inputAction(Input::PUSHED, vBtn);
			if(optionVibrateOnPush)
			{
				Base::vibrate(32);
			}
		}
	}

	currElem = elem;
}

bool VController::keyInput(Input::Event e)
{
	if(!isInKeyboardMode())
		return false;
	return kb.keyInput(*this, renderer_, e);
}

void VController::draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden)
{
	draw(cmds, emuSystemControls, activeFF, showHidden, alpha);
}

void VController::draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden, float alpha)
{
	using namespace Gfx;
	if(unlikely(alpha == 0.))
		return;
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	cmds.setColor(1., 1., 1., alpha);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(isInKeyboardMode())
		kb.draw(cmds, mainWin.projectionPlane);
	else if(emuSystemControls)
		gp.draw(cmds, showHidden);
	#else
	if(isInKeyboardMode())
		kb.draw(cmds, mainWin.projectionPlane);
	#endif
	//GeomRect::draw(menuBound);
	//GeomRect::draw(ffBound);
	if(menuBtnState == 1 || (showHidden && menuBtnState))
	{
		cmds.setColor(1., 1., 1., alpha);
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
		menuBtnSpr.setCommonProgram(cmds, IMG_MODE_MODULATE);
		menuBtnSpr.draw(cmds);
	}
	if(ffBtnState == 1 || (showHidden && ffBtnState))
	{
		cmds.setColor(1., 1., 1., alpha);
		cmds.setCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
		ffBtnSpr.setCommonProgram(cmds, IMG_MODE_MODULATE);
		if(activeFF)
			cmds.setColor(1., 0., 0., alpha);
		ffBtnSpr.draw(cmds);
	}
}

int VController::numElements() const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	return (EmuSystem::inputHasTriggerBtns && !gp.triggersInline()) ? 7 : 5;
	#else
	return 5;
	#endif
}

IG::WindowRect VController::bounds(int elemIdx) const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dPad().bounds();
		case 1: return gp.centerBtnBounds();
		case 2: return gp.faceBtnBounds();
		case 3: return menuBound;
		case 4: return ffBound;
		case 5: return gp.lTriggerBounds();
		case 6: return gp.rTriggerBounds();
		default: bug_unreachable("elemIdx == %d", elemIdx); return {0,0,0,0};
	}
	#else
	switch(elemIdx)
	{
		case 0: return {0,0,0,0};
		case 1: return {0,0,0,0};
		case 2: return {0,0,0,0};
		case 3: return menuBound;
		case 4: return ffBound;
		default: bug_unreachable("elemIdx == %d", elemIdx); return {0,0,0,0};
	}
	#endif
}

void VController::setPos(int elemIdx, IG::Point2D<int> pos)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dPad().setPos(pos);
		bcase 1: gp.setCenterBtnPos(pos);
		bcase 2: gp.setFaceBtnPos(pos);
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bcase 5: gp.setLTriggerPos(pos);
		bcase 6: gp.setRTriggerPos(pos);
		bdefault: bug_unreachable("elemIdx == %d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bdefault: bug_unreachable("elemIdx == %d", elemIdx);
	}
	#endif
}

void VController::setState(int elemIdx, uint state)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dPad().setState(state);
		bcase 1: gp.setCenterButtonsState(state);
		bcase 2: gp.setFaceButtonsState(state);
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bcase 5: gp.setLTriggerState(state);
		bcase 6: gp.setRTriggerState(state);
		bdefault: bug_unreachable("elemIdx == %d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bdefault: bug_unreachable("elemIdx == %d", elemIdx);
	}
	#endif
}

uint VController::state(int elemIdx) const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dPad().state();
		case 1: return gp.centerButtonsState();
		case 2: return gp.faceButtonsState();
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		case 5: return gp.lTriggerState();
		case 6: return gp.rTriggerState();
		default: bug_unreachable("elemIdx == %d", elemIdx); return 0;
	}
	#else
	switch(elemIdx)
	{
		case 0: return 0;
		case 1: return 0;
		case 2: return 0;
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		default: bug_unreachable("elemIdx == %d", elemIdx); return 0;
	}
	#endif
}

bool VController::isInKeyboardMode() const
{
	return EmuSystem::inputHasKeyboard && vController.kbMode;
}

void VController::updateMapping(uint player)
{
	updateVControllerMapping(player, map);
}

void VController::updateKeyboardMapping()
{
	kb.updateKeyboardMapping();
}

[[gnu::weak]] SysVController::KbMap updateVControllerKeyboardMapping(uint mode) { return {}; }

void VController::setMenuImage(Gfx::PixmapTexture &img)
{
	menuBtnSpr.init({}, img);
}

void VController::setFastForwardImage(Gfx::PixmapTexture &img)
{
	ffBtnSpr.init({}, img);
}

void VController::setKeyboardImage(Gfx::PixmapTexture &img)
{
	kb.setImg(renderer_, &img);
}

bool VController::menuHitTest(IG::WP pos)
{
	auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_MENU_IDX].state != 0 && menuBound.overlaps(pos);
}

bool VController::fastForwardHitTest(IG::WP pos)
{
	auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_FF_IDX].state != 0 && ffBound.overlaps(pos);
}

void VController::setUsesScaledCoordinates(bool on)
{
	#ifdef CONFIG_BASE_ANDROID
	useScaledCoordinates = on;
	#endif
}

bool VController::usesScaledCoordinates()
{
	return useScaledCoordinates;
}

void VController::setAlpha(float val)
{
	alpha = val;
}

#ifdef CONFIG_VCONTROLS_GAMEPAD
VControllerGamepad &VController::gamePad()
{
	return gp;
}
#endif

Gfx::Renderer &VController::renderer()
{
	return renderer_;
}
