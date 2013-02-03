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

#define thisModuleName "vController"
#include <VController.hh>

template<>
void VController<systemFaceBtns, systemCenterBtns, systemHasTriggerBtns, systemHasRevBtnLayout>::updateMapping(uint player)
{
	updateVControllerMapping(player, map);
}

#ifdef CONFIG_VCONTROLLER_KEYBOARD
template<>
void VController<systemFaceBtns, systemCenterBtns, systemHasTriggerBtns, systemHasRevBtnLayout>::updateKeyboardMapping()
{
	updateVControllerKeyboardMapping(kb.mode, kbMap);
}
#endif

void VControllerDPad::init()
{
	origin = LT2DO;
	visualizeBounds = 0;
}

void VControllerDPad::setImg(ResourceImage *dpadR, GC texHeight)
{
	spr.init(-.5, -.5, .5, .5, dpadR);
	spr.setImg(dpadR, 0., 0., 1., 64./texHeight);
}

void VControllerDPad::updateBoundingAreaGfx()
{
	if(visualizeBounds)
	{
		mapPix.init(padArea.xSize(), padArea.ySize());
		mapImg.init(mapPix, 0, Gfx::BufferImage::linear, 0);
		mapSpr.init(&mapImg);

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
		mapSpr.setPos(padArea);
	}
}

void VControllerDPad::place(GC padFullSize, GC centerBtnYOffset)
{
	GC padAreaXOffset = Gfx::proj.aspectRatio < 1. ? Gfx::xMMSize(0.25) : Gfx::xMMSize(1.);
	GC padExtraArea = 1.5;
	padBase.init(1, 1);
	padBase.setYSize(padFullSize);
	padBase.setPos(padAreaXOffset * -origin.xScaler(), origin.onYCenter() ? 0 : centerBtnYOffset * -origin.yScaler(), origin, origin);
	spr.setPos(padBase);

	padArea.setPosRel(padBase.xIPos(C2DO), padBase.yIPos(C2DO), (GC)padBase.iYSize*padExtraArea, C2DO);

	updateBoundingAreaGfx();
}

void VControllerDPad::setBoundingAreaVisible(bool on)
{
	if(visualizeBounds == on)
		return;
	visualizeBounds = on;
	if(!on)
	{
		if(mapSpr.img)
		{
			logMsg("deallocating bounding box display resources");
			mapSpr.deinitAndFreeImg();
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
	if(padArea.overlaps(cx, cy))
	{
		int x = cx - padArea.xCenter(), y = cy - padArea.yCenter();
		int xDeadzone = deadzone, yDeadzone = deadzone;
		if(IG::abs(x) > deadzone)
			yDeadzone += (IG::abs(x) - deadzone)/diagonalSensitivity;
		if(IG::abs(y) > deadzone)
			xDeadzone += (IG::abs(y) - deadzone)/diagonalSensitivity;
		//logMsg("dpad offset %d,%d, deadzone %d,%d", x, y, xDeadzone, yDeadzone);
		int pad = 4; // init to center
		if(IG::abs(x) > xDeadzone)
		{
			if(x > 0)
				pad = 5; // right
			else
				pad = 3; // left
		}
		if(IG::abs(y) > yDeadzone)
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
		spr.setImg(spr.img, 0., .5, spr.img->textureDesc().xEnd/*384./512.*/, 1.);
	else
		spr.setImg(spr.img, 0., 0., spr.img->textureDesc().xEnd/*384./512.*/, .5);
}

void VControllerKeyboard::setImg(ResourceImage *img)
{
	spr.init(-.5, -.5, .5, .5, img);
	updateImg();
}

void VControllerKeyboard::place(GC btnSize, GC yOffset)
{
	area.init(3, 2);
	area.setXSize(IG::min(btnSize*10, Gfx::proj.w));
	GC vArea = Gfx::proj.h - yOffset*2;
	if(area.ySize > vArea)
		area.setYSize(vArea);
	area.setPos(0, yOffset, CB2DO, CB2DO);

	keyXSize = (area.iXSize / cols) + (area.iXSize * (1./256.));
	keyYSize = area.iYSize / 4;
	logMsg("key size %dx%d", keyXSize, keyYSize);

	spr.setPos(area);
}

void VControllerKeyboard::draw()
{
	spr.draw();
}

int VControllerKeyboard::getInput(int cx, int cy)
{
	if(area.overlaps(cx, cy))
	{
		int relX = cx - area.xIPos(LT2DO), relY = cy - area.yIPos(LT2DO);
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
