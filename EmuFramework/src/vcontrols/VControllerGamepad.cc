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
#include <imagine/util/math/int.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/gui/View.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

void VControllerDPad::setImg(Gfx::Renderer &r, Gfx::Texture &dpadR, float texHeight)
{
	spr = {{{-.5, -.5}, {.5, .5}}, {&dpadR, {{}, {1., 64.f/texHeight}}}};
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r, Gfx::ProjectionPlane projP)
{
	if(visualizeBounds && padArea.xSize())
	{
		IG::MemPixmap mapMemPix{{padArea.size(), IG::PIXEL_FMT_RGB565}};
		auto mapPix = mapMemPix.view();
		for(auto y : iotaCount(mapPix.h()))
			for(auto x : iotaCount(mapPix.w()))
			{
				int input = getInput({padArea.xPos(LT2DO) + (int)x, padArea.yPos(LT2DO) + (int)y});
				//logMsg("got input %d", input);
				*((uint16_t*)mapPix.pixel({(int)x, (int)y})) = input == -1 ? IG::PIXEL_DESC_RGB565.build(1., 0., 0., 1.)
										: IG::isOdd(input) ? IG::PIXEL_DESC_RGB565.build(1., 1., 1., 1.)
										: IG::PIXEL_DESC_RGB565.build(0., 1., 0., 1.);
			}
		mapImg = r.makeTexture({mapPix.desc(), &r.make(View::imageCommonTextureSampler)});
		mapImg.write(0, mapPix, {});
		mapSpr = {{}, mapImg};
		mapSpr.setPos(padArea, projP);
	}
}

void VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone, Gfx::ProjectionPlane projP)
{
	if(deadzone != newDeadzone)
	{
		deadzone = newDeadzone;
		updateBoundingAreaGfx(r, projP);
	}
}

void VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity, Gfx::ProjectionPlane projP)
{
	if(diagonalSensitivity != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity = newDiagonalSensitivity;
		updateBoundingAreaGfx(r, projP);
	}
}

IG::WindowRect VControllerDPad::bounds() const
{
	return padBaseArea;
}

void VControllerDPad::setSize(Gfx::Renderer &r, int sizeInPixels, Gfx::ProjectionPlane projP)
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
			updateBoundingAreaGfx(r, projP);
	}
}

void VControllerDPad::setPos(IG::WP pos, IG::WindowRect viewBounds, Gfx::ProjectionPlane projP)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(viewBounds);
	padBase = projP.unProjectRect(padBaseArea);
	spr.setPos(padBase);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea, projP);
	}
}

void VControllerDPad::setBoundingAreaVisible(Gfx::Renderer &r, bool on, Gfx::ProjectionPlane projP)
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
		updateBoundingAreaGfx(r, projP);
	}
}

void VControllerDPad::draw(Gfx::RendererCommands &cmds) const
{
	cmds.basicEffect().enableTexture(cmds);
	cmds.set(View::imageCommonTextureSampler);
	spr.draw(cmds);
	if(visualizeBounds)
	{
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

VControllerGamepad::VControllerGamepad(int faceButtons, int centerButtons):
	centerBtns{centerButtons},
	faceBtns{faceButtons}
{
	if(EmuSystem::inputHasTriggers())
	{
		lTriggerPtr = &faceBtns.buttons()[EmuSystem::inputLTriggerIndex];
		rTriggerPtr = &faceBtns.buttons()[EmuSystem::inputRTriggerIndex];
	}
}

void VControllerGamepad::setBoundingAreaVisible(Gfx::Renderer &r, bool on, Gfx::ProjectionPlane projP)
{
	dp.setBoundingAreaVisible(r, on, projP);
	centerBtns.setShowBounds(on);
	faceBtns.setShowBounds(on);
}

static FRect faceButtonCoordinates(int slot, float texHeight)
{
	switch(slot)
	{
		case 0: return {{0., 82.f/texHeight}, {32./64., 114.f/texHeight}};
		case 1: return {{33./64., 83.f/texHeight}, {1., 114.f/texHeight}};
		case 2:
			if(texHeight == 128.f)
				return {{0., 82.f/texHeight}, {32./64., 114.f/texHeight}};
			else
				return {{0., 115.f/texHeight}, {32./64., 147.f/texHeight}};
		case 3:
			if(texHeight == 128.f)
				return {{33./64., 83.f/texHeight}, {1., 114.f/texHeight}};
			else
				return {{33./64., 116.f/texHeight}, {1., 147.f/texHeight}};
		case 4: return {{0., 148.f/texHeight}, {32./64., 180.f/texHeight}};
		case 5: return {{33./64., 149.f/texHeight}, {1., 180.f/texHeight}};
		case 6: return {{0., 181.f/texHeight}, {32./64., 213.f/texHeight}};
		case 7: return {{33./64., 182.f/texHeight}, {1., 213.f/texHeight}};
	}
	bug_unreachable("invalid slot:%d", slot);
}

void VControllerGamepad::setImg(Gfx::Renderer &r, Gfx::Texture &pics)
{
	float h = EmuSystem::inputFaceBtns == 2 || EmuSystem::inputHasShortBtnTexture ? 128. : 256.;
	dp.setImg(r, pics, h);
	centerBtns.buttons()[0].setImage({&pics, {{0., 65.f/h}, {32./64., 81.f/h}}}, 2.f);
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtns.buttons()[1].setImage({&pics, {{33./64., 65.f/h}, {1., 81.f/h}}}, 2.f);
	}
	auto faceBtnMap = EmuSystem::vControllerImageMap;
	for(auto i : iotaCount(EmuSystem::inputFaceBtns))
	{
		faceBtns.buttons()[i].setImage({&pics, faceButtonCoordinates(faceBtnMap[i], h)});
	}
}

void VControllerGamepad::setFaceButtonSize(Gfx::Renderer &r, IG::WP sizeInPixels, IG::WP extraSizePixels, Gfx::ProjectionPlane projP)
{
	dp.setSize(r, IG::makeEvenRoundedUp(int(sizeInPixels.x*(double)2.5)), projP);
	faceBtns.setButtonSize(sizeInPixels, extraSizePixels);
}

void VControllerGamepad::drawDPads(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane) const
{
	if(VController::shouldDraw(dp.state(), showHidden))
	{
		dp.draw(cmds);
	}
}

void VControllerGamepad::drawButtons(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane projP) const
{
	faceBtns.draw(cmds, projP, showHidden);
	centerBtns.draw(cmds, projP, showHidden);
}

void VControllerGamepad::draw(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane projP) const
{
	drawDPads(cmds, showHidden, projP);
	drawButtons(cmds, showHidden, projP);
}

void VControllerGamepad::setSpacingPixels(int space)
{
	centerBtns.setSpacing(space);
	faceBtns.setSpacing(space);
}

void VControllerGamepad::setStaggerType(int type)
{
	faceBtns.setStaggerType(type);
}

void VControllerGamepad::setTriggersInline(bool on)
{
	if(!EmuSystem::inputHasTriggers())
		return;
	lTriggerPtr->setShouldSkipLayout(!on);
	rTriggerPtr->setShouldSkipLayout(!on);
}

bool VControllerGamepad::triggersInline() const
{
	if(!EmuSystem::inputHasTriggers())
		return false;
	return !lTriggerPtr->shouldSkipLayout();
}

int VControllerGamepad::faceButtonRows() const
{
	return faceBtns.rows();
}

}
