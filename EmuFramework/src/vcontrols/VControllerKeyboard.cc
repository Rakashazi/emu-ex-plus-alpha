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

#define LOGTAG "VControllerKeyboard"
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/util/math/space.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

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

void VControllerKeyboard::place(Gfx::GC btnSize, Gfx::GC yOffset, Gfx::ProjectionPlane projP)
{
	Gfx::GC xSize, ySize;
	IG::setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, projP.width()));
	Gfx::GC vArea = projP.height() - yOffset*2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	Gfx::GCRect boundGC {{}, {xSize, ySize}};
	boundGC.setPos({0., projP.bounds().y + yOffset}, CB2DO);
	spr.setPos(boundGC);
	bound = projP.projectRect(boundGC);
	keyXSize = std::max(bound.xSize() / VKEY_COLS, 1u);
	keyYSize = std::max(bound.ySize() / KEY_ROWS, 1u);
	logMsg("key size %dx%d", keyXSize, keyYSize);
}

void VControllerKeyboard::draw(Gfx::RendererCommands &cmds, Gfx::ProjectionPlane projP) const
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
	if(shiftIsActive() && mode_ == 0)
	{
		cmds.setColor(.2, .71, .9, 1./2.);
		cmds.setCommonProgram(Gfx::CommonProgram::NO_TEX, projP.makeTranslate());
		IG::WindowRect rect{};
		rect.x = bound.x + (shiftRect.x * keyXSize);
		rect.x2 = bound.x + ((shiftRect.x2 + 1) * keyXSize);
		rect.y = bound.y + (shiftRect.y * keyYSize);
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
	//logMsg("pointer %d,%d key @ %d,%d, idx %d", relX, relY, row, col, idx);
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
			logMsg("dismiss kb");
			unselectKey();
			v.toggleKeyboard();
		}
		else if(currentKey() == VController::CHANGE_KEYBOARD_MODE)
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

IG::WindowRect VControllerKeyboard::selectKey(unsigned x, unsigned y)
{
	if(x >= VKEY_COLS || y >= KEY_ROWS)
	{
		logErr("selected key:%dx%d out of range", x, y);
		return {{-1, -1}, {-1, -1}};
	}
	return extendKeySelection({{(int)x, (int)y}, {(int)x, (int)y}});
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
		logMsg("skipping blank key index");
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
	return selected;
}

unsigned VControllerKeyboard::currentKey(int x, int y) const
{
	return table[y][x];
}

unsigned VControllerKeyboard::currentKey() const
{
	return currentKey(selected.x, selected.y);
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
	table[3][0] = table[3][1] = table[3][2] = VController::TOGGLE_KEYBOARD;
	table[3][3] = table[3][4] = table[3][5] = VController::CHANGE_KEYBOARD_MODE;
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
