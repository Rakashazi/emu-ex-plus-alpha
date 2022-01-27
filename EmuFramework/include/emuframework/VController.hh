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
#include <emuframework/config.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/input/Input.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/PixmapTexture.hh>
#include <vector>
#include <span>
#include <optional>

namespace IG
{
class Window;
class ApplicationContext;
class IO;
}

namespace IG::Gfx
{
class GlyphTextureSet;
class Viewport;
}

namespace EmuEx
{

using namespace IG;
class VController;
class EmuApp;
class EmuViewController;
struct WindowData;

enum class VControllerState : uint8_t
{
	OFF, SHOWN, HIDDEN
};

struct VControllerLayoutPosition
{
	IG::WP pos{};
	_2DOrigin origin{LT2DO};
	VControllerState state{};

	constexpr VControllerLayoutPosition() {}
	constexpr VControllerLayoutPosition(_2DOrigin origin, IG::WP pos, VControllerState state = {}):
		pos{pos}, origin{origin}, state{state} {}
};

class VControllerDPad
{
public:
	constexpr VControllerDPad() {}
	void setImg(Gfx::Renderer &r, Gfx::Texture &dpadR, float texHeight);
	void draw(Gfx::RendererCommands &cmds) const;
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on, Gfx::ProjectionPlane);
	int getInput(IG::WP c) const;
	IG::WindowRect bounds() const;
	void setPos(IG::WP pos, Gfx::ProjectionPlane);
	void setSize(Gfx::Renderer &r, unsigned sizeInPixels, Gfx::ProjectionPlane);
	void setDeadzone(Gfx::Renderer &r, int newDeadzone, Gfx::ProjectionPlane);
	void setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity, Gfx::ProjectionPlane);
	constexpr VControllerState state() const { return state_; }
	constexpr void setState(VControllerState state) { state_ = state; }

protected:
	Gfx::Sprite spr{};
	Gfx::Sprite mapSpr{};
	Gfx::PixmapTexture mapImg{};
	Gfx::GCRect padBase{};
	IG::WindowRect padBaseArea{}, padArea{};
	int deadzone{};
	float diagonalSensitivity = 1.;
	int btnSizePixels{};
	VControllerState state_ = VControllerState::SHOWN;
	bool visualizeBounds{};

	void updateBoundingAreaGfx(Gfx::Renderer &, Gfx::ProjectionPlane);
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
	void place(float btnSize, float yOffset, Gfx::ProjectionPlane);
	void draw(Gfx::RendererCommands &cmds, Gfx::ProjectionPlane) const;
	int getInput(IG::WP c) const;
	int translateInput(unsigned idx) const;
	bool keyInput(VController &v, Gfx::Renderer &r, Input::Event e);
	[[nodiscard]] IG::WindowRect selectKey(unsigned x, unsigned y);
	void selectKeyRel(int x, int y);
	void unselectKey();
	[[nodiscard]] IG::WindowRect extendKeySelection(IG::WindowRect);
	unsigned currentKey() const;
	unsigned currentKey(int x, int y) const;
	int mode() const { return mode_; }
	void setMode(Gfx::Renderer &r, int mode);
	void applyMap(KbMap map);
	void updateKeyboardMapping();
	void setShiftActive(bool);
	bool toggleShiftActive();
	bool shiftIsActive() const;

protected:
	Gfx::Sprite spr{};
	IG::WindowRect bound{};
	unsigned keyXSize{}, keyYSize{};
	unsigned mode_{};
	IG::WindowRect selected{{-1, -1}, {-1, -1}};
	IG::WindowRect shiftRect{{-1, -1}, {-1, -1}};
	float texXEnd{};
	KeyTable table{};
};

class VControllerButton
{
public:
	constexpr VControllerButton() {}
	void setPos(IG::WP pos, Gfx::ProjectionPlane, _2DOrigin = C2DO);
	void setSize(IG::WP size, IG::WP extendedSize = {});
	void setImage(Gfx::TextureSpan img, float aspectRatio = 1.f);
	void setState(VControllerState state);
	void setShowBounds(bool);
	void setShouldSkipLayout(bool);
	void setEnabled(bool);
	constexpr bool showBounds() const {return showBoundingArea; }
	constexpr IG::WindowRect bounds() const { return bounds_; }
	constexpr IG::WindowRect realBounds() const { return extendedBounds_; }
	constexpr VControllerState state() const { return state_; }
	constexpr bool shouldSkipLayout() const { return skipLayout; }
	constexpr const Gfx::Sprite &sprite() const { return spr; }
	constexpr bool isEnabled() const { return !disabled; }
	void draw(Gfx::RendererCommands &cmds, std::optional<Gfx::Color>, bool showHidden) const;

protected:
	Gfx::Sprite spr{};
	IG::WindowRect bounds_{};
	IG::WindowRect extendedBounds_{};
	float aspectRatio{};
	VControllerState state_ = VControllerState::SHOWN;
	bool skipLayout{};
	bool disabled{};
	bool showBoundingArea{};
};

class VControllerButtonGroup
{
public:
	VControllerButtonGroup(int size);
	std::vector<VControllerButton> &buttons();
	const std::vector<VControllerButton> &buttons() const;
	void setPos(IG::WP pos, Gfx::ProjectionPlane);
	void setState(VControllerState state);
	void setButtonSize(IG::WP size, IG::WP extendedSize = {});
	void setStaggerType(uint8_t);
	void setSpacing(int16_t);
	void setShowBounds(bool);
	constexpr IG::WindowRect bounds() const { return bounds_; }
	constexpr VControllerState state() const { return state_; }
	int rows() const;
	int buttonsToLayout() const;
	int buttonsPerRow() const;
	std::array<int, 2> findButtonIndices(IG::WP windowPos) const;
	void draw(Gfx::RendererCommands &cmds, Gfx::ProjectionPlane projP, bool showHidden) const;

protected:
	std::vector<VControllerButton> btns{};
	IG::WindowRect bounds_{};
	IG::WP btnSize{};
	int16_t btnStagger{};
	int16_t btnRowShift{};
	uint16_t btnSpace{};
	VControllerState state_ = VControllerState::SHOWN;
	uint8_t btnStaggerType = 2;
	bool showBoundingArea{};
};

class VControllerGamepad
{
public:
	VControllerGamepad(int faceButtons, int centerButtons);
	VControllerDPad &dPad() { return dp; }
	const VControllerDPad &dPad() const { return dp; }
	VControllerButtonGroup &centerButtons() { return centerBtns; }
	const VControllerButtonGroup &centerButtons() const { return centerBtns; }
	VControllerButtonGroup &faceButtons() { return faceBtns; }
	const VControllerButtonGroup &faceButtons() const { return faceBtns; }
	VControllerButton &lTrigger() const { return *lTriggerPtr; }
	VControllerButton &rTrigger() const { return *rTriggerPtr; }
	void setFaceButtonSize(Gfx::Renderer &, IG::WP sizeInPixels, IG::WP extraSizePixels, Gfx::ProjectionPlane);
	void setBoundingAreaVisible(Gfx::Renderer &r, bool on, Gfx::ProjectionPlane);
	void setImg(Gfx::Renderer &r, Gfx::Texture &pics);
	void drawDPads(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane) const;
	void drawButtons(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane) const;
	void draw(Gfx::RendererCommands &cmds, bool showHidden, Gfx::ProjectionPlane) const;
	void setSpacingPixels(int);
	void setStaggerType(int);
	void setTriggersInline(bool on);
	bool triggersInline() const;
	int faceButtonRows() const;

private:
	VControllerDPad dp{};
	VControllerButtonGroup centerBtns;
	VControllerButtonGroup faceBtns;
	VControllerButton *lTriggerPtr{};
	VControllerButton *rTriggerPtr{};
};

enum class VControllerVisibility : uint8_t
{
	OFF, ON, AUTO
};

class VController : public EmuAppHelper<VController>
{
public:
	static constexpr int C_ELEM = 0, F_ELEM = 8, D_ELEM = 32;
	static constexpr unsigned TURBO_BIT = IG::bit(31), ACTION_MASK = 0x7FFFFFFF;
	static constexpr unsigned TOGGLE_KEYBOARD = 65536;
	static constexpr unsigned CHANGE_KEYBOARD_MODE = 65537;
	using Map = std::array<unsigned, D_ELEM+9>;
	using KbMap = VControllerKeyboard::KbMap;
	using VControllerLayoutPositionArr = std::array<std::array<VControllerLayoutPosition, 7>, 2>;

	VController(IG::ApplicationContext, int faceButtons, int centerButtons);
	float xMMSize(float mm) const;
	float yMMSize(float mm) const;
	int xMMSizeToPixel(const IG::Window &win, float mm) const;
	int yMMSizeToPixel(const IG::Window &win, float mm) const;
	void setInputPlayer(uint8_t player);
	uint8_t inputPlayer() const;
	void updateMapping();
	void updateKeyboardMapping();
	bool hasTriggers() const;
	void setImg(Gfx::Texture &pics);
	void setMenuBtnPos(IG::WP pos);
	void setFFBtnPos(IG::WP pos);
	void inputAction(Input::Action action, unsigned vBtn);
	void resetInput();
	void place();
	void toggleKeyboard();
	bool pointerInputEvent(Input::Event e, IG::WindowRect gameRect);
	bool keyInput(Input::Event e);
	void draw(Gfx::RendererCommands &cmds, bool activeFF, bool showHidden = false);
	void draw(Gfx::RendererCommands &cmds, bool activeFF, bool showHidden, float alpha);
	int numElements() const;
	IG::WindowRect bounds(int elemIdx) const;
	void setPos(int elemIdx, IG::WP pos);
	void setState(int elemIdx, VControllerState state);
	VControllerState state(int elemIdx) const;
	void setButtonSize(unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, Gfx::ProjectionPlane projP);
	bool isInKeyboardMode() const;
	void setMenuImage(Gfx::TextureSpan img);
	void setFastForwardImage(Gfx::TextureSpan img);
	void setKeyboardImage(Gfx::TextureSpan img);
	bool menuHitTest(IG::WP pos);
	bool fastForwardHitTest(IG::WP pos);
	void setButtonAlpha(std::optional<uint8_t>);
	constexpr uint8_t buttonAlpha() const { return alpha; }
	VControllerGamepad &gamePad();
	VControllerKeyboard &keyboard() { return kb; }
	void setRenderer(Gfx::Renderer &renderer);
	Gfx::Renderer &renderer();
	void setWindow(const IG::Window &win);
	bool hasWindow() const { return win; }
	const IG::Window &window() const { return *win; }
	const WindowData &windowData() const { return *winData; }
	VControllerLayoutPositionArr &layoutPosition() { return layoutPos; };
	const VControllerLayoutPositionArr &layoutPosition() const { return layoutPos; };
	bool layoutPositionChanged() const { return layoutPosChanged; };
	void setLayoutPositionChanged(bool changed = true) { layoutPosChanged = changed; }
	IG::ApplicationContext appContext() const;
	const Gfx::GlyphTextureSet &face() const;
	void setFace(const Gfx::GlyphTextureSet &face);
	bool setButtonSize(std::optional<uint16_t> mm100xOpt, bool placeElements = true);
	uint16_t buttonSize() const;
	float buttonGCSize() const;
	int buttonPixelSize(const IG::Window &) const;
	bool setButtonXPadding(std::optional<uint16_t> opt, bool placeElements = true);
	uint16_t buttonXPadding() const;
	bool setButtonYPadding(std::optional<uint16_t> opt, bool placeElements = true);
	uint16_t buttonYPadding() const;
	bool setButtonSpacing(std::optional<uint16_t> mm100xOpt, bool placeElements = true);
	void setDefaultButtonSpacing(uint16_t mm100x);
	uint16_t buttonSpacing() const;
	bool setButtonStagger(std::optional<uint16_t> mm100xOpt, bool placeElements = true);
	void setDefaultButtonStagger(uint16_t mm100x);
	uint16_t buttonStagger() const;
	bool setDpadDeadzone(std::optional<uint16_t> mm100xOpt);
	uint16_t dpadDeadzone() const;
	bool setDpadDiagonalSensitivity(std::optional<uint16_t> opt);
	uint16_t dpadDiagonalSensitivity() const;
	void setTriggersInline(std::optional<bool> opt, bool placeElements = true);
	bool triggersInline() const;
	void setBoundingAreaVisible(std::optional<bool> opt, bool placeElements = true);
	bool boundingAreaVisible() const;
	void setShowOnTouchInput(std::optional<bool> opt);
	bool showOnTouchInput() const;
	void setVibrateOnTouchInput(EmuApp &, std::optional<bool> opt);
	bool vibrateOnTouchInput() const;
	bool shouldShowOnTouchInput() const;
	void setGamepadControlsVisibility(std::optional<VControllerVisibility>);
	VControllerVisibility gamepadControlsVisibility() const;
	void setGamepadControlsVisible(bool);
	bool gamepadControlsVisible() const;
	static bool visibilityIsValid(VControllerVisibility);
	void setPhysicalControlsPresent(bool);
	bool updateAutoOnScreenControlVisible();
	bool readConfig(IO &, unsigned key, unsigned size);
	void writeConfig(IO &) const;
	void readSerializedLayoutPositions(IO &, unsigned size);
	unsigned serializedLayoutPositionsSize() const;
	void configure(IG::Window &, Gfx::Renderer &, const Gfx::GlyphTextureSet &face);
	static VControllerLayoutPosition pixelToLayoutPos(IG::WP pos, IG::WP size, IG::WindowRect viewBounds);
	static IG::WP layoutToPixelPos(VControllerLayoutPosition, Gfx::Viewport);
	void resetPositions();
	void resetOptions();
	void resetAllOptions();
	static bool shouldDraw(VControllerState state, bool showHidden = false);
	void setGamepadIsEnabled(bool on) { gamepadDisabledFlags = on ? 0 : GAMEPAD_BITS; }
	void setGamepadDPadIsEnabled(bool on) { gamepadDisabledFlags = IG::setOrClearBits(gamepadDisabledFlags, GAMEPAD_DPAD_BIT, !on); }
	void setGamepadButtonsAreEnabled(bool on) { gamepadDisabledFlags = IG::setOrClearBits(gamepadDisabledFlags, GAMEPAD_BUTTONS_BIT, !on); }
	bool gamepadIsEnabled() const { return gamepadDisabledFlags != GAMEPAD_BITS; }
	bool gamepadDPadIsEnabled() const { return !(gamepadDisabledFlags & GAMEPAD_DPAD_BIT); }
	bool gamepadButtonsAreEnabled() const { return !(gamepadDisabledFlags & GAMEPAD_BUTTONS_BIT); }
	bool gamepadIsActive() const;

private:
	static constexpr uint8_t GAMEPAD_DPAD_BIT = IG::bit(0);
	static constexpr uint8_t GAMEPAD_BUTTONS_BIT = IG::bit(1);
	static constexpr uint8_t GAMEPAD_BITS = GAMEPAD_DPAD_BIT | GAMEPAD_BUTTONS_BIT;

	Gfx::Renderer *renderer_{};
	const IG::Window *win{};
	const WindowData *winData{};
	const Gfx::GlyphTextureSet *facePtr{};
	VControllerGamepad gp;
	VControllerKeyboard kb{};
	float alphaF{};
	VControllerLayoutPositionArr layoutPos{};
	VControllerButton menuBtn{};
	VControllerButton ffBtn{};
	Input::DragTracker<std::array<int, 2>> dragTracker{};
	Map map{};
	uint16_t defaultButtonSize{};
	uint16_t btnSize{};
	uint16_t buttonXPadding_{};
	uint16_t buttonYPadding_{};
	uint16_t dpadDzone{};
	uint16_t dpadDiagonalSensitivity_{};
	uint16_t defaultButtonSpacing_{200};
	uint16_t buttonSpacing_{defaultButtonSpacing_};
	uint16_t defaultButtonStagger_{1};
	uint16_t buttonStagger_{defaultButtonStagger_};
	bool triggersInline_{};
	bool boundingAreaVisible_{};
	bool showOnTouchInput_ = true;
	static constexpr auto DEFAULT_GAMEPAD_CONTROLS_VISIBILITY{Config::envIsLinux ? VControllerVisibility::OFF : VControllerVisibility::AUTO};
	VControllerVisibility gamepadControlsVisibility_{DEFAULT_GAMEPAD_CONTROLS_VISIBILITY};
	uint8_t inputPlayer_{};
	bool layoutPosChanged{};
	bool physicalControlsPresent{};
	bool gamepadIsVisible{gamepadControlsVisibility_ != VControllerVisibility::OFF};
	uint8_t gamepadDisabledFlags{};
	bool kbMode{};
	uint8_t alpha{};
	IG_UseMemberIf(Config::BASE_SUPPORTS_VIBRATOR, bool, vibrateOnTouchInput_){};

	std::array<int, 2> findGamepadElements(IG::WP pos);
	int keyboardKeyFromPointer(Input::Event);
	void applyButtonSize();
};

static constexpr unsigned VCTRL_LAYOUT_DPAD_IDX = 0,
	VCTRL_LAYOUT_CENTER_BTN_IDX = 1,
	VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX = 2,
	VCTRL_LAYOUT_MENU_IDX = 3,
	VCTRL_LAYOUT_FF_IDX = 4,
	VCTRL_LAYOUT_L_IDX = 5,
	VCTRL_LAYOUT_R_IDX = 6;

void updateVControllerMapping(unsigned player, VController::Map &map);
VController::KbMap updateVControllerKeyboardMapping(unsigned mode);

}
