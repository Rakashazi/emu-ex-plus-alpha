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

#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/util/math.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"VControllerKeyboard"};

void VControllerKeyboard::updateImg()
{
	texture.bounds = mode_ == VControllerKbMode::LAYOUT_2 ? FRect{{0., .5}, {texXEnd, 1.}} : FRect{{}, {texXEnd, .5}};
	kbQuad.write(0, {.bounds = bound.as<int16_t>(), .textureSpan = texture});
}

void VControllerKeyboard::setImg(Gfx::RendererTask &task, Gfx::TextureSpan img)
{
	kbQuad = {task, {.size = 1}};
	texture = img;
	selectQuads = {task, {.size = 1}};
	selectQuads.write(0, {.bounds = {{}, {1, 1}}});
	texXEnd = img.bounds.x2;
	updateImg();
}

void VControllerKeyboard::place(int btnSize, int yOffset, WRect viewBounds)
{
	int xSize, ySize;
	setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, viewBounds.xSize()));
	int vArea = viewBounds.ySize() - yOffset * 2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	WRect bounds {{}, {xSize, ySize}};
	bounds.setPos({viewBounds.xCenter(), viewBounds.y2 - yOffset}, CB2DO);
	bound = bounds;
	keyXSize = std::max(bound.xSize() / VKEY_COLS, 1);
	keyYSize = std::max(bound.ySize() / KEY_ROWS, 1);
	//log.info("key size {}x{}", keyXSize, keyYSize);
	updateImg();
}

void VControllerKeyboard::draw(Gfx::RendererCommands &__restrict__ cmds) const
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	basicEffect.drawSprite(cmds, kbQuad, 0, texture);
	constexpr auto selectCol = Gfx::Color{.2, .71, .9, 1./3.}.multiplyAlpha();
	constexpr auto shiftCol = Gfx::Color{.2, .71, .9, .5}.multiplyAlpha();
	if(selected.x != -1)
	{
		cmds.setColor(selectCol);
		basicEffect.disableTexture(cmds);
		IG::WindowRect rect{};
		rect.x = bound.x + 1 + (selected.x * keyXSize);
		rect.x2 = bound.x + 1 + ((selected.x2 + 1) * keyXSize);
		rect.y = bound.y + 1 + (selected.y * keyYSize);
		rect.y2 = rect.y + keyYSize;
		basicEffect.setModelView(cmds, Mat4::makeTranslateScale(rect));
		cmds.drawQuad(selectQuads, 0);
	}
	if(shiftIsActive() && mode_ == VControllerKbMode::LAYOUT_1)
	{
		cmds.setColor(shiftCol);
		basicEffect.disableTexture(cmds);
		IG::WindowRect rect{};
		rect.x = bound.x + 1 + (shiftRect.x * keyXSize);
		rect.x2 = bound.x + 1 + ((shiftRect.x2 + 1) * keyXSize);
		rect.y = bound.y + 1 + (shiftRect.y * keyYSize);
		rect.y2 = rect.y + keyYSize;
		basicEffect.setModelView(cmds, Mat4::makeTranslateScale(rect));
		cmds.drawQuad(selectQuads, 0);
	}
}

int VControllerKeyboard::getInput(WPt c) const
{
	if(!bound.overlaps(c))
		return -1;
	int relX = c.x - bound.x, relY = c.y - bound.y;
	int row = std::min(relY/keyYSize, 3);
	int col = std::min(relX/keyXSize, 19);
	int idx = col + (row * VKEY_COLS);
	//log.debug("pointer {},{} key @ {},{}, idx {}", relX, relY, row, col, idx);
	return idx;
}

KeyInfo VControllerKeyboard::translateInput(int idx) const
{
	assumeExpr(idx < VKEY_COLS * KEY_ROWS);
	return table[0][idx];
}

bool VControllerKeyboard::keyInput(VController& v, const Input::KeyEvent& e)
{
	if(selected.x == -1)
	{
		if(e.pushed(Input::DefaultKey::CONFIRM) || e.pushed(Input::DefaultKey::DIRECTION))
		{
			selected = selectKey(0, 3);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if(e.isDefaultConfirmButton())
	{
		if(currentKey() == VController::TOGGLE_KEYBOARD)
		{
			if(!e.pushed() || e.repeated())
				return false;
			log.info("dismiss kb");
			unselectKey();
			v.toggleKeyboard();
		}
		else if(currentKey() == VController::CHANGE_KEYBOARD_MODE)
		{
			if(!e.pushed() || e.repeated())
				return false;
			log.info("switch kb mode");
			cycleMode(v.system());
			v.resetInput();
		}
		else if(e.pushed())
		{
			v.app().handleSystemKeyInput(currentKey(), Input::Action::PUSHED, 0);
		}
		else
		{
			v.app().handleSystemKeyInput(currentKey(), Input::Action::RELEASED, 0);
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

WRect VControllerKeyboard::selectKey(int x, int y)
{
	if(x >= VKEY_COLS || y >= KEY_ROWS)
	{
		log.error("selected key:{}x{} out of range", x, y);
		return {{-1, -1}, {-1, -1}};
	}
	return extendKeySelection({{x, y}, {x, y}});
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
	selected = extendKeySelection(selected);
	if(!currentKey(selected.x, selected.y))
	{
		log.info("skipping blank key index");
		selectKeyRel(x, y);
	}
}

void VControllerKeyboard::unselectKey()
{
	selected = {{-1, -1}, {-1, -1}};
}

IG::WindowRect VControllerKeyboard::extendKeySelection(IG::WindowRect selected)
{
	auto key = currentKey(selected.x, selected.y);
	for([[maybe_unused]] auto i : iotaCount(selected.x))
	{
		if(table[selected.y][selected.x - 1] == key)
			selected.x--;
		else
			break;
	}
	for([[maybe_unused]] auto i : iotaCount((VKEY_COLS - 1) - selected.x2))
	{
		if(table[selected.y][selected.x2 + 1] == key)
			selected.x2++;
		else
			break;
	}
	log.info("extended selection to:{}:{}", selected.x, selected.x2);
	return selected;
}

KeyInfo VControllerKeyboard::currentKey(int x, int y) const
{
	return table[y][x];
}

KeyInfo VControllerKeyboard::currentKey() const
{
	return currentKey(selected.x, selected.y);
}

void VControllerKeyboard::setMode(EmuSystem& sys, VControllerKbMode mode)
{
	mode_ = mode;
	updateImg();
	updateKeyboardMapping(sys);
}

void VControllerKeyboard::cycleMode(EmuSystem& sys)
{
	setMode(sys,
		mode() == VControllerKbMode::LAYOUT_1 ? VControllerKbMode::LAYOUT_2
		: VControllerKbMode::LAYOUT_1);
}

void VControllerKeyboard::applyMap(KbMap map)
{
	table = {};
	// 1st row
	auto *__restrict tablePtr = &table[0][0];
	auto *__restrict mapPtr = &map[0];
	for([[maybe_unused]] auto i : iotaCount(10))
	{
		tablePtr[0] = *mapPtr;
		tablePtr[1] = *mapPtr;
		tablePtr += 2;
		mapPtr++;
	}
	// 2nd row
	mapPtr = &map[10];
	if(mode_ == VControllerKbMode::LAYOUT_1)
	{
		tablePtr = &table[1][1];
		for([[maybe_unused]] auto i : iotaCount(9))
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
		for([[maybe_unused]] auto i : iotaCount(10))
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
	for([[maybe_unused]] auto i : iotaCount(7))
	{
		tablePtr[0] = *mapPtr;
		tablePtr[1] = *mapPtr;
		tablePtr += 2;
		mapPtr++;
	}
	table[2][17] = table[2][18] = table[2][19] = *mapPtr;
	// 4th row
	table[3][0] = table[3][1] = table[3][2] = VController::TOGGLE_KEYBOARD;
	table[3][3] = table[3][4] = table[3][5] = VController::CHANGE_KEYBOARD_MODE;
	tablePtr = &table[3][6];
	mapPtr = &map[33];
	for([[maybe_unused]] auto i : iotaCount(8))
	{
		*tablePtr++ = *mapPtr;
	}
	mapPtr += 4;
	table[3][14] = table[3][15] = table[3][16] = *mapPtr;
	mapPtr += 2;
	table[3][17] = table[3][18] = table[3][19] = *mapPtr;

	/*iterateTimes(table.size(), i)
	{
		log.debug("row:{}", i);
		iterateTimes(table[0].size(), j)
		{
			log.info("col:{} = {}", j, table[i][j]);
		}
	}*/
}

void VControllerKeyboard::updateKeyboardMapping(EmuSystem &sys)
{
	auto map = sys.vControllerKeyboardMap(mode());
	applyMap(map);
}

void VControllerKeyboard::setShiftActive(bool on)
{
	if(on)
	{
		shiftRect = selectKey(0, 2);
	}
	else
	{
		shiftRect = {{-1, -1}, {-1, -1}};
	}
}

bool VControllerKeyboard::toggleShiftActive()
{
	setShiftActive(shiftIsActive() ^ 1);
	return shiftIsActive();
}

bool VControllerKeyboard::shiftIsActive() const
{
	return shiftRect.x != -1;
}

}
