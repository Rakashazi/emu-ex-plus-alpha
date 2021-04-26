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
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/PixmapTexture.hh>

class VController;
class EmuApp;
struct WindowData;

namespace Base
{
class Window;
class ApplicationContext;
}

namespace Gfx
{
class GlyphTextureSet;
}

struct VControllerLayoutPosition
{
	enum { OFF = 0, SHOWN = 1, HIDDEN = 2 };
	_2DOrigin origin{LT2DO};
	unsigned state = OFF;
	IG::Point2D<int> pos{};

	constexpr VControllerLayoutPosition() {}
	constexpr VControllerLayoutPosition(_2DOrigin origin, IG::Point2D<int> pos): origin(origin), pos(pos) {}
	constexpr VControllerLayoutPosition(_2DOrigin origin, IG::Point2D<int> pos, unsigned state): origin(origin), state(state), pos(pos) {}
};

class VControllerDPad
{
public:
	constexpr VControllerDPad() {}
	void setImg(Gfx::Renderer &r, Gfx::Texture &dpadR, Gfx::GTexC texHeight);
	void draw(Gfx::RendererCommands &cmds) const;
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on, const WindowData &winData);
	int getInput(IG::WP c) const;
	IG::WindowRect bounds() const;
	void setPos(IG::Point2D<int> pos, const WindowData &winData);
	void setSize(Gfx::Renderer &r, unsigned sizeInPixels, const WindowData &winData);
	void setDeadzone(Gfx::Renderer &r, int newDeadzone, const WindowData &winData);
	void setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity, const WindowData &winData);
	unsigned state() const { return state_; }
	void setState(unsigned state) { state_ = state; }

protected:
	Gfx::Sprite spr{};
	unsigned state_ = 1;
	Gfx::GCRect padBase{};
	IG::WindowRect padBaseArea{}, padArea{};
	int deadzone = 0;
	float diagonalSensitivity = 1.;
	Gfx::PixmapTexture mapImg{};
	Gfx::Sprite mapSpr{};
	bool visualizeBounds = 0;
	int btnSizePixels = 0;

	void updateBoundingAreaGfx(Gfx::Renderer &r, const WindowData &winData);
};

class VControllerKeyboard
{
public:
	static constexpr unsigned VKEY_COLS = 20;
	static constexpr unsigned KEY_ROWS = 4;
	static constexpr unsigned KEY_COLS = VKEY_COLS/2;
	using KeyTable = std::array<std::array<unsigned, VKEY_COLS>, KEY_ROWS>;
	using KbMap = std::array<unsigned, KEY_ROWS * KEY_COLS>;

	constexpr VControllerKeyboard() {}
	void updateImg(Gfx::Renderer &r);
	void setImg(Gfx::Renderer &r, Gfx::TextureSpan img);
	void place(Gfx::GC btnSize, Gfx::GC yOffset, const WindowData &winData);
	void draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP) const;
	int getInput(IG::WP c) const;
	int translateInput(unsigned idx) const;
	bool keyInput(VController &v, Gfx::Renderer &r, Input::Event e);
	void selectKey(unsigned x, unsigned y);
	void selectKeyRel(int x, int y);
	void unselectKey();
	void extendKeySelection();
	unsigned currentKey() const;
	int mode() const { return mode_; }
	void setMode(Gfx::Renderer &r, int mode);
	void applyMap(KbMap map);
	void updateKeyboardMapping();

protected:
	Gfx::Sprite spr{};
	IG::WindowRect bound{};
	unsigned keyXSize = 0, keyYSize = 0;
	unsigned mode_ = 0;
	IG::WindowRect selected{{-1, -1}, {-1, -1}};
	Gfx::GTexC texXEnd = 0;
	KeyTable table{};
};

class VControllerGamepad
{
public:
	static constexpr unsigned MAX_CENTER_BTNS = 2;
	static constexpr unsigned MAX_FACE_BTNS = 8;

	constexpr VControllerGamepad(unsigned faceButtons): activeFaceBtns{faceButtons} {}
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on, const WindowData &winData);
	bool boundingAreaVisible() const;
	void setImg(Gfx::Renderer &r, Gfx::Texture &pics);
	unsigned rowsForButtons(unsigned activeButtons);
	void setBaseBtnSize(Gfx::Renderer &r, unsigned sizeInPixels, const WindowData &winData);
	IG::WindowRect centerBtnBounds() const;
	void setCenterBtnPos(IG::Point2D<int> pos, const WindowData &winData);
	IG::WindowRect lTriggerBounds() const;
	void setLTriggerPos(IG::Point2D<int> pos, const WindowData &winData);
	IG::WindowRect rTriggerBounds() const;
	void setRTriggerPos(IG::Point2D<int> pos, const WindowData &winData);
	void layoutBtnRows(unsigned a[], unsigned btns, unsigned rows, IG::Point2D<int> pos, const WindowData &winData);
	IG::WindowRect faceBtnBounds() const;
	void setFaceBtnPos(IG::Point2D<int> pos, const WindowData &winData);
	std::array<int, 2> getCenterBtnInput(IG::WP pos) const;
	std::array<int, 2> getBtnInput(IG::WP pos) const;
	void draw(Gfx::RendererCommands &cmds, bool showHidden, const WindowData &winData) const;
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
	void setFaceButtonsState(unsigned state) { faceBtnsState = state; }
	void setCenterButtonsState(unsigned state) { centerBtnsState = state; }
	void setLTriggerState(unsigned state) { lTriggerState_ = state; }
	void setRTriggerState(unsigned state) { rTriggerState_ = state; }
	unsigned faceButtonsState() const { return faceBtnsState; }
	unsigned centerButtonsState() const { return centerBtnsState; }
	unsigned lTriggerState() const { return lTriggerState_; }
	unsigned rTriggerState() const { return rTriggerState_; }
	void setTriggersInline(bool on) { triggersInline_ = on; }
	void setActiveFaceButtons(unsigned num) { activeFaceBtns = num; }
	bool triggersInline() const { return triggersInline_; }

private:
	VControllerDPad dp{};
	Gfx::GC btnExtraXSize = 0.001, btnExtraYSize = 0.001, btnExtraYSizeMultiRow = 0.001;
	int btnSpacePixels = 0, btnStaggerPixels = 0, btnRowShiftPixels = 0;
	Gfx::GC btnSpace = 0, btnStagger = 0, btnRowShift = 0;//, btnAreaXOffset = 0;
	unsigned faceBtnsState = 1;
	unsigned centerBtnsState = 1;
	unsigned lTriggerState_ = 1;
	unsigned rTriggerState_ = 1;
	IG::WindowRect centerBtnBound[MAX_CENTER_BTNS]{};
	IG::WindowRect centerBtnsBound{};
	IG::WindowRect faceBtnBound[MAX_FACE_BTNS]{};
	IG::WindowRect faceBtnsBound{}, lTriggerBound{}, rTriggerBound{};
	Gfx::Sprite centerBtnSpr[MAX_CENTER_BTNS]{};
	Gfx::Sprite circleBtnSpr[MAX_FACE_BTNS]{};
	Gfx::GC btnSize = 0;
	int btnSizePixels = 0;
	unsigned activeFaceBtns = 0;
	bool triggersInline_ = false;
	bool showBoundingArea = false;
};

class VController : public EmuAppHelper<VController>
{
public:
	static constexpr int C_ELEM = 0, F_ELEM = 8, D_ELEM = 32;
	static constexpr unsigned TURBO_BIT = IG::bit(31), ACTION_MASK = 0x7FFFFFFF;
	using Map = std::array<unsigned, D_ELEM+9>;
	using KbMap = VControllerKeyboard::KbMap;
	using VControllerLayoutPositionArr = std::array<std::array<VControllerLayoutPosition, 7>, 2>;

	constexpr VController(unsigned faceButtons)
		#ifdef CONFIG_VCONTROLS_GAMEPAD
			:gp{faceButtons}
		#endif
		{}
	Gfx::GC xMMSize(Gfx::GC mm) const;
	Gfx::GC yMMSize(Gfx::GC mm) const;
	int xMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const;
	int yMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const;
	void setInputPlayer(uint8_t player);
	uint8_t inputPlayer() const;
	void updateMapping();
	void updateKeyboardMapping();
	bool hasTriggers() const;
	void setImg(Gfx::Texture &pics);
	void setBoundingAreaVisible(bool on);
	bool boundingAreaVisible() const;
	void setMenuBtnPos(IG::Point2D<int> pos);
	void setFFBtnPos(IG::Point2D<int> pos);
	void inputAction(Input::Action action, unsigned vBtn);
	void resetInput(bool init = 0);
	void init(float alpha, unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP);
	void place();
	void toggleKeyboard();
	void applyInput(Input::Event e);
	bool keyInput(Input::Event e);
	void draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden = false);
	void draw(Gfx::RendererCommands &cmds, bool emuSystemControls, bool activeFF, bool showHidden, float alpha);
	int numElements() const;
	IG::WindowRect bounds(int elemIdx) const;
	void setPos(int elemIdx, IG::Point2D<int> pos);
	void setState(int elemIdx, unsigned state);
	unsigned state(int elemIdx) const;
	void setBaseBtnSize(unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP);
	bool isInKeyboardMode() const;
	void setMenuImage(Gfx::TextureSpan img);
	void setFastForwardImage(Gfx::TextureSpan img);
	void setKeyboardImage(Gfx::TextureSpan img);
	bool menuHitTest(IG::WP pos);
	bool fastForwardHitTest(IG::WP pos);
	void resetUsesScaledCoordinates();
	void setUsesScaledCoordinates(std::optional<bool>);
	bool usesScaledCoordinates() const;
	std::optional<bool> usesScaledCoordinatesOption() const;
	void setAlpha(float val);
	VControllerGamepad &gamePad();
	void setRenderer(Gfx::Renderer &renderer);
	Gfx::Renderer &renderer();
	void setWindow(const Base::Window &win);
	bool hasWindow() const { return win; }
	const Base::Window &window() const { return *win; }
	const WindowData &windowData() const { return *winData; }
	VControllerLayoutPositionArr &layoutPosition() { return layoutPos; };
	bool layoutPositionChanged() const { return layoutPosChanged; };
	void setLayoutPositionChanged(bool changed = true) { layoutPosChanged = changed; }
	Base::ApplicationContext appContext() const;
	const Gfx::GlyphTextureSet &face() const;
	void setFace(const Gfx::GlyphTextureSet &face);

private:
	Gfx::Renderer *renderer_{};
	const Base::Window *win{};
	const WindowData *winData{};
	const Gfx::GlyphTextureSet *facePtr{};

	#ifdef CONFIG_VCONTROLS_GAMEPAD
	VControllerGamepad gp;
	#endif
	VControllerKeyboard kb{};
	float alpha{};
	static constexpr bool useScaledCoordinatesDefault = true;
	static constexpr bool useScaledCoordinatesIsMutable = Config::EmuFramework::USE_SCALED_COORDINATES;
	IG_enableMemberIf(useScaledCoordinatesIsMutable, bool, useScaledCoordinates){useScaledCoordinatesDefault};
	uint8_t inputPlayer_{};
	bool layoutPosChanged{};
	VControllerLayoutPositionArr layoutPos{};

	// menu button
	Gfx::Sprite menuBtnSpr{};
	IG::WindowRect menuBound{};
	unsigned menuBtnState = 1;

	// fast-forward button
	Gfx::Sprite ffBtnSpr{};
	IG::WindowRect ffBound{};
	unsigned ffBtnState = 1;

	std::array<std::array<int, 2>, Config::Input::MAX_POINTERS> ptrElem{};
	Map map{};
	unsigned kbMode = 0;

	std::array<int, 2> findElementUnderPos(Input::Event e);
};

static constexpr unsigned VCTRL_LAYOUT_DPAD_IDX = 0,
	VCTRL_LAYOUT_CENTER_BTN_IDX = 1,
	VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX = 2,
	VCTRL_LAYOUT_MENU_IDX = 3,
	VCTRL_LAYOUT_FF_IDX = 4,
	VCTRL_LAYOUT_L_IDX = 5,
	VCTRL_LAYOUT_R_IDX = 6;

using SysVController = VController;
void updateVControllerMapping(unsigned player, SysVController::Map &map);
SysVController::KbMap updateVControllerKeyboardMapping(unsigned mode);
