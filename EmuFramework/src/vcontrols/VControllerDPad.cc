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
	spr.set(img);
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r)
{
	if(config.visualizeBounds && padArea.xSize())
	{
		MemPixmap mapMemPix{{padArea.size(), PIXEL_FMT_RGB565}};
		auto mapPix = mapMemPix.view();
		auto pixels = mapPix.mdspan<uint16_t>();
		for(auto y : iotaCount(pixels.extent(0)))
			for(auto x : iotaCount(pixels.extent(1)))
			{
				auto input = getInput({padArea.xPos(LT2DO) + int(x), padArea.yPos(LT2DO) + int(y)});
				//logMsg("got input %d", input);
				pixels[y, x] = input == std::array{-1, -1} ? PIXEL_DESC_RGB565.build(1., 0., 0.)
										: (input[0] != -1 && input[1] != -1) ? PIXEL_DESC_RGB565.build(0., 1., 0.)
										: PIXEL_DESC_RGB565.build(1., 1., 1.);
			}
		mapImg = r.makeTexture({mapPix.desc(), View::imageSamplerConfig});
		mapImg.write(0, mapPix, {});
		mapSpr.set(mapImg);
		mapSpr.setPos(padArea);
	}
}

static bool isValidDeadzone(int val) { return val >= 100 && val <= 300; }

bool VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone, const Window &win)
{
	if(!isValidDeadzone(newDeadzone))
		return false;
	if(config.deadzoneMM100x == newDeadzone)
		return true;
	config.deadzoneMM100x = newDeadzone;
	deadzonePixels = win.widthMMInPixels(newDeadzone / 100.f);
	updateBoundingAreaGfx(r);
	return true;
}

static bool isValidDiagonalSensitivity(float val) { return val >= .01f && val <= 1.f; }

bool VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity)
{
	if(!isValidDiagonalSensitivity(newDiagonalSensitivity))
		return false;
	if(config.diagonalSensitivity == newDiagonalSensitivity)
		return true;
	logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
	config.diagonalSensitivity = newDiagonalSensitivity;
	updateBoundingAreaGfx(r);
	return true;
}

void VControllerDPad::setSize(Gfx::Renderer &r, int sizeInPixels)
{
	//logMsg("set dpad pixel size: %d", sizeInPixels);
	btnSizePixels = sizeInPixels;
	auto rect = makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {{}, {int(padBaseArea.xSize() * 1.5f), int(padBaseArea.xSize() * 1.5f)}};
	if(config.visualizeBounds)
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
	if(config.visualizeBounds)
	{
		mapSpr.setPos(padArea);
	}
}

void VControllerDPad::setShowBounds(Gfx::Renderer &r, bool on)
{
	if(config.visualizeBounds == on)
		return;
	config.visualizeBounds = on;
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
	deadzonePixels = win.widthMMInPixels(config.deadzoneMM100x / 100.f);
}

void VControllerDPad::transposeKeysForPlayer(const EmuApp &app, int player)
{
	for(auto &k : config.keys)
	{
		k = app.transposeKeyForPlayer(k, player);
	}
}

void VControllerDPad::draw(Gfx::RendererCommands &__restrict__ cmds, float alpha) const
{
	cmds.basicEffect().enableTexture(cmds);
	cmds.setColor({alpha, alpha, alpha, alpha});
	spr.draw(cmds);
	if(config.visualizeBounds)
	{
		mapSpr.draw(cmds);
	}
}

std::array<int, 2> VControllerDPad::getInput(WP c) const
{
	std::array<int, 2> pad{-1, -1};
	if(!padArea.overlaps(c))
		return pad;
	c -= padArea.center();
	int xDeadzone = deadzonePixels, yDeadzone = deadzonePixels;
	if(std::abs(c.x) > deadzonePixels)
		yDeadzone += (std::abs(c.x) - deadzonePixels) * config.diagonalSensitivity;
	if(std::abs(c.y) > deadzonePixels)
		xDeadzone += (std::abs(c.y) - deadzonePixels) * config.diagonalSensitivity;
	if(std::abs(c.x) > xDeadzone)
	{
		if(c.x > 0)
			pad[0] = config.keys[1]; // right
		else
			pad[0] = config.keys[3]; // left
	}
	if(std::abs(c.y) > yDeadzone)
	{
		if(c.y > 0)
			pad[1] = config.keys[2]; // down
		else
			pad[1] = config.keys[0]; // up
	}
	return pad;
}

void VControllerDPad::Config::validate(const EmuApp &app)
{
	for(auto &k : keys) { k = app.validateSystemKey(k, false); }
	if(!isValidDiagonalSensitivity(diagonalSensitivity))
		diagonalSensitivity = defaultDPadDiagonalSensitivity;
	if(!isValidDeadzone(deadzoneMM100x))
		deadzoneMM100x = defaultDPadDeadzoneMM100x;
}

}
