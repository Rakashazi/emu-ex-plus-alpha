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
#include <imagine/gfx/Texture.hh>
#include <imagine/util/variant.hh>
#include <vector>
#include <span>
#include <optional>

namespace IG
{
class Window;
class ApplicationContext;
class FileIO;
class MapIO;
}

namespace EmuEx
{

using namespace IG;
class VController;
class EmuApp;
class EmuViewController;
struct SystemInputDeviceDesc;
struct InputComponentDesc;
enum class InputComponent : uint8_t;
struct WindowData;

enum class VControllerState : uint8_t
{
	OFF, SHOWN, HIDDEN
};

struct VControllerLayoutPosition
{
	IG::WP pos{};
	_2DOrigin origin{LT2DO};
	VControllerState state{VControllerState::SHOWN};

	constexpr VControllerLayoutPosition() = default;
	constexpr VControllerLayoutPosition(_2DOrigin origin, IG::WP pos, VControllerState state = VControllerState::SHOWN):
		pos{pos}, origin{origin}, state{state} {}

	static VControllerLayoutPosition fromPixelPos(IG::WP pos, IG::WP size, IG::WindowRect viewBounds);
	IG::WP toPixelPos(IG::WindowRect viewBounds) const;
};

enum class VControllerImageIndex
{
	button1,
	button2,
	button3,
	button4,
	button5,
	button6,
	button7,
	button8,
	auxButton1,
	auxButton2,
};

constexpr int defaultDPadDeadzone = 135;
constexpr float defaultDPadDiagonalSensitivity = 1.75f;
constexpr uint16_t defaultButtonBoundsPadding = 200;

class VControllerDPad
{
public:
	constexpr VControllerDPad(std::span<const unsigned, 4> keys):
		keys{keys[0], keys[1], keys[2], keys[3]} {}
	void setImage(Gfx::TextureSpan);
	void draw(Gfx::RendererCommands &__restrict__, bool showHidden = false) const;
	void setShowBounds(Gfx::Renderer &r, bool on);
	bool showBounds() const { return visualizeBounds; }
	std::array<int, 2> getInput(IG::WP c) const;
	IG::WRect bounds() const { return padBaseArea; }
	IG::WRect realBounds() const { return padArea; }
	void setPos(IG::WP pos, IG::WindowRect viewBounds);
	void setSize(Gfx::Renderer &, int sizeInPixels);
	void setDeadzone(Gfx::Renderer &, int newDeadzone, const IG::Window &);
	auto deadzone() const { return deadzoneMM100x; }
	void setDiagonalSensitivity(Gfx::Renderer &, float newDiagonalSensitivity);
	auto diagonalSensitivity() const { return diagonalSensitivity_; }
	std::string name(const EmuApp &) const { return "D-Pad"; }
	void updateMeasurements(const IG::Window &win);
	void transposeKeysForPlayer(const EmuApp &, int player);

protected:
	Gfx::Sprite spr;
	Gfx::Sprite mapSpr;
	Gfx::Texture mapImg;
	IG::WindowRect padBaseArea, padArea;
	int deadzonePixels{};
	int deadzoneMM100x{defaultDPadDeadzone};
	float diagonalSensitivity_{defaultDPadDiagonalSensitivity};
	int btnSizePixels{};
	std::array<unsigned, 4> keys{};
	bool visualizeBounds{};

	void updateBoundingAreaGfx(Gfx::Renderer &);

public:
	VControllerState state{VControllerState::SHOWN};
};

enum class VControllerKbMode: uint8_t
{
	LAYOUT_1,
	LAYOUT_2
};

class VControllerKeyboard
{
public:
	static constexpr int VKEY_COLS = 20;
	static constexpr int KEY_ROWS = 4;
	static constexpr int KEY_COLS = VKEY_COLS/2;
	using KeyTable = std::array<std::array<unsigned, VKEY_COLS>, KEY_ROWS>;
	using KbMap = std::array<unsigned, KEY_ROWS * KEY_COLS>;

	constexpr VControllerKeyboard() = default;
	void updateImg(Gfx::Renderer &r);
	void setImg(Gfx::Renderer &r, Gfx::TextureSpan img);
	void place(int btnSize, int yOffset, WRect viewBounds);
	void draw(Gfx::RendererCommands &__restrict__) const;
	int getInput(IG::WP c) const;
	unsigned translateInput(int idx) const;
	bool keyInput(VController &v, Gfx::Renderer &r, const Input::KeyEvent &e);
	[[nodiscard]] IG::WindowRect selectKey(unsigned x, unsigned y);
	void selectKeyRel(int x, int y);
	void unselectKey();
	[[nodiscard]] IG::WindowRect extendKeySelection(IG::WindowRect);
	unsigned currentKey() const;
	unsigned currentKey(int x, int y) const;
	VControllerKbMode mode() const { return mode_; }
	void setMode(EmuSystem &, Gfx::Renderer &, VControllerKbMode mode);
	void cycleMode(EmuSystem &, Gfx::Renderer &);
	void applyMap(KbMap map);
	void updateKeyboardMapping(EmuSystem &);
	void setShiftActive(bool);
	bool toggleShiftActive();
	bool shiftIsActive() const;

protected:
	Gfx::Sprite spr;
	IG::WRect bound;
	int keyXSize{}, keyYSize{};
	IG::WRect selected{{-1, -1}, {-1, -1}};
	IG::WRect shiftRect{{-1, -1}, {-1, -1}};
	float texXEnd{};
	KeyTable table{};
	VControllerKbMode mode_{};
};

class VControllerButtonBase
{
public:
	constexpr VControllerButtonBase(unsigned key): key{key} {}
	void setPos(IG::WP pos, IG::WRect viewBounds, _2DOrigin = C2DO);
	void setSize(IG::WP size);
	void setImage(Gfx::TextureSpan, float aR = 1.f);
	IG::WindowRect bounds() const { return bounds_; }
	const Gfx::Sprite &sprite() const { return spr; }
	void draw(Gfx::RendererCommands &__restrict__, std::optional<Gfx::Color>) const;
	void draw(Gfx::RendererCommands &__restrict__ cmds) const { draw(cmds, {}); };
	std::string name(const EmuApp &) const;

protected:
	Gfx::Sprite spr;
	IG::WindowRect bounds_;
	float aspectRatio{};
public:
	unsigned key{};
	bool enabled = true;
	bool skipLayout{};
};

class VControllerUIButton : public VControllerButtonBase
{
public:
	using SelectFunc = DelegateFunc<void(VControllerUIButton &, const Input::MotionEvent &)>;

	Gfx::Color color;

	VControllerUIButton(unsigned key): VControllerButtonBase{key} {}
	void draw(Gfx::RendererCommands &__restrict__) const;
	WRect realBounds() const { return bounds(); }
	bool overlaps(WP windowPos) const { return enabled && realBounds().overlaps(windowPos); }
};

class VControllerButton : public VControllerButtonBase
{
public:
	constexpr VControllerButton(unsigned key): VControllerButtonBase{key} {}
	void setPos(IG::WP pos, IG::WindowRect viewBounds, _2DOrigin = C2DO);
	void setSize(IG::WP size, IG::WP extendedSize = {});
	void setShowBounds(bool on) { showBoundingArea = on; }
	bool showBounds() const {return showBoundingArea; }
	IG::WRect realBounds() const { return extendedBounds_; }
	void drawBounds(Gfx::RendererCommands &__restrict__) const;
	void draw(Gfx::RendererCommands &__restrict__) const;
	bool overlaps(WP windowPos) const { return enabled && realBounds().overlaps(windowPos); }

protected:
	bool showBoundingArea{};
	IG::WindowRect extendedBounds_{};
};

class VControllerButtonGroup
{
public:
	VControllerButtonGroup(std::span<const unsigned> buttonCodes, _2DOrigin layoutOrigin);
	void setPos(IG::WP pos, IG::WindowRect viewBounds);
	void setButtonSize(IG::WP size);
	void setStaggerType(uint8_t);
	auto stagger() const { return btnStaggerType; }
	void setSpacing(int16_t spacingMM100x, const Window &);
	auto spacing() const { return spacingMM100x; }
	void setXPadding(uint16_t p) { buttonXPadding_ = p; }
	auto xPadding() const { return buttonXPadding_; }
	void setYPadding(uint16_t p) { buttonYPadding_ = p; }
	auto yPadding() const { return buttonYPadding_; }
	IG::WP paddingPixels() const { return {int(btnSize.x * (xPadding() / 1000.f)), int(btnSize.y * (yPadding() / 1000.f))}; }
	bool showsBounds() const { return showBoundingArea; }
	void setShowBounds(bool on) { showBoundingArea = on; }
	auto bounds() const { return bounds_; }
	IG::WRect paddingRect() const { return {{int(-paddingPixels().x), int(-paddingPixels().y)}, {int(paddingPixels().x), int(paddingPixels().y)}}; }
	IG::WRect realBounds() const { return bounds() + paddingRect(); }
	int rows() const;
	std::array<int, 2> findButtonIndices(IG::WP windowPos) const;
	void draw(Gfx::RendererCommands &__restrict__, bool showHidden = false) const;
	std::string name(const EmuApp &) const;
	void updateMeasurements(const IG::Window &win);
	void transposeKeysForPlayer(const EmuApp &, int player);

	std::vector<VControllerButton> buttons;
protected:
	IG::WRect bounds_;
	IG::WP btnSize{};
	int spacingPixels{};
	int16_t btnStagger{};
	int16_t btnRowShift{};
	uint16_t spacingMM100x{200};
	uint16_t buttonXPadding_{defaultButtonBoundsPadding};
	uint16_t buttonYPadding_{defaultButtonBoundsPadding};
	uint8_t btnStaggerType{2};
	int8_t rowItems{};
	bool showBoundingArea{};

	void drawButtons(Gfx::RendererCommands &__restrict__) const;
public:
	VControllerState state{VControllerState::SHOWN};
	_2DOrigin layoutOrigin{};
};

class VControllerUIButtonGroup
{
public:
	VControllerUIButtonGroup(std::span<const unsigned> buttonCodes, _2DOrigin layoutOrigin);
	void setPos(IG::WP pos, IG::WindowRect viewBounds);
	void setButtonSize(IG::WP size);
	auto bounds() const { return bounds_; }
	IG::WRect realBounds() const { return bounds(); }
	int rows() const;
	void draw(Gfx::RendererCommands &__restrict__, bool showHidden = false) const;
	std::string name(const EmuApp &) const;

	std::vector<VControllerUIButton> buttons;
protected:
	IG::WRect bounds_{};
	IG::WP btnSize{};
public:
	int8_t rowItems{};
	VControllerState state{VControllerState::SHOWN};
	_2DOrigin layoutOrigin{};
};

using VControllerElementVariant = std::variant<VControllerButtonGroup, VControllerUIButtonGroup, VControllerDPad>;

class VControllerElement : public VControllerElementVariant
{
public:
	using VControllerElementVariant::VControllerElementVariant;

	std::array<VControllerLayoutPosition, 2> layoutPos;

	constexpr auto dPad() { return std::get_if<VControllerDPad>(this); }
	constexpr auto dPad() const { return std::get_if<VControllerDPad>(this); }
	constexpr auto buttonGroup() { return std::get_if<VControllerButtonGroup>(this); }
	constexpr auto buttonGroup() const { return std::get_if<VControllerButtonGroup>(this); }
	constexpr auto uiButtonGroup() { return std::get_if<VControllerUIButtonGroup>(this); }
	constexpr auto uiButtonGroup() const { return std::get_if<VControllerUIButtonGroup>(this); }
	IG::WRect bounds() const { return visit([](auto &e){ return e.bounds(); }, *this); }
	IG::WRect realBounds() const { return visit([](auto &e){ return e.realBounds(); }, *this); }

	void setPos(IG::WP pos, IG::WRect viewBounds)
	{
		visit([&](auto &e){ e.setPos(pos, viewBounds); }, *this);
	}

	void setState(VControllerState state) { visit([&](auto &e){ e.state = state; }, *this); }
	VControllerState state() const { return visit([](auto &e){ return e.state; }, *this); }
	void draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden = false) const { visit([&](auto &e){ e.draw(cmds, showHidden); }, *this); }

	void place(IG::WRect viewBounds, IG::WRect contentBounds, int layoutIdx)
	{
		auto &lPos = layoutPos[layoutIdx];
		setPos(lPos.toPixelPos(contentBounds), viewBounds);
		setState(lPos.state);
	}

	void setShowBounds(Gfx::Renderer &r, bool on)
	{
		visit(overloaded
		{
			[&](VControllerDPad &e){ e.setShowBounds(r, on); },
			[&](VControllerButtonGroup &e){ e.setShowBounds(on); },
			[](auto &e){}
		}, *this);
	}

	_2DOrigin layoutOrigin() const
	{
		return visit(overloaded
		{
			[&](const VControllerDPad &e){ return LB2DO; },
			[](auto &e){ return e.layoutOrigin; }
		}, *this);
	}

	std::string name(const EmuApp &app) const
	{
		return visit([&](auto &e){ return e.name(app); }, *this);
	}

	void updateMeasurements(const IG::Window &win)
	{
		visit([&](auto &e)
		{
			if constexpr(requires {e.updateMeasurements(win);})
			{
				e.updateMeasurements(win);
			}
		}, *this);
	}

	void transposeKeysForPlayer(const EmuApp &app, int player)
	{
		visit([&](auto &e)
		{
			if constexpr(requires {e.transposeKeysForPlayer(app, player);})
			{
				e.transposeKeysForPlayer(app, player);
			}
		}, *this);
	}
};

enum class VControllerVisibility : uint8_t
{
	OFF, ON, AUTO
};

class VController : public EmuAppHelper<VController>
{
public:
	static constexpr unsigned TOGGLE_KEYBOARD = 65536;
	static constexpr unsigned CHANGE_KEYBOARD_MODE = 65537;
	using KbMap = VControllerKeyboard::KbMap;
	static constexpr uint8_t GAMEPAD_DPAD_BIT = IG::bit(0);
	static constexpr uint8_t GAMEPAD_BUTTONS_BIT = IG::bit(1);
	static constexpr uint8_t GAMEPAD_BITS = GAMEPAD_DPAD_BIT | GAMEPAD_BUTTONS_BIT;

	VController(IG::ApplicationContext);
	int xMMSizeToPixel(const IG::Window &win, float mm) const;
	int yMMSizeToPixel(const IG::Window &win, float mm) const;
	void setInputPlayer(uint8_t player);
	uint8_t inputPlayer() const;
	void setDisabledInputKeys(std::span<const unsigned> keys);
	void updateKeyboardMapping();
	void updateTextures();
	void inputAction(Input::Action action, unsigned vBtn);
	void resetInput();
	void place();
	void toggleKeyboard();
	bool pointerInputEvent(const Input::MotionEvent &, IG::WindowRect gameRect);
	bool keyInput(const Input::KeyEvent &);
	void draw(Gfx::RendererCommands &__restrict__, bool showHidden = false);
	void draw(Gfx::RendererCommands &__restrict__, bool showHidden, float alpha);
	void setButtonSize(int gamepadBtnSizeInPixels, int uiBtnSizeInPixels);
	bool isInKeyboardMode() const;
	void setKeyboardImage(Gfx::TextureSpan img);
	void setButtonAlpha(std::optional<uint8_t>);
	constexpr uint8_t buttonAlpha() const { return alpha; }
	VControllerKeyboard &keyboard() { return kb; }
	void setRenderer(Gfx::Renderer &renderer);
	Gfx::Renderer &renderer();
	void setWindow(const IG::Window &win);
	bool hasWindow() const { return win; }
	const IG::Window &window() const { return *win; }
	const WindowData &windowData() const { return *winData; }
	bool layoutPositionChanged() const { return layoutPosChanged; };
	void setLayoutPositionChanged(bool changed = true) { layoutPosChanged = changed; }
	IG::ApplicationContext appContext() const;
	const Gfx::GlyphTextureSet &face() const;
	void setFace(const Gfx::GlyphTextureSet &face);
	bool setButtonSize(std::optional<uint16_t> mm100xOpt, bool placeElements = true);
	uint16_t buttonSize() const { return btnSize; }
	int buttonPixelSize(const IG::Window &) const;
	void setShowOnTouchInput(std::optional<bool> opt);
	bool showOnTouchInput() const;
	void setVibrateOnTouchInput(EmuApp &, std::optional<bool> opt);
	bool vibrateOnTouchInput() const;
	bool shouldShowOnTouchInput() const;
	void setGamepadControlsVisibility(std::optional<VControllerVisibility>);
	VControllerVisibility gamepadControlsVisibility() const;
	void setGamepadControlsVisible(bool on) { gamepadIsVisible = on; }
	bool gamepadControlsVisible() const { return gamepadIsVisible; }
	static bool visibilityIsValid(VControllerVisibility);
	void setPhysicalControlsPresent(bool);
	bool updateAutoOnScreenControlVisible();
	bool readConfig(EmuApp &, MapIO &, unsigned key, size_t size);
	void writeConfig(FileIO &) const;
	void configure(IG::Window &, Gfx::Renderer &, const Gfx::GlyphTextureSet &face);
	void applyLayout();
	void resetPositions();
	void resetAllOptions();
	static bool shouldDraw(VControllerState state, bool showHidden = false);
	void setGamepadIsEnabled(bool on) { gamepadDisabledFlags = on ? 0 : GAMEPAD_BITS; }
	void setGamepadDPadIsEnabled(bool on) { gamepadDisabledFlags = IG::setOrClearBits(gamepadDisabledFlags, GAMEPAD_DPAD_BIT, !on); }
	void setGamepadButtonsAreEnabled(bool on) { gamepadDisabledFlags = IG::setOrClearBits(gamepadDisabledFlags, GAMEPAD_BUTTONS_BIT, !on); }
	bool gamepadIsEnabled() const { return gamepadDisabledFlags != GAMEPAD_BITS; }
	bool gamepadDPadIsEnabled() const { return !(gamepadDisabledFlags & GAMEPAD_DPAD_BIT); }
	bool gamepadButtonsAreEnabled() const { return !(gamepadDisabledFlags & GAMEPAD_BUTTONS_BIT); }
	bool gamepadIsActive() const;
	bool allowButtonsPastContentBounds() const { return allowButtonsPastContentBounds_; }
	bool setAllowButtonsPastContentBounds(bool on) { return allowButtonsPastContentBounds_ = on; }
	auto gamepadItems() { return gpElements.size(); }
	void reset(SystemInputDeviceDesc);
	VControllerElement &add(InputComponentDesc);
	VControllerElement &add(std::span<const unsigned> keyCodes, InputComponent, _2DOrigin layoutOrigin);
	bool remove(VControllerElement &);
	std::span<VControllerElement> deviceElements() { return gpElements; }
	std::span<VControllerElement> guiElements() { return uiElements; }
	WRect layoutBounds() const;

private:
	Gfx::Renderer *renderer_{};
	const IG::Window *win{};
	const WindowData *winData{};
	const Gfx::GlyphTextureSet *facePtr{};
	VControllerKeyboard kb{};
	std::vector<VControllerElement> gpElements{};
	std::vector<VControllerElement> uiElements{};
	float alphaF{};
	Input::DragTracker<std::array<int, 2>> dragTracker{};
	uint16_t defaultButtonSize{};
	uint16_t btnSize{};
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
	IG_UseMemberIf(Config::DISPLAY_CUTOUT, bool, allowButtonsPastContentBounds_){};
	IG_UseMemberIf(Config::BASE_SUPPORTS_VIBRATOR, bool, vibrateOnTouchInput_){};

	std::array<int, 2> findGamepadElements(IG::WP pos);
	int keyboardKeyFromPointer(const Input::MotionEvent &);
	void applyButtonSize();
};

}
