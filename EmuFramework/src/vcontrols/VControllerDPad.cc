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

#define LOGTAG "VControllerGamepad"
#include <emuframework/VController.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/util/math/int.hh>
#include <imagine/base/Window.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/gui/View.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

void VControllerDPad::setImage(Gfx::TextureSpan img)
{
	spr = {{{-.5, -.5}, {.5, .5}}, img};
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r)
{
	if(visualizeBounds && padArea.xSize())
	{
		MemPixmap mapMemPix{{padArea.size(), PIXEL_FMT_RGB565}};
		auto mapPix = mapMemPix.view();
		for(auto y : iotaCount(mapPix.h()))
			for(auto x : iotaCount(mapPix.w()))
			{
				auto input = getInput({padArea.xPos(LT2DO) + (int)x, padArea.yPos(LT2DO) + (int)y});
				//logMsg("got input %d", input);
				*((uint16_t*)mapPix.pixel({(int)x, (int)y})) = input == std::array{-1, -1} ? PIXEL_DESC_RGB565.build(1., 0., 0., 1.)
										: (input[0] != -1 && input[1] != -1) ? PIXEL_DESC_RGB565.build(0., 1., 0., 1.)
										: PIXEL_DESC_RGB565.build(1., 1., 1., 1.);
			}
		mapImg = r.makeTexture({mapPix.desc(), View::imageSamplerConfig});
		mapImg.write(0, mapPix, {});
		mapSpr = {{}, mapImg};
		mapSpr.setPos(padArea);
	}
}

void VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone, const Window &win)
{
	if(deadzoneMM100x != newDeadzone)
	{
		deadzoneMM100x = newDeadzone;
		deadzonePixels = win.widthMMInPixels(deadzoneMM100x / 100.);
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity)
{
	if(diagonalSensitivity_ != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity_ = newDiagonalSensitivity;
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setSize(Gfx::Renderer &r, int sizeInPixels)
{
	//logMsg("set dpad pixel size: %d", sizeInPixels);
	btnSizePixels = sizeInPixels;
	auto rect = makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {{}, {int(padBaseArea.xSize()*1.5), int(padBaseArea.xSize()*1.5)}};
	if(visualizeBounds)
	{
		if(changedSize)
			updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setPos(WP pos, WindowRect viewBounds)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(viewBounds);
	spr.setPos(padBaseArea);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea);
	}
}

void VControllerDPad::setShowBounds(Gfx::Renderer &r, bool on)
{
	if(visualizeBounds == on)
		return;
	visualizeBounds = on;
	if(!on)
	{
		if(mapSpr.hasTexture())
		{
			logMsg("deallocating bounding box display resources");
			mapSpr = {};
			mapImg = {};
		}
	}
	else
	{
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::updateMeasurements(const Window &win)
{
	deadzonePixels = win.widthMMInPixels(deadzoneMM100x / 100.);
}

void VControllerDPad::transposeKeysForPlayer(const EmuApp &app, int player)
{
	for(auto &k : keys)
	{
		k = app.transposeKeyForPlayer(k, player);
	}
}

void VControllerDPad::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden) const
{
	if(!VController::shouldDraw(state, showHidden))
		return;
	cmds.basicEffect().enableTexture(cmds);
	spr.draw(cmds);
	if(visualizeBounds)
	{
		mapSpr.draw(cmds);
	}
}

std::array<int, 2> VControllerDPad::getInput(WP c) const
{
	std::array<int, 2> pad{-1, -1};
	if(state == VControllerState::OFF || !padArea.overlaps(c))
		return pad;
	int x = c.x - padArea.xCenter(), y = c.y - padArea.yCenter();
	int xDeadzone = deadzonePixels, yDeadzone = deadzonePixels;
	if(std::abs(x) > deadzonePixels)
		yDeadzone += (std::abs(x) - deadzonePixels)/diagonalSensitivity_;
	if(std::abs(y) > deadzonePixels)
		xDeadzone += (std::abs(y) - deadzonePixels)/diagonalSensitivity_;
	//logMsg("dpad offset %d,%d, deadzone %d,%d", x, y, xDeadzone, yDeadzone);
	if(std::abs(x) > xDeadzone)
	{
		if(x > 0)
			pad[0] = keys[1]; // right
		else
			pad[0] = keys[3]; // left
	}
	if(std::abs(y) > yDeadzone)
	{
		if(y > 0)
			pad[1] = keys[2]; // down
		else
			pad[1] = keys[0]; // up
	}
	return pad;
}

}
