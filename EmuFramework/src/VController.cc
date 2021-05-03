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
#include "privateInput.hh"
#include "WindowData.hh"
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/math/space.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/base/Window.hh>

static constexpr unsigned TOGGLE_KEYBOARD = 65536;
static constexpr unsigned CHANGE_KEYBOARD_MODE = 65537;

void VControllerDPad::setImg(Gfx::Renderer &r, Gfx::Texture &dpadR, Gfx::GTexC texHeight)
{
	using namespace Gfx;
	spr = {{{-.5, -.5}, {.5, .5}}, {&dpadR, {{}, {1., 64._gtexc/texHeight}}}};
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r, const WindowData &winData)
{
	if(visualizeBounds && padArea.xSize())
	{
		IG::MemPixmap mapMemPix{{padArea.size(), IG::PIXEL_FMT_RGB565}};
		auto mapPix = mapMemPix.view();
		iterateTimes(mapPix.h(), y)
			iterateTimes(mapPix.w(), x)
			{
				int input = getInput({padArea.xPos(LT2DO) + (int)x, padArea.yPos(LT2DO) + (int)y});
				//logMsg("got input %d", input);
				*((uint16_t*)mapPix.pixel({(int)x, (int)y})) = input == -1 ? IG::PIXEL_DESC_RGB565.build(1., 0., 0., 1.)
										: IG::isOdd(input) ? IG::PIXEL_DESC_RGB565.build(1., 1., 1., 1.)
										: IG::PIXEL_DESC_RGB565.build(0., 1., 0., 1.);
			}
		mapImg = r.makePixmapTexture({mapPix, &r.make(View::imageCommonTextureSampler)});
		mapImg.write(0, mapPix, {});
		mapSpr = {{}, mapImg};
		mapSpr.setPos(padArea, winData.projection.plane());
	}
}

void VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone, const WindowData &winData)
{
	if(deadzone != newDeadzone)
	{
		deadzone = newDeadzone;
		updateBoundingAreaGfx(r, winData);
	}
}

void VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity, const WindowData &winData)
{
	if(diagonalSensitivity != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity = newDiagonalSensitivity;
		updateBoundingAreaGfx(r, winData);
	}
}

IG::WindowRect VControllerDPad::bounds() const
{
	return padBaseArea;
}

void VControllerDPad::setSize(Gfx::Renderer &r, unsigned sizeInPixels, const WindowData &winData)
{
	//logMsg("set dpad pixel size: %d", sizeInPixels);
	btnSizePixels = sizeInPixels;
	auto rect = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {{}, {int(padBaseArea.xSize()*1.5), int(padBaseArea.xSize()*1.5)}};
	if(visualizeBounds)
	{
		if(changedSize)
			updateBoundingAreaGfx(r, winData);
	}
}

void VControllerDPad::setPos(IG::Point2D<int> pos, const WindowData &winData)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(winData.viewport().bounds());
	padBase = winData.projection.plane().unProjectRect(padBaseArea);
	spr.setPos(padBase);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea, winData.projection.plane());
	}
}

void VControllerDPad::setBoundingAreaVisible(Gfx::Renderer &r, bool on, const WindowData &winData)
{
	if(visualizeBounds == on)
		return;
	visualizeBounds = on;
	if(!on)
	{
		if(mapSpr.image())
		{
			logMsg("deallocating bounding box display resources");
			mapSpr = {};
			mapImg = {};
		}
	}
	else
	{
		updateBoundingAreaGfx(r, winData);
	}
}

void VControllerDPad::draw(Gfx::RendererCommands &cmds) const
{
	cmds.set(View::imageCommonTextureSampler);
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
		spr.setUVBounds({{0., .5}, {texXEnd, 1.}});
	else
		spr.setUVBounds({{}, {texXEnd, .5}});
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerKeyboard::setImg(Gfx::Renderer &r, Gfx::TextureSpan img)
{
	spr = {{{-.5, -.5}, {.5, .5}}, img};
	texXEnd = img.uvBounds().x2;
	updateImg(r);
}

void VControllerKeyboard::place(Gfx::GC btnSize, Gfx::GC yOffset, const WindowData &winData)
{
	Gfx::GC xSize, ySize;
	IG::setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, winData.projection.plane().width()));
	Gfx::GC vArea = winData.projection.plane().height() - yOffset*2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	Gfx::GCRect boundGC {{}, {xSize, ySize}};
	boundGC.setPos({0., winData.projection.plane().bounds().y + yOffset}, CB2DO);
	spr.setPos(boundGC);
	bound = winData.projection.plane().projectRect(boundGC);
	keyXSize = std::max(bound.xSize() / VKEY_COLS, 1u);
	keyYSize = std::max(bound.ySize() / KEY_ROWS, 1u);
	logMsg("key size %dx%d", keyXSize, keyYSize);
}

void VControllerKeyboard::draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP) const
{
	if(spr.image()->levels() > 1)
		cmds.set(View::imageCommonTextureSampler);
	else
		cmds.set(Gfx::CommonTextureSampler::NO_MIP_CLAMP);
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
	unsigned row = std::min(relY/keyYSize, 3u);
	unsigned col = std::min(relX/keyXSize, 19u);
	unsigned idx = col + (row * VKEY_COLS);
	logMsg("pointer %d,%d key @ %d,%d, idx %d", relX, relY, row, col, idx);
	return idx;
}

int VControllerKeyboard::translateInput(unsigned idx) const
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
			EmuSystem::handleInputAction(&v.app(), Input::Action::PUSHED, currentKey());
		}
		else
		{
			EmuSystem::handleInputAction(&v.app(), Input::Action::RELEASED, currentKey());
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

void VControllerKeyboard::selectKey(unsigned x, unsigned y)
{
	if(x >= VKEY_COLS || y >= KEY_ROWS)
	{
		logErr("selected key:%dx%d out of range", x, y);
	}
	selected = {{(int)x, (int)y}, {(int)x, (int)y}};
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
	selected = {{-1, -1}, {-1, -1}};
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

unsigned VControllerKeyboard::currentKey() const
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

void VControllerGamepad::setBoundingAreaVisible(Gfx::Renderer &r, bool on, const WindowData &winData)
{
	showBoundingArea = on;
	dp.setBoundingAreaVisible(r, on, winData);
}

bool VControllerGamepad::boundingAreaVisible() const
{
	return showBoundingArea;
}

void VControllerGamepad::setImg(Gfx::Renderer &r, Gfx::Texture &pics)
{
	using namespace Gfx;
	pics.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
	Gfx::GTexC h = EmuSystem::inputFaceBtns == 2 || EmuSystem::inputHasShortBtnTexture ? 128. : 256.;
	dp.setImg(r, pics, h);
	iterateTimes(EmuSystem::inputCenterBtns, i)
	{
		centerBtnSpr[i] = {};
	}
	centerBtnSpr[0].setImg({&pics, {{0., 65._gtexc/h}, {32./64., 81._gtexc/h}}});
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnSpr[1].setImg({&pics, {{33./64., 65._gtexc/h}, {1., 81._gtexc/h}}});
	}

	iterateTimes(EmuSystem::inputFaceBtns, i)
	{
		circleBtnSpr[i] = {};
	}
	if(EmuSystem::inputFaceBtns == 2)
	{
		circleBtnSpr[0].setImg({&pics, {{0., 82._gtexc/h}, {32./64., 114._gtexc/h}}});
		circleBtnSpr[1].setImg({&pics, {{33./64., 83._gtexc/h}, {1., 114._gtexc/h}}});
	}
	else if(EmuSystem::inputFaceBtns == 4 && EmuSystem::inputHasShortBtnTexture)
	{
		circleBtnSpr[0].setImg({&pics, {{0., 82._gtexc/h}, {32./64., 114._gtexc/h}}});
		circleBtnSpr[1].setImg({&pics, {{33./64., 83._gtexc/h}, {1., 114._gtexc/h}}});
		circleBtnSpr[2].setImg({&pics, {{0., 82._gtexc/h}, {32./64., 114._gtexc/h}}});
		circleBtnSpr[3].setImg({&pics, {{33./64., 83._gtexc/h}, {1., 114._gtexc/h}}});
	}
	else // for tall overlay image
	{
		circleBtnSpr[0].setImg({&pics, {{0., 82._gtexc/h}, {32./64., 114._gtexc/h}}});
		circleBtnSpr[1].setImg({&pics, {{33./64., 83._gtexc/h}, {1., 114._gtexc/h}}});
		circleBtnSpr[2].setImg({&pics, {{0., 115._gtexc/h}, {32./64., 147._gtexc/h}}});
		circleBtnSpr[3].setImg({&pics, {{33./64., 116._gtexc/h}, {1., 147._gtexc/h}}});
		if(EmuSystem::inputFaceBtns >= 6)
		{
			circleBtnSpr[4].setImg({&pics, {{0., 148._gtexc/h}, {32./64., 180._gtexc/h}}});
			circleBtnSpr[5].setImg({&pics, {{33./64., 149._gtexc/h}, {1., 180._gtexc/h}}});
		}
		if(EmuSystem::inputFaceBtns == 8)
		{
			circleBtnSpr[6].setImg({&pics, {{0., 181._gtexc/h}, {32./64., 213._gtexc/h}}});
			circleBtnSpr[7].setImg({&pics, {{33./64., 182._gtexc/h}, {1., 213._gtexc/h}}});
		}
	}
}

IG::WindowRect VControllerGamepad::centerBtnBounds() const
{
	return centerBtnsBound;
}

void VControllerGamepad::setCenterBtnPos(IG::Point2D<int> pos, const WindowData &winData)
{
	centerBtnsBound.setPos(pos, C2DO);
	centerBtnsBound.fitIn(winData.viewport().bounds());
	int buttonXSpace = btnSpacePixels;//btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
	int extraXSize = buttonXSpace + btnSizePixels * btnExtraXSize;
	int spriteYPos = centerBtnsBound.yCenter() - centerBtnsBound.ySize()/6;
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnBound[1] = IG::makeWindowRectRel({(centerBtnsBound.x2 - btnSizePixels) - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(winData.projection.plane().unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
		centerBtnSpr[1].setPos(winData.projection.plane().unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x2 - btnSizePixels, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
	}
	else
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(winData.projection.plane().unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {centerBtnsBound.xSize(), btnSizePixels/2})));
	}
}

static unsigned lTriggerIdx()
{
	return EmuSystem::inputFaceBtns-2;
}

static unsigned rTriggerIdx()
{
	return EmuSystem::inputFaceBtns-1;
}

IG::WindowRect VControllerGamepad::lTriggerBounds() const
{
	return lTriggerBound;
}

void VControllerGamepad::setLTriggerPos(IG::Point2D<int> pos, const WindowData &winData)
{
	unsigned idx = lTriggerIdx();
	lTriggerBound.setPos(pos, C2DO);
	lTriggerBound.fitIn(winData.viewport().bounds());
	auto lTriggerAreaGC = winData.projection.plane().unProjectRect(lTriggerBound);
	faceBtnBound[idx] = lTriggerBound;
	circleBtnSpr[idx].setPos(lTriggerBound, winData.projection.plane());
}

IG::WindowRect VControllerGamepad::rTriggerBounds() const
{
	return rTriggerBound;
}

void VControllerGamepad::setRTriggerPos(IG::Point2D<int> pos, const WindowData &winData)
{
	unsigned idx = rTriggerIdx();
	rTriggerBound.setPos(pos, C2DO);
	rTriggerBound.fitIn(winData.viewport().bounds());
	auto rTriggerAreaGC = winData.projection.plane().unProjectRect(rTriggerBound);
	faceBtnBound[idx] = rTriggerBound;
	circleBtnSpr[idx].setPos(rTriggerBound, winData.projection.plane());
}

void VControllerGamepad::layoutBtnRows(unsigned a[], unsigned btns, unsigned rows, IG::Point2D<int> pos, const WindowData &winData)
{
	int btnsPerRow = btns/rows;
	//logMsg("laying out buttons with size %d, space %d, row shift %d, stagger %d", btnSizePixels, btnSpacePixels, btnRowShiftPixels, btnStaggerPixels);
	faceBtnsBound.setPos(pos, C2DO);
	faceBtnsBound.fitIn(winData.viewport().bounds());
	auto btnArea = winData.projection.plane().unProjectRect(faceBtnsBound);

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
		unsigned buttonXSpace = btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
		unsigned buttonYSpace = btnExtraYSizeVal ? btnSpacePixels * 2 : btnSpacePixels;
		int extraXSize = buttonXSpace + (Gfx::GC)btnSizePixels * btnExtraXSize;
		int extraYSize = buttonYSpace + (Gfx::GC)btnSizePixels * btnExtraYSizeVal;
		auto &bound = faceBtnBound[a[i]];
		bound = winData.projection.plane().projectRect(faceBtn);
		bound = IG::makeWindowRectRel(bound.pos(LT2DO) - IG::WP{extraXSize/2, extraYSize/2},
			bound.size() + IG::WP{extraXSize, extraYSize});
	}
}

IG::WindowRect VControllerGamepad::faceBtnBounds() const
{
	return faceBtnsBound;
}

unsigned VControllerGamepad::rowsForButtons(unsigned activeButtons)
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

void VControllerGamepad::setFaceBtnPos(IG::Point2D<int> pos, const WindowData &winData)
{
	using namespace IG;

	if(!EmuSystem::inputHasTriggerBtns)
	{
		unsigned btnMap[] 		{1, 0};
		unsigned btnMap2Rev[] {0, 1};
		unsigned btnMap3[] 		{0, 1, 2};
		unsigned btnMap4[] 		{0, 1, 2, 3};
		unsigned btnMap6Rev[] {0, 1, 2, 3, 4, 5};
		unsigned btnMap6[] 		{2, 1, 0, 3, 4, 5};
		unsigned rows = rowsForButtons(activeFaceBtns);
		if(activeFaceBtns == 6)
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap6Rev : btnMap6, std::size(btnMap6), rows, pos, winData);
		else if(activeFaceBtns == 4)
			layoutBtnRows(btnMap4, std::size(btnMap4), rows, pos, winData);
		else if(activeFaceBtns == 3)
			layoutBtnRows(btnMap3, std::size(btnMap3), rows, pos, winData);
		else
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap2Rev : btnMap, std::size(btnMap), rows, pos, winData);
	}
	else
	{
		if(triggersInline_)
		{
			unsigned btnMap8[] {0, 1, 2, 6, 3, 4, 5, 7};
			unsigned btnMap6[] {1, 0, 5, 3, 2, 4};
			unsigned btnMap4[] {1, 0, 2, 3};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, std::size(btnMap8), 2, pos, winData);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, std::size(btnMap6), 2, pos, winData);
			else
				layoutBtnRows(btnMap4, std::size(btnMap4), 2, pos, winData);
		}
		else
		{
			unsigned btnMap8[] {0, 1, 2, 3, 4, 5};
			unsigned btnMap6[] {1, 0, 3, 2};
			unsigned btnMap4[] {1, 0};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, std::size(btnMap8), 2, pos, winData);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, std::size(btnMap6), 2, pos, winData);
			else
				layoutBtnRows(btnMap4, std::size(btnMap4), 1, pos, winData);
		}
	}
}

void VControllerGamepad::setBaseBtnSize(Gfx::Renderer &r, unsigned sizeInPixels, const WindowData &winData)
{
	btnSizePixels = sizeInPixels;
	btnSize = winData.projection.plane().unprojectXSize(sizeInPixels);
	dp.setSize(r, IG::makeEvenRoundedUp(int(sizeInPixels*(double)2.5)), winData);

	// face buttons
	unsigned btns = (EmuSystem::inputHasTriggerBtns && !triggersInline_) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns;
	unsigned rows = rowsForButtons(activeFaceBtns);
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
	unsigned count = 0;
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
	unsigned count = 0;
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

void VControllerGamepad::draw(Gfx::RendererCommands &cmds, bool showHidden, const WindowData &winData) const
{
	using namespace Gfx;
	if(dp.state() == 1 || (showHidden && dp.state()))
	{
		dp.draw(cmds);
	}

	if(faceBtnsState == 1 || (showHidden && faceBtnsState))
	{
		cmds.set(View::imageCommonTextureSampler);
		if(showBoundingArea)
		{
			cmds.setCommonProgram(CommonProgram::NO_TEX);
			iterateTimes((EmuSystem::inputHasTriggerBtns && !triggersInline_) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
			{
				GeomRect::draw(cmds, faceBtnBound[i], winData.projection.plane());
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
		cmds.set(View::imageCommonTextureSampler);
		if(showBoundingArea)
		{
			if(lTriggerState_ == 1 || (showHidden && lTriggerState_))
			{
				cmds.setCommonProgram(CommonProgram::NO_TEX);
				GeomRect::draw(cmds, faceBtnBound[lTriggerIdx()], winData.projection.plane());
			}
			if(rTriggerState_ == 1 || (showHidden && rTriggerState_))
			{
				cmds.setCommonProgram(CommonProgram::NO_TEX);
				GeomRect::draw(cmds, faceBtnBound[rTriggerIdx()], winData.projection.plane());
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
		cmds.set(View::imageCommonTextureSampler);
		if(showBoundingArea)
		{
			cmds.setCommonProgram(CommonProgram::NO_TEX);
			iterateTimes(EmuSystem::inputCenterBtns, i)
			{
				GeomRect::draw(cmds, centerBtnBound[i], winData.projection.plane());
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
	return useScaledCoordinates ? windowData().projection.plane().xSMMSize(mm) : windowData().projection.plane().xMMSize(mm);
}

Gfx::GC VController::yMMSize(Gfx::GC mm) const
{
	return useScaledCoordinates ? windowData().projection.plane().ySMMSize(mm) : windowData().projection.plane().yMMSize(mm);
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

void VController::setImg(Gfx::Texture &pics)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setImg(renderer(), pics);
	#endif
}

void VController::setBoundingAreaVisible(bool on)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBoundingAreaVisible(renderer(), on, windowData());
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
	menuBound.fitIn(windowData().viewport().bounds());
	menuBtnSpr.setPos(menuBound, windowData().projection.plane());
}

void VController::setFFBtnPos(IG::Point2D<int> pos)
{
	ffBound.setPos(pos, C2DO);
	ffBound.fitIn(windowData().viewport().bounds());
	ffBtnSpr.setPos(ffBound, windowData().projection.plane());
}

void VController::setBaseBtnSize(unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(projP.unprojectYSize(gamepadBtnSizeInPixels), projP.unprojectYSize(gamepadBtnSizeInPixels * .75), windowData());
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBaseBtnSize(renderer(), gamepadBtnSizeInPixels, windowData());
	#endif
	int size = uiBtnSizeInPixels;
	if(menuBound.xSize() != size)
		logMsg("set UI button size: %d", size);
	menuBound = IG::makeWindowRectRel({0, 0}, {size, size});
	ffBound = IG::makeWindowRectRel({0, 0}, {size, size});
}

void VController::inputAction(Input::Action action, unsigned vBtn)
{
	if(isInKeyboardMode())
	{
		EmuSystem::handleInputAction(&app(), action, kb.translateInput(vBtn));
	}
	else
	{
		assert(vBtn < std::size(map));
		auto turbo = map[vBtn] & TURBO_BIT;
		auto keyCode = map[vBtn] & ACTION_MASK;
		if(turbo)
		{
			if(action == Input::Action::PUSHED)
			{
				app().addTurboInputEvent(keyCode);
			}
			else
			{
				app().removeTurboInputEvent(keyCode);
			}
		}
		EmuSystem::handleInputAction(&app(), action, keyCode);
	}
}

void VController::resetInput(bool init)
{
	for(auto &e : ptrElem)
	{
		for(auto &vBtn : e)
		{
			if(!init && vBtn != -1) // release old key, if any
				inputAction(Input::Action::RELEASED, vBtn);
			vBtn = -1;
		}
	}
}

void VController::init(float alpha, unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
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
			kb.setMode(renderer(), kb.mode() ^ true);
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
			inputAction(Input::Action::RELEASED, vBtn);
		}
	}

	// push new buttons
	for(auto vBtn : elem)
	{
		if(vBtn != -1 && !IG::contains(currElem, vBtn))
		{
			//logMsg("pushing %d", vBtn);
			inputAction(Input::Action::PUSHED, vBtn);
			if(optionVibrateOnPush)
			{
				appContext().vibrate(IG::Milliseconds{32});
			}
		}
	}

	currElem = elem;
}

bool VController::keyInput(Input::Event e)
{
	if(!isInKeyboardMode())
		return false;
	return kb.keyInput(*this, renderer(), e);
}

void VController::draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden)
{
	draw(cmds, emuSystemControls, activeFF, showHidden, alpha);
}

void VController::draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden, float alpha)
{
	using namespace Gfx;
	if(alpha == 0.f) [[unlikely]]
		return;
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	cmds.setColor(1., 1., 1., alpha);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(isInKeyboardMode())
		kb.draw(cmds, windowData().projection.plane());
	else if(emuSystemControls)
		gp.draw(cmds, showHidden, windowData());
	#else
	if(isInKeyboardMode())
		kb.draw(cmds, windowData().projection.plane());
	#endif
	//GeomRect::draw(menuBound);
	//GeomRect::draw(ffBound);
	if(menuBtnState == 1 || (showHidden && menuBtnState))
	{
		cmds.setColor(1., 1., 1., alpha);
		cmds.set(View::imageCommonTextureSampler);
		menuBtnSpr.setCommonProgram(cmds, IMG_MODE_MODULATE);
		menuBtnSpr.draw(cmds);
	}
	if(ffBtnState == 1 || (showHidden && ffBtnState))
	{
		cmds.setColor(1., 1., 1., alpha);
		cmds.set(View::imageCommonTextureSampler);
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
		default: bug_unreachable("elemIdx == %d", elemIdx); return {};
	}
	#else
	switch(elemIdx)
	{
		case 0: return {};
		case 1: return {};
		case 2: return {};
		case 3: return menuBound;
		case 4: return ffBound;
		default: bug_unreachable("elemIdx == %d", elemIdx); return {};
	}
	#endif
}

void VController::setPos(int elemIdx, IG::Point2D<int> pos)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dPad().setPos(pos, windowData());
		bcase 1: gp.setCenterBtnPos(pos, windowData());
		bcase 2: gp.setFaceBtnPos(pos, windowData());
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bcase 5: gp.setLTriggerPos(pos, windowData());
		bcase 6: gp.setRTriggerPos(pos, windowData());
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

void VController::setState(int elemIdx, unsigned state)
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

unsigned VController::state(int elemIdx) const
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
	return EmuSystem::inputHasKeyboard && kbMode;
}

void VController::setInputPlayer(uint8_t player)
{
	inputPlayer_ = player;
	updateMapping();
}

uint8_t VController::inputPlayer() const
{
	return inputPlayer_;
}

void VController::updateMapping()
{
	updateVControllerMapping(inputPlayer(), map);
}

void VController::updateKeyboardMapping()
{
	kb.updateKeyboardMapping();
}

[[gnu::weak]] SysVController::KbMap updateVControllerKeyboardMapping(unsigned mode) { return {}; }

void VController::setMenuImage(Gfx::TextureSpan img)
{
	menuBtnSpr = {{}, img};
}

void VController::setFastForwardImage(Gfx::TextureSpan img)
{
	ffBtnSpr = {{}, img};
}

void VController::setKeyboardImage(Gfx::TextureSpan img)
{
	kb.setImg(renderer(), img);
}

bool VController::menuHitTest(IG::WP pos)
{
	auto &layoutPos = layoutPosition()[windowData().viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_MENU_IDX].state != 0 && menuBound.overlaps(pos);
}

bool VController::fastForwardHitTest(IG::WP pos)
{
	auto &layoutPos = layoutPosition()[windowData().viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_FF_IDX].state != 0 && ffBound.overlaps(pos);
}

void VController::resetUsesScaledCoordinates()
{
	useScaledCoordinates = useScaledCoordinatesDefault;
}

void VController::setUsesScaledCoordinates(std::optional<bool> opt)
{
	if(!opt)
		return;
	useScaledCoordinates = *opt;
}

bool VController::usesScaledCoordinates() const
{
	return useScaledCoordinates;
}

std::optional<bool> VController::usesScaledCoordinatesOption() const
{
	if(!useScaledCoordinatesIsMutable || usesScaledCoordinates() == useScaledCoordinatesDefault)
		return {};
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

void VController::setRenderer(Gfx::Renderer &renderer)
{
	renderer_ = &renderer;
}

Gfx::Renderer &VController::renderer()
{
	return *renderer_;
}

void VController::setWindow(const Base::Window &win_)
{
	win = &win_;
	winData = &::windowData(win_);
}

Base::ApplicationContext VController::appContext() const
{
	assert(hasWindow());
	return window().appContext();
}

const Gfx::GlyphTextureSet &VController::face() const
{
	return *facePtr;
}

void VController::setFace(const Gfx::GlyphTextureSet &face)
{
	facePtr = &face;
}
