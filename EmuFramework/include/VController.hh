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

#pragma once
#include <util/number.h>
#include <util/area2.h>
#include <gfx/GfxSprite.hh>
#include <base/Base.hh>
#include <input/DragPointer.hh>
#include <resource2/image/ResourceImage.h>
#include <EmuOptions.hh>
#include <EmuSystem.hh>

class VControllerDPad
{
public:
	constexpr VControllerDPad() { }
	Area padBase;
	Rect2<int> padArea;
	int deadzone = 0;
	float diagonalSensitivity = 0;
	Gfx::Sprite spr;
	_2DOrigin origin;

	GfxBufferImage mapImg;
	Pixmap mapPix {PixelFormatRGB565};
	Gfx::Sprite mapSpr;
	bool visualizeBounds = 0;

	void init();
	void setImg(ResourceImage *dpadR, GC texHeight);
	void place(GC padFullSize, GC centerBtnYOffset);
	void draw();
	void setBoundingAreaVisible(bool on);
	int getInput(int cx, int cy);
private:
	void updateBoundingAreaGfx();
};

class VControllerKeyboard
{
public:
	Gfx::Sprite spr;
	Area area;
	uint keyXSize, keyYSize;
	static const uint cols = 10;
	uint mode;

	void init();
	void updateImg();
	void setImg(ResourceImage *img);
	void place(GC btnSize, GC yOffset);
	void draw();
	int getInput(int cx, int cy);
};

template <uint faceBtns, uint centerBtns = 2, bool hasTriggerButtons = 0, bool revFaceMapping = 0>
class VControllerGamepad
{
public:
	constexpr VControllerGamepad() { }
	Rect2<int> faceBtnBound[faceBtns], centerBtnBound[centerBtns];
	Gfx::Sprite circleBtnSpr[faceBtns], centerBtnSpr[centerBtns];
	Area faceBtn[faceBtns], centerBtn[centerBtns];
	VControllerDPad dp;
	Area btnArea;
	uint triggerPos = 0;
	uint activeFaceBtns = faceBtns;
	GC btnSize = 0, btnSpace = 0, btnStagger = 0, btnRowShift = 0;
	GC btnExtraXSize = 0.001, btnExtraYSize = 0.001, btnExtraYSizeMultiRow = 0.001;
	_2DOrigin btnO {RB2DO}, cenBtnO;
	bool showBoundingArea = 0;

	void init(float alpha, GC size)
	{
		dp.init();

		forEachInArray(faceBtn, e)
		{
			e->init(1, 1);
		}

		forEachInArray(centerBtn, e)
		{
			e->init(0, 1);
		}

		btnSize = size;
	}

	void setBoundingAreaVisible(bool on)
	{
		showBoundingArea = on;
		dp.setBoundingAreaVisible(on);
	}

	bool boundingAreaVisible()
	{
		return showBoundingArea;
	}

	void setImg(ResourceImage *pics)
	{
		assert(pics);
		if(circleBtnSpr[0].img)
		{
			circleBtnSpr[0].img->deinit();
		}
		GC h = faceBtns == 2 ? 128. : 256.;
		dp.setImg(pics, h);
		forEachInArray(centerBtnSpr, e)
		{
			e->init(pics);
		}
		centerBtnSpr[0].setImg(pics, 0., 65./h, 32./64., 81./h);
		if (centerBtns == 2)
		{
			centerBtnSpr[1].setImg(pics, 33./64., 65./h, 1., 81./h);
		}

		forEachInArray(circleBtnSpr, e)
		{
			e->init(pics);
		}
		if (faceBtns == 2)
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
			if(faceBtns >= 6)
			{
				circleBtnSpr[4].setImg(pics, 0., 148./h, 32./64., 180./h);
				circleBtnSpr[5].setImg(pics, 33./64., 149./h, 1., 180./h);
			}
			if(faceBtns == 8)
			{
				circleBtnSpr[6].setImg(pics, 0., 181./h, 32./64., 213./h);
				circleBtnSpr[7].setImg(pics, 33./64., 182./h, 1., 213./h);
			}
		}
	}

	void layoutBtnRows(Area *a[], uint btns, uint rows, GC centerBtnYOffset, GC btnAreaXOffset)
	{
		int btnsPerRow = btns/rows;
		//Area btnArea; // area to place face buttons
		btnArea.init(0, 1);
		btnArea.setYSize(btnSize*rows + btnSpace*(rows-1) + IG::abs(btnStagger*(btnsPerRow-1)));
		btnArea.setXSize(btnSize*btnsPerRow + btnSpace*(btnsPerRow-1) + IG::abs(btnRowShift*(rows-1)));
		btnArea.setPos(btnAreaXOffset * -btnO.xScaler(), btnO.onYCenter() ? 0 : centerBtnYOffset * -btnO.yScaler(), btnO, btnO);

		int row = 0, btnPos = 0;
		GC yOffset = (btnStagger < 0) ? -btnStagger*(btnsPerRow-1) : 0,
			xOffset = -btnRowShift*(rows-1),
			staggerOffset = 0;
		iterateTimes(btns, i)
		{
			a[i]->setPos(btnArea.xPos(LB2DO) + xOffset, btnArea.yPos(LB2DO) + yOffset + staggerOffset, LB2DO, C2DO);
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
		}
	}

	void place()
	{
		using namespace IG;
		assert(btnSize != 0);

		forEachInArray(faceBtn, e)
		{
			e->setYSize(btnSize);
		}

		forEachInArray(centerBtn, e)
		{
			e->setXSize(btnSize);
			e->setYSize(btnSize * .5);
		}

		// D-Pad
		GC centerBtnYOffset = centerBtn[0].ySize * 1.5;
		if(optionTouchCtrlDpadPos != NULL2DO)
		{
			GC padFullSize = btnSize*2.5;
			if(btnRowShift != 0)
				padFullSize = btnSize*2.8;
			dp.place(padFullSize, centerBtnYOffset);
		}

		// Face Buttons
		GC btnAreaXOffset = Gfx::proj.aspectRatio < 1. ? Gfx::xMMSize(0.25) : Gfx::xMMSize(1.);
		GC buttonXSpace = btnExtraXSize ? btnSpace * 2 : btnSpace;
		GC buttonYSpace = btnExtraYSize ? btnSpace * 2 : btnSpace;
		if(optionTouchCtrlFaceBtnPos != NULL2DO)
		{
			uint extraXSize = Gfx::toIXSize(buttonXSpace + btnSize * btnExtraXSize);
			if(!hasTriggerButtons)
			{
				Area *btnMap[] = { &faceBtn[1], &faceBtn[0] };
				Area *btnMap2Rev[] = { &faceBtn[0], &faceBtn[1] };
				Area *btnMap3[] = { &faceBtn[0], &faceBtn[1], &faceBtn[2] };
				Area *btnMap4[] = { &faceBtn[0], &faceBtn[1], &faceBtn[2], &faceBtn[3] };
				Area *btnMap6Rev[] = { &faceBtn[0], &faceBtn[1], &faceBtn[2], &faceBtn[3], &faceBtn[4], &faceBtn[5] };
				Area *btnMap6[] = { &faceBtn[2], &faceBtn[1], &faceBtn[0], &faceBtn[3], &faceBtn[4], &faceBtn[5] };
				uint rows = (activeFaceBtns > 3) ? 2 : 1;
				if(activeFaceBtns == 6)
					layoutBtnRows(revFaceMapping ? btnMap6Rev : btnMap6, sizeofArray(btnMap6), rows, centerBtnYOffset, btnAreaXOffset);
				else if(activeFaceBtns == 4)
					layoutBtnRows(btnMap4, sizeofArray(btnMap4), rows, centerBtnYOffset, btnAreaXOffset);
				else if(activeFaceBtns == 3)
					layoutBtnRows(btnMap3, sizeofArray(btnMap3), rows, centerBtnYOffset, btnAreaXOffset);
				else
					layoutBtnRows(revFaceMapping ? btnMap2Rev : btnMap, sizeofArray(btnMap), rows, centerBtnYOffset, btnAreaXOffset);
				uint extraYSize = Gfx::toIYSize(buttonYSpace + btnSize * (rows == 1 ? btnExtraYSize : btnExtraYSizeMultiRow));
				forEachInArray(faceBtnBound, e)
				{
					e->setPosRel(faceBtn[e_i].xIPos(C2DO), faceBtn[e_i].yIPos(C2DO),
							faceBtn[0].iXSize + extraXSize, faceBtn[0].iYSize + extraYSize,
							C2DO);
				}
			}
			else
			{
				if(triggerPos == TRIGGERS_INLINE)
				{
					Area *btnMap8[] = { &faceBtn[0], &faceBtn[1], &faceBtn[2], &faceBtn[6], &faceBtn[3], &faceBtn[4], &faceBtn[5],  &faceBtn[7] };
					Area *btnMap6[] = { &faceBtn[1], &faceBtn[0], &faceBtn[5], &faceBtn[3], &faceBtn[2], &faceBtn[4] };
					Area *btnMap4[] = { &faceBtn[1], &faceBtn[0], &faceBtn[2], &faceBtn[3] };
					if(activeFaceBtns == 8)
						layoutBtnRows(btnMap8, sizeofArray(btnMap8), 2, centerBtnYOffset, btnAreaXOffset);
					else if(activeFaceBtns == 6)
						layoutBtnRows(btnMap6, sizeofArray(btnMap6), 2, centerBtnYOffset, btnAreaXOffset);
					else
						layoutBtnRows(btnMap4, sizeofArray(btnMap4), 2, centerBtnYOffset, btnAreaXOffset);
				}
				else
				{
					Area *btnMap8[] = { &faceBtn[0], &faceBtn[1], &faceBtn[2], &faceBtn[3], &faceBtn[4], &faceBtn[5], };
					Area *btnMap6[] = { &faceBtn[1], &faceBtn[0], &faceBtn[3], &faceBtn[2] };
					Area *btnMap4[] = { &faceBtn[1], &faceBtn[0] };
					if(activeFaceBtns == 8)
						layoutBtnRows(btnMap8, sizeofArray(btnMap8), 2, centerBtnYOffset, btnAreaXOffset);
					else if(activeFaceBtns == 6)
						layoutBtnRows(btnMap6, sizeofArray(btnMap6), 2, centerBtnYOffset, btnAreaXOffset);
					else
						layoutBtnRows(btnMap4, sizeofArray(btnMap4), 1, centerBtnYOffset, btnAreaXOffset);
					if(triggerPos == TRIGGERS_RIGHT)
					{
						_2DOrigin lO = btnO.onTop() ? LB2DO : LT2DO;
						_2DOrigin rO = btnO.onTop() ? RB2DO : RT2DO;
						faceBtn[faceBtns-2].setPos(btnArea.xPos(lO), btnArea.yPos(lO) - btnSpace * signOf(btnO.yScaler()), lO.flipY(), C2DO);
						faceBtn[faceBtns-1].setPos(btnArea.xPos(rO), btnArea.yPos(rO) - btnSpace * signOf(btnO.yScaler()), rO.flipY(), C2DO);
					}
					else if(triggerPos == TRIGGERS_LEFT)
					{
						_2DOrigin lO = dp.origin.onTop() ? LB2DO : LT2DO;
						_2DOrigin rO = dp.origin.onTop() ? RB2DO : RT2DO;
						faceBtn[faceBtns-2].setPos(dp.padBase.xPos(lO), dp.padBase.yPos(lO) - btnSpace * signOf(dp.origin.yScaler()), lO.flipY(), C2DO);
						faceBtn[faceBtns-1].setPos(dp.padBase.xPos(rO), dp.padBase.yPos(rO) - btnSpace * signOf(dp.origin.yScaler()), rO.flipY(), C2DO);
					}
					else
					{
						_2DOrigin lO = dp.origin.onTop() ? LB2DO : LT2DO;
						_2DOrigin rO = btnO.onTop() ? RB2DO : RT2DO;
						faceBtn[faceBtns-2].setPos(dp.padBase.xPos(lO), dp.padBase.yPos(lO) - btnSpace * signOf(dp.origin.yScaler()), lO.flipY(), C2DO);
						faceBtn[faceBtns-1].setPos(btnArea.xPos(rO), btnArea.yPos(rO) - btnSpace * signOf(btnO.yScaler()), rO.flipY(), C2DO);
					}
				}
				uint extraYSize = Gfx::toIYSize(buttonYSpace + btnSize * btnExtraYSizeMultiRow);
				forEachInArray(faceBtnBound, e)
				{
					bool useExtraSize = 0;
					if((int)e_i < (int)faceBtns-2 || triggerPos == TRIGGERS_INLINE)
						useExtraSize = 1;
					e->setPosRel(faceBtn[e_i].xIPos(C2DO), faceBtn[e_i].yIPos(C2DO),
						faceBtn[0].iXSize + (useExtraSize ? extraXSize : 0),
						faceBtn[0].iYSize + (useExtraSize ? extraYSize : 0),
						C2DO);
				}
			}

			forEachInArray(circleBtnSpr, e)
			{
				e->setPos(faceBtn[e_i]);
			}
		}

		// Center Buttons
		{
			Area cBtnArea;
			cBtnArea.init(0, 1);
			cBtnArea.setYSize(centerBtn[0].ySize * 1.5);
			if(centerBtns == 2)
			{
				cBtnArea.setXSize((btnSize*2) + btnSpace);
				cBtnArea.setPos(btnAreaXOffset * -cenBtnO.xScaler(), 0, cenBtnO, cenBtnO);
				centerBtn[0].setPos(cBtnArea.xPos(LC2DO), cBtnArea.yPos(LC2DO), LC2DO, C2DO);
				centerBtn[1].setPos(centerBtn[0].xPos(RC2DO)+btnSpace, cBtnArea.yPos(LC2DO), LC2DO, C2DO);
			}
			else
			{
				cBtnArea.setXSize(btnSize);
				cBtnArea.setPos(btnAreaXOffset * -cenBtnO.xScaler(), 0, cenBtnO, cenBtnO);
				centerBtn[0].setPos(cBtnArea.xPos(LC2DO), cBtnArea.yPos(LC2DO), LC2DO, C2DO);
			}
			uint extraXSize = Gfx::toIXSize(buttonXSpace + btnSize * btnExtraXSize);
			if(sizeofArray(centerBtnBound) == 2) // special bounding arrangement for 2 buttons
			{
				centerBtnBound[0].setPosRel(centerBtn[0].xIPos(LC2DO), centerBtn[0].yIPos(LC2DO),
											centerBtn[0].iXSize + extraXSize/2, cBtnArea.iYSize, LC2DO);
				centerBtnBound[1].setPosRel(centerBtn[1].xIPos(RC2DO), centerBtn[1].yIPos(RC2DO),
											centerBtn[0].iXSize + extraXSize/2, cBtnArea.iYSize, RC2DO);
			}
			else
			{
				forEachInArray(centerBtnBound, e)
				{
					e->setPosRel(centerBtn[e_i].xIPos(C2DO), centerBtn[e_i].yIPos(C2DO),
							centerBtn[0].iXSize + extraXSize, cBtnArea.iYSize, C2DO);
				}
			}
			forEachInArray(centerBtnSpr, e)
			{
				e->setPos(centerBtn[e_i]);
			}
		}

		// TODO: check if overlapping game area and enable/disable blending
	}

	void getCenterBtnInput(int x, int y, int btnOut[2])
	{
		uint count = 0;
		forEachInArray(centerBtnBound, e)
		{
			if(e->overlaps(x, y))
			{
				//logMsg("overlaps %d", (int)e_i);
				btnOut[count] = e_i;
				count++;
				if(count == 2)
					return;
			}
		}
	}

	void getBtnInput(int x, int y, int btnOut[2])
	{
		uint count = 0;
		forEachInArray(faceBtnBound, e)
		{
			if(e_i == activeFaceBtns)
				break;

			if(e->overlaps(x, y))
			{
				//logMsg("overlaps %d", (int)e_i);
				btnOut[count] = e_i;
				count++;
				if(count == 2)
					return;
			}
		}
	}

	void draw()
	{
		if(optionTouchCtrlDpadPos != NULL2DO)
		{
			dp.draw();
		}

		if(optionTouchCtrlFaceBtnPos != NULL2DO)
		{
			//gfx_resetTransforms(); GeomRect::draw(&btnArea);
			forEachInArray(faceBtn, e)
			{
				if(e_i == activeFaceBtns)
					break;
				//if(!circleBtnSpr.img) break;
				//if(faceBtnI[e_i])
				if(showBoundingArea)
					{ Gfx::resetTransforms(); GeomRect::draw(faceBtnBound[e_i]); }
				circleBtnSpr[e_i].draw();
			}
		}

		forEachInArray(centerBtn, e)
		{
			//if(!circleBtnSpr.img) break;
			//if(centerBtnI[e_i])
			if(showBoundingArea)
				{ Gfx::resetTransforms(); GeomRect::draw(centerBtnBound[e_i]); }
			centerBtnSpr[e_i].draw();
		}
	}
};

template <uint faceBtns, uint centerBtns, bool hasTriggerButtons = 0, bool revFaceMapping = 0>
class VController
{
public:
	int ptrElem[Input::maxCursors][2], prevPtrElem[Input::maxCursors][2];
	VControllerGamepad<faceBtns, centerBtns, hasTriggerButtons, revFaceMapping> gp;
	float alpha;
	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	VControllerKeyboard kb;
	uint kbMode;
	#endif

	bool hasTriggers() const
	{
		return hasTriggerButtons;
	}

	void setImg(ResourceImage *pics)
	{
		gp.setImg(pics);
	}

	void setBoundingAreaVisible(bool on)
	{
		gp.setBoundingAreaVisible(on);
	}

	bool boundingAreaVisible()
	{
		return gp.boundingAreaVisible();
	}

	void resetInput(bool init = 0)
	{
		iterateTimes(Input::maxCursors, i)
		{
			iterateTimes(2, j)
			{
				if(!init && ptrElem[i][j] != -1) // release old key, if any
					EmuSystem::handleOnScreenInputAction(INPUT_RELEASED, ptrElem[i][j]);
				ptrElem[i][j] = -1;
				prevPtrElem[i][j] = -1;
			}
		}
	}

	void init(float alpha, GC size)
	{
		var_selfs(alpha);
		gp.init(alpha, size);
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		kb.init();
		kbMode = 0;
		#endif
		resetInput(1);
	}

	void place()
	{
		gp.place();
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		kb.place(gp.btnSize, gp.centerBtn[0].ySize * 1.5);
		#endif
		resetInput();
	}

	#ifdef CONFIG_VCONTROLLER_KEYBOARD
	void toggleKeyboard()
	{
		logMsg("toggling keyboard");
		resetInput();
		toggle(kbMode);
	}
	#endif

	static const int C_ELEM = 0, F_ELEM = 8, D_ELEM = 32;

	void findElementUnderPos(const InputEvent &e, int elemOut[2])
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
			}
			else
				elemOut[0] = kbChar;
			return;
		}
		#endif

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

		if(optionTouchCtrlFaceBtnPos != NULL2DO)
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

		if(optionTouchCtrlDpadPos != NULL2DO)
		{
			int elem = gp.dp.getInput(e.x, e.y);
			if(elem != -1)
			{
				elemOut[0] = D_ELEM + elem;
				return;
			}
		}
	}

	void applyInput(const InputEvent &e)
	{
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
				EmuSystem::handleOnScreenInputAction(INPUT_RELEASED, vBtn);
			}
		}

		// push new buttons
		iterateTimes(2, i)
		{
			auto vBtn = elem[i];
			if(vBtn != -1 && !mem_findFirstValue(ptrElem[e.devId], vBtn))
			{
				//logMsg("pushing %d", vBtn);
				EmuSystem::handleOnScreenInputAction(INPUT_PUSHED, vBtn);
				if(optionVibrateOnPush)
				{
					Base::vibrate(32);
				}
			}
		}

		memcpy(ptrElem[e.devId], elem, sizeof(elem));
	}

	void draw()
	{
		draw(alpha);
	}

	void draw(float alpha)
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
		gp.draw();
	}
};

typedef VController<systemFaceBtns, systemCenterBtns, systemHasTriggerBtns, systemHasRevBtnLayout> SysVController;
extern SysVController vController;
