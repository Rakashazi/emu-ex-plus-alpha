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
#include <emuframework/EmuSystem.hh>
#include <emuframework/TurboInput.hh>
#include <cmath>

class VControllerDPad
{
public:
	constexpr VControllerDPad() {}
	void setImg(Gfx::Renderer &r, Gfx::PixmapTexture &dpadR, Gfx::GTexC texHeight);
	void draw(Gfx::Renderer &r) const;
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on);
	int getInput(IG::WP c) const;
	IG::WindowRect bounds() const;
	void setPos(IG::Point2D<int> pos);
	void setSize(Gfx::Renderer &r, uint sizeInPixels);
	void setDeadzone(Gfx::Renderer &r, int newDeadzone);
	void setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity);
	uint state() const { return state_; }
	void setState(uint state) { state_ = state; }

protected:
	Gfx::Sprite spr{};
	uint state_ = 1;
	Gfx::GCRect padBase{};
	IG::WindowRect padBaseArea{}, padArea{};
	int deadzone = 0;
	float diagonalSensitivity = 1.;
	Gfx::PixmapTexture mapImg{};
	Gfx::Sprite mapSpr{};
	bool visualizeBounds = 0;
	int btnSizePixels = 0;

	void updateBoundingAreaGfx(Gfx::Renderer &r);
};

class VControllerKeyboard
{
public:
	constexpr VControllerKeyboard() {}
	void updateImg(Gfx::Renderer &r);
	void setImg(Gfx::Renderer &r, Gfx::PixmapTexture *img);
	void place(Gfx::GC btnSize, Gfx::GC yOffset);
	void draw(Gfx::Renderer &r) const;
	int getInput(IG::WP c) const;
	int mode() const { return mode_; }
	void setMode(Gfx::Renderer &r, int mode);

protected:
	Gfx::Sprite spr{};
	IG::WindowRect bound{};
	uint keyXSize = 0, keyYSize = 0;
	static const uint cols = 10;
	uint mode_ = 0;
	Gfx::GTexC texXEnd = 0;
};

class VControllerGamepad
{
public:
	static constexpr uint MAX_CENTER_BTNS = 2;
	static constexpr uint MAX_FACE_BTNS = 8;

	constexpr VControllerGamepad(uint faceButtons): activeFaceBtns{faceButtons} {}
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on);
	bool boundingAreaVisible() const;
	void setImg(Gfx::Renderer &r, Gfx::PixmapTexture &pics);
	uint rowsForButtons(uint activeButtons);
	void setBaseBtnSize(Gfx::Renderer &r, uint sizeInPixels);
	IG::WindowRect centerBtnBounds() const;
	void setCenterBtnPos(IG::Point2D<int> pos);
	IG::WindowRect lTriggerBounds() const;
	void setLTriggerPos(IG::Point2D<int> pos);
	IG::WindowRect rTriggerBounds() const;
	void setRTriggerPos(IG::Point2D<int> pos);
	void layoutBtnRows(uint a[], uint btns, uint rows, IG::Point2D<int> pos);
	IG::WindowRect faceBtnBounds() const;
	void setFaceBtnPos(IG::Point2D<int> pos);
	std::array<int, 2> getCenterBtnInput(IG::WP pos) const;
	std::array<int, 2> getBtnInput(IG::WP pos) const;
	void draw(Gfx::Renderer &r, bool showHidden) const;
	VControllerDPad &dPad() { return dp; }
	const VControllerDPad &dPad() const { return dp; }
	Gfx::GC spacing() const { return btnSpace; }
	int spacingPixels() const { return btnSpacePixels; }
	void setSpacing(Gfx::GC mm) { btnSpace = mm; }
	void setSpacingPixels(int mm) { btnSpacePixels = mm; }
	void setRowShift(Gfx::GC mm) { btnRowShift = mm; }
	void setRowShiftPixels(int mm) { btnRowShiftPixels = mm; }
	void setStagger(Gfx::GC mm) { btnStagger = mm; }
	void setStaggerPixels(int mm) { btnStaggerPixels = mm; }
	void setExtraXSize(Gfx::GC mm) { btnExtraXSize = mm; }
	void setExtraYSize(Gfx::GC mm) { btnExtraYSize = mm; }
	void setExtraYSizeMultiRow(Gfx::GC mm) { btnExtraYSizeMultiRow = mm; }
	void setFaceButtonsState(uint state) { faceBtnsState = state; }
	void setCenterButtonsState(uint state) { centerBtnsState = state; }
	void setLTriggerState(uint state) { lTriggerState_ = state; }
	void setRTriggerState(uint state) { rTriggerState_ = state; }
	uint faceButtonsState() const { return faceBtnsState; }
	uint centerButtonsState() const { return centerBtnsState; }
	uint lTriggerState() const { return lTriggerState_; }
	uint rTriggerState() const { return rTriggerState_; }
	void setTriggersInline(bool on) { triggersInline_ = on; }
	void setActiveFaceButtons(uint num) { activeFaceBtns = num; }
	bool triggersInline() const { return triggersInline_; }

private:
	VControllerDPad dp{};
	Gfx::GC btnExtraXSize = 0.001, btnExtraYSize = 0.001, btnExtraYSizeMultiRow = 0.001;
	int btnSpacePixels = 0, btnStaggerPixels = 0, btnRowShiftPixels = 0;
	Gfx::GC btnSpace = 0, btnStagger = 0, btnRowShift = 0;//, btnAreaXOffset = 0;
	uint faceBtnsState = 1;
	uint centerBtnsState = 1;
	uint lTriggerState_ = 1;
	uint rTriggerState_ = 1;
	IG::WindowRect centerBtnBound[MAX_CENTER_BTNS]{};
	IG::WindowRect centerBtnsBound{};
	IG::WindowRect faceBtnBound[MAX_FACE_BTNS]{};
	IG::WindowRect faceBtnsBound{}, lTriggerBound{}, rTriggerBound{};
	Gfx::Sprite centerBtnSpr[MAX_CENTER_BTNS]{};
	Gfx::Sprite circleBtnSpr[MAX_FACE_BTNS]{};
	Gfx::GC btnSize = 0;
	int btnSizePixels = 0;
	uint activeFaceBtns = 0;
	bool triggersInline_ = false;
	bool showBoundingArea = false;
};

class VController
{
public:
	static constexpr int C_ELEM = 0, F_ELEM = 8, D_ELEM = 32;
	static constexpr uint TURBO_BIT = IG::bit(31), ACTION_MASK = 0x7FFFFFFF;
	using Map = std::array<uint, D_ELEM+9>;
	using KbMap = std::array<uint, 40>;

	constexpr VController(Gfx::Renderer &r, uint faceButtons): renderer_{r}
		#ifdef CONFIG_VCONTROLS_GAMEPAD
			,gp{faceButtons}
		#endif
		{}
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
	void setMenuImage(Gfx::PixmapTexture &img);
	void setFastForwardImage(Gfx::PixmapTexture &img);
	void setKeyboardImage(Gfx::PixmapTexture &img);
	bool menuHitTest(IG::WP pos);
	bool fastForwardHitTest(IG::WP pos);
	void setUsesScaledCoordinates(bool on);
	bool usesScaledCoordinates();
	void setAlpha(float val);
	VControllerGamepad &gamePad();
	Gfx::Renderer &renderer();

private:
	Gfx::Renderer &renderer_;

	#ifdef CONFIG_VCONTROLS_GAMEPAD
	VControllerGamepad gp;
	#endif
	VControllerKeyboard kb{};

	#ifdef CONFIG_BASE_ANDROID
	bool useScaledCoordinates = true;
	#else
	static constexpr bool useScaledCoordinates = false;
	#endif

	float alpha = 0;

	// menu button
	Gfx::Sprite menuBtnSpr{};
	IG::WindowRect menuBound{};
	uint menuBtnState = 1;

	// fast-forward button
	Gfx::Sprite ffBtnSpr{};
	IG::WindowRect ffBound{};
	uint ffBtnState = 1;

	std::array<std::array<int, 2>, Config::Input::MAX_POINTERS> ptrElem{};
	Map map{};
	uint kbMode = 0;
	KbMap kbMap{};

	std::array<int, 2> findElementUnderPos(Input::Event e);
};

using SysVController = VController;
void updateVControllerMapping(uint player, SysVController::Map &map);
void updateVControllerKeyboardMapping(uint mode, SysVController::KbMap &map);
