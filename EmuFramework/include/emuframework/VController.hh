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
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/base/Base.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/TurboInput.hh>
#include <cmath>

extern TurboInput turboActions;

class VControllerDPad
{
public:
	Gfx::Sprite spr{};
	uint state = 1;

	constexpr VControllerDPad() {}
	void init();
	void setImg(Gfx::PixmapTexture &dpadR, Gfx::GTexC texHeight);
	void draw() const;
	void setBoundingAreaVisible(bool on);
	int getInput(int cx, int cy) const;
	IG::WindowRect bounds() const;
	void setPos(IG::Point2D<int> pos);
	void setSize(uint sizeInPixels);
	void setDeadzone(int newDeadzone);
	void setDiagonalSensitivity(float newDiagonalSensitivity);

private:
	Gfx::GCRect padBase{};
	IG::WindowRect padBaseArea{}, padArea{};
	int deadzone = 0;
	float diagonalSensitivity = 1.;
	Gfx::PixmapTexture mapImg{};
	Gfx::Sprite mapSpr{};
	bool visualizeBounds = 0;
	int btnSizePixels = 0;

	void updateBoundingAreaGfx();
};

class VControllerKeyboard
{
public:
	Gfx::Sprite spr{};
	IG::WindowRect bound{};
	uint keyXSize = 0, keyYSize = 0;
	static const uint cols = 10;
	uint mode = 0;
	Gfx::GTexC texXEnd = 0;

	constexpr VControllerKeyboard() {}
	void init();
	void updateImg();
	void setImg(Gfx::PixmapTexture *img);
	void place(Gfx::GC btnSize, Gfx::GC yOffset);
	void draw() const;
	int getInput(int cx, int cy) const;
};

class VControllerGamepad
{
public:
	static constexpr uint MAX_CENTER_BTNS = 2;
	static constexpr uint MAX_FACE_BTNS = 8;
	VControllerDPad dp{};
	int btnSpacePixels = 0, btnStaggerPixels = 0, btnRowShiftPixels = 0;
	Gfx::GC btnSpace = 0, btnStagger = 0, btnRowShift = 0;//, btnAreaXOffset = 0;
	Gfx::GC btnExtraXSize = 0.001, btnExtraYSize = 0.001, btnExtraYSizeMultiRow = 0.001;
	uint centerBtnsState = 1;
	uint lTriggerState = 1;
	uint rTriggerState = 1;
	uint faceBtnsState = 1;
	uint activeFaceBtns = 0;
	bool triggersInline = false;

	constexpr VControllerGamepad() {}
	void init(float alpha);
	void setBoundingAreaVisible(bool on);
	bool boundingAreaVisible() const;
	void setImg(Gfx::PixmapTexture &pics);
	uint rowsForButtons(uint activeButtons);
	void setBaseBtnSize(uint sizeInPixels);
	IG::WindowRect centerBtnBounds() const;
	void setCenterBtnPos(IG::Point2D<int> pos);
	IG::WindowRect lTriggerBounds() const;
	void setLTriggerPos(IG::Point2D<int> pos);
	IG::WindowRect rTriggerBounds() const;
	void setRTriggerPos(IG::Point2D<int> pos);
	void layoutBtnRows(uint a[], uint btns, uint rows, IG::Point2D<int> pos);
	IG::WindowRect faceBtnBounds() const;
	void setFaceBtnPos(IG::Point2D<int> pos);
	std::array<int, 2> getCenterBtnInput(int x, int y) const;
	std::array<int, 2> getBtnInput(int x, int y) const;
	void draw(bool showHidden) const;

private:
	IG::WindowRect centerBtnBound[MAX_CENTER_BTNS]{};
	IG::WindowRect centerBtnsBound{};
	IG::WindowRect faceBtnBound[MAX_FACE_BTNS]{};
	IG::WindowRect faceBtnsBound{}, lTriggerBound{}, rTriggerBound{};
	Gfx::Sprite centerBtnSpr[MAX_CENTER_BTNS]{};
	Gfx::Sprite circleBtnSpr[MAX_FACE_BTNS]{};
	bool showBoundingArea = false;
	Gfx::GC btnSize = 0;
	int btnSizePixels = 0;
};

class VController
{
public:
	static constexpr int C_ELEM = 0, F_ELEM = 8, D_ELEM = 32;
	static constexpr uint TURBO_BIT = IG::bit(31), ACTION_MASK = 0x7FFFFFFF;
	using Map = std::array<uint, D_ELEM+9>;
	using KbMap = std::array<uint, 40>;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	VControllerGamepad gp{};
	#endif
	VControllerKeyboard kb{};

	// menu button
	Gfx::Sprite menuBtnSpr{};
	IG::WindowRect menuBound{};
	uint menuBtnState = 1;

	// fast-forward button
	Gfx::Sprite ffBtnSpr{};
	IG::WindowRect ffBound{};
	uint ffBtnState = 1;

	#ifdef CONFIG_BASE_ANDROID
	bool useScaledCoordinates = true;
	#else
	static constexpr bool useScaledCoordinates = false;
	#endif

	float alpha = 0;

	constexpr VController() {}
	Gfx::GC xMMSize(Gfx::GC mm) const;
	Gfx::GC yMMSize(Gfx::GC mm) const;
	int xMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const;
	int yMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const;
	void updateMapping(uint player);
	void updateKeyboardMapping();
	bool hasTriggers() const;
	void setImg(Gfx::PixmapTexture &pics);
	void setBoundingAreaVisible(bool on);
	bool boundingAreaVisible() const;
	void setMenuBtnPos(IG::Point2D<int> pos);
	void setFFBtnPos(IG::Point2D<int> pos);
	void inputAction(uint action, uint vBtn);
	void resetInput(bool init = 0);
	void init(float alpha, uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP);
	void place();
	void toggleKeyboard();
	std::array<int, 2> findElementUnderPos(Input::Event e);
	void applyInput(Input::Event e);
	void draw(bool emuSystemControls, bool activeFF, bool showHidden = false);
	void draw(bool emuSystemControls, bool activeFF, bool showHidden, float alpha);
	int numElements() const;
	IG::WindowRect bounds(int elemIdx) const;
	void setPos(int elemIdx, IG::Point2D<int> pos);
	void setState(int elemIdx, uint state);
	uint state(int elemIdx) const;
	void setBaseBtnSize(uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP);
	bool isInKeyboardMode() const;

private:
	std::array<std::array<int, 2>, Config::Input::MAX_POINTERS> ptrElem{};
	Map map{};
	uint kbMode = 0;
	KbMap kbMap{};
};

using SysVController = VController;
extern SysVController vController;
void updateVControllerMapping(uint player, SysVController::Map &map);
void updateVControllerKeyboardMapping(uint mode, SysVController::KbMap &map);
