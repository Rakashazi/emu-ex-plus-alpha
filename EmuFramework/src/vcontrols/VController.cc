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

#define LOGTAG "VController"
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include "../EmuOptions.hh"
#include "../WindowData.hh"
#include "../privateInput.hh"
#include <imagine/util/math/int.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/base/Window.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

static constexpr uint8_t DEFAULT_ALPHA = 255. * .5;

VController::VController(IG::ApplicationContext ctx):
	alphaF{DEFAULT_ALPHA / 255.},
	defaultButtonSize
	{
		#ifdef CONFIG_BASE_IOS
		uint16_t(ctx.deviceIsIPad() ? 1400 : 850)
		#else
		850
		#endif
	},
	btnSize{defaultButtonSize},
	alpha{DEFAULT_ALPHA} {}

int VController::xMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.widthMMInPixels(mm);
}

int VController::yMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.heightMMInPixels(mm);
}

static FRect gamepadButtonImageRect(VControllerImageIndex idx, float texHeight)
{
	using enum VControllerImageIndex;
	switch(idx)
	{
		case button1: return {{0., 82.f/texHeight}, {32./64., 114.f/texHeight}};
		case button2: return {{33./64., 83.f/texHeight}, {1., 114.f/texHeight}};
		case button3:
			if(texHeight == 128.f)
				return {{0., 82.f/texHeight}, {32./64., 114.f/texHeight}};
			else
				return {{0., 115.f/texHeight}, {32./64., 147.f/texHeight}};
		case button4:
			if(texHeight == 128.f)
				return {{33./64., 83.f/texHeight}, {1., 114.f/texHeight}};
			else
				return {{33./64., 116.f/texHeight}, {1., 147.f/texHeight}};
		case button5: return {{0., 148.f/texHeight}, {32./64., 180.f/texHeight}};
		case button6: return {{33./64., 149.f/texHeight}, {1., 180.f/texHeight}};
		case button7: return {{0., 181.f/texHeight}, {32./64., 213.f/texHeight}};
		case button8: return {{33./64., 182.f/texHeight}, {1., 213.f/texHeight}};
		case auxButton1: return {{0., 65.f/texHeight}, {32./64., 81.f/texHeight}};
		case auxButton2: return {{33./64., 65.f/texHeight}, {1., 81.f/texHeight}};
	}
	bug_unreachable("invalid VControllerImageIndex");
}

static int gamepadButtonImageAspectRatio(VControllerImageIndex idx)
{
	using enum VControllerImageIndex;
	switch(idx)
	{
		case auxButton1 ... auxButton2: return 2;
		default: return 1;
	}
}

static void mapGamepadButtonImage(VControllerButton &btn, const EmuSystem &sys, Gfx::Texture &tex, unsigned key, float texHeight)
{
	auto idx = sys.mapVControllerButton(key);
	btn.setImage({&tex, gamepadButtonImageRect(idx, texHeight)}, gamepadButtonImageAspectRatio(idx));
}

static void updateTexture(const EmuApp &app, VControllerElement &e)
{
	const float h = EmuSystem::inputFaceBtns == 2 || EmuSystem::inputHasShortBtnTexture ? 128. : 256.;
	visit(overloaded
	{
		[&](VControllerDPad &dpad){ dpad.setImage(Gfx::TextureSpan{&app.asset(AssetID::GAMEPAD_OVERLAY), {{}, {1., 64.f/h}}}); },
		[&](VControllerButtonGroup &grp)
		{
			for(auto &btn : grp.buttons)
			{
				mapGamepadButtonImage(btn, app.system(), app.asset(AssetID::GAMEPAD_OVERLAY), btn.key, h);
			}
		},
		[&](VControllerUIButtonGroup &grp)
		{
			for(auto &btn : grp.buttons)
			{
				switch(btn.key)
				{
					case guiKeyIdxLastView: btn.setImage({&app.asset(AssetID::MENU)}); break;
					case guiKeyIdxToggleFastForward:
					case guiKeyIdxFastForward: btn.setImage({&app.asset(AssetID::FAST_FORWARD)}); break;
					default: btn.setImage({&app.asset(AssetID::MENU)}); break;
				}
			}
		}
	}, e);
}

void VController::updateTextures()
{
	for(auto &e : gpElements)
	{
		updateTexture(app(), e);
	}
	for(auto &e : uiElements)
	{
		updateTexture(app(), e);
	}
}

void VController::setButtonSize(int gamepadBtnSizeInPixels, int uiBtnSizeInPixels)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(gamepadBtnSizeInPixels, gamepadBtnSizeInPixels * .75, layoutBounds());
	if constexpr(VCONTROLS_GAMEPAD)
	{
		IG::WP size{gamepadBtnSizeInPixels, gamepadBtnSizeInPixels};
		for(auto &elem : gpElements)
		{
			visit(overloaded
			{
				[&](VControllerDPad &dpad){ dpad.setSize(renderer(), IG::makeEvenRoundedUp(int(size.x*(double)2.5))); },
				[&](VControllerButtonGroup &grp){ grp.setButtonSize(size); },
				[](auto &){}
			}, elem);
		}
	}
	IG::WP size = {uiBtnSizeInPixels, uiBtnSizeInPixels};
	for(auto &e : uiElements)
	{
		visit(overloaded
		{
			[&](VControllerUIButtonGroup &grp){ grp.setButtonSize(size); },
			[](auto &){}
		}, e);
	}
}

void VController::applyButtonSize()
{
	setButtonSize(buttonPixelSize(window()), face().nominalHeight()*1.75);
}

void VController::inputAction(Input::Action action, unsigned vBtn)
{
	if(isInKeyboardMode())
	{
		system().handleInputAction(&app(), {kb.translateInput(vBtn), action});
	}
	else
	{
		app().handleSystemKeyInput({vBtn, action});
	}
}

void VController::resetInput()
{
	for(auto &e : dragTracker.stateList())
	{
		for(auto &vBtn : e.data)
		{
			if(vBtn != -1) // release old key, if any
				inputAction(Input::Action::RELEASED, vBtn);
		}
	}
	dragTracker.reset();
}

void VController::place()
{
	if(!hasWindow())
		return;
	auto &winData = windowData();
	auto &win = window();
	applyButtonSize();
	auto contentBounds = windowData().contentBounds();
	auto bounds = layoutBounds();
	bool isPortrait = window().isPortrait();
	for(auto &elem : gpElements)
	{
		elem.place(bounds, contentBounds, isPortrait);
	}
	for(auto &elem : uiElements)
	{
		elem.place(bounds, contentBounds, isPortrait);
	}
	dragTracker.setDragStartPixels(window().widthMMInPixels(1.));
}

void VController::toggleKeyboard()
{
	logMsg("toggling keyboard");
	resetInput();
	kbMode ^= true;
	system().onVKeyboardShown(kb, kbMode);
}

std::array<int, 2> VController::findGamepadElements(IG::WP pos)
{
	for(const auto &gpElem : gpElements)
	{
		auto indices = visit(overloaded
		{
			[&](const VControllerDPad &dpad) -> std::array<int, 2>
			{
				if(!gamepadDPadIsEnabled())
					return {-1, -1};
				return dpad.getInput(pos);
			},
			[&](const VControllerButtonGroup &grp) -> std::array<int, 2>
			{
				if(!gamepadButtonsAreEnabled())
					return {-1, -1};
				return grp.findButtonIndices(pos);
			},
			[](auto &e) -> std::array<int, 2> { return {-1, -1}; }
		}, gpElem);
		if(indices != std::array<int, 2>{-1, -1})
			return indices;
	}
	return {-1, -1};
}

int VController::keyboardKeyFromPointer(const Input::MotionEvent &e)
{
	assert(isInKeyboardMode());
	if(e.pushed())
	{
		kb.unselectKey();
	}
	int kbIdx = kb.getInput(e.pos());
	if(kbIdx == -1)
		return -1;
	if(kb.translateInput(kbIdx) == TOGGLE_KEYBOARD)
	{
		if(!e.pushed())
			return -1;
		logMsg("dismiss kb");
		toggleKeyboard();
	}
	else if(kb.translateInput(kbIdx) == CHANGE_KEYBOARD_MODE)
	{
		if(!e.pushed())
			return -1;
		logMsg("switch kb mode");
		kb.cycleMode(system(), renderer());
		resetInput();
	}
	else
		 return kbIdx;
	return -1;
}

bool VController::pointerInputEvent(const Input::MotionEvent &e, IG::WindowRect gameRect)
{
	if(e.pushed())
	{
		for(const auto &grp: uiElements)
		{
			for(const auto &btn: grp.uiButtonGroup()->buttons)
			{
				if(btn.bounds().overlaps(e.pos()))
				{
					app().handleKeyInput({btn.key, e.state()}, e);
					return true;
				}
			}
		}
	}
	static constexpr std::array<int, 2> nullElems{-1, -1};
	std::array<int, 2> newElems = nullElems;
	if(isInKeyboardMode())
	{
		newElems[0] = keyboardKeyFromPointer(e);
	}
	else
	{
		if(gamepadIsActive())
		{
			newElems = findGamepadElements(e.pos());
		}
	}
	bool elementsArePushed = newElems != nullElems;
	auto applyInputActions =
		[&](std::array<int, 2> prevElements, std::array<int, 2> currElements)
		{
			// release old buttons
			for(auto vBtn : prevElements)
			{
				if(vBtn != -1 && !IG::contains(currElements, vBtn))
				{
					//logMsg("releasing %d", vBtn);
					inputAction(Input::Action::RELEASED, vBtn);
				}
			}
			// push new buttons
			for(auto vBtn : currElements)
			{
				if(vBtn != -1 && !IG::contains(prevElements, vBtn))
				{
					//logMsg("pushing %d", vBtn);
					inputAction(Input::Action::PUSHED, vBtn);
					if(vibrateOnTouchInput())
					{
						app().vibrationManager().vibrate(IG::Milliseconds{32});
					}
				}
			}
		};
	dragTracker.inputEvent(e,
		[&](Input::DragTrackerState dragState, auto &currElems)
		{
			applyInputActions(nullElems, newElems);
			currElems = newElems;
			if(!elementsArePushed)
			{
				elementsArePushed |= system().onPointerInputStart(e, dragState, gameRect);
			}
		},
		[&](Input::DragTrackerState dragState, Input::DragTrackerState prevDragState, auto &currElems)
		{
			applyInputActions(currElems, newElems);
			currElems = newElems;
			if(!elementsArePushed)
			{
				elementsArePushed |= system().onPointerInputUpdate(e, dragState, prevDragState, gameRect);
			}
		},
		[&](Input::DragTrackerState dragState, auto &currElems)
		{
			applyInputActions(currElems, nullElems);
			elementsArePushed |= system().onPointerInputEnd(e, dragState, gameRect);
		});
	 if(!elementsArePushed && !gamepadControlsVisible() && shouldShowOnTouchInput()
			&& !isInKeyboardMode() && e.isTouch() && e.pushed()) [[unlikely]]
		{
			logMsg("turning on on-screen controls from touch input");
			setGamepadControlsVisible(true);
			app().viewController().placeEmuViews();
		}
	return elementsArePushed;
}

bool VController::keyInput(const Input::KeyEvent &e)
{
	if(!isInKeyboardMode())
		return false;
	return kb.keyInput(*this, renderer(), e);
}

void VController::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden)
{
	draw(cmds, showHidden, alphaF);
}

void VController::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden, float alpha)
{
	if(alpha == 0.f) [[unlikely]]
		return;
	cmds.set(Gfx::BlendMode::ALPHA);
	Gfx::Color whiteCol{1., 1., 1., alpha};
	cmds.setColor(whiteCol);
	if(isInKeyboardMode())
		kb.draw(cmds);
	else if(gamepadIsVisible)
	{
		for(const auto &e : gpElements)
		{
			if(e.buttonGroup() && gamepadDisabledFlags & VController::GAMEPAD_BUTTONS_BIT)
				continue;
			if(e.dPad() && gamepadDisabledFlags & VController::GAMEPAD_DPAD_BIT)
				continue;
			e.draw(cmds, showHidden);
		}
	}
	for(auto &e : uiElements)
	{
		cmds.setColor(whiteCol);
		e.draw(cmds);
	}
}

bool VController::isInKeyboardMode() const
{
	return EmuSystem::inputHasKeyboard && kbMode;
}

void VController::setInputPlayer(uint8_t player)
{
	inputPlayer_ = player;
	for(auto &e : gpElements)
	{
		e.transposeKeysForPlayer(app(), player);
	}
}

uint8_t VController::inputPlayer() const
{
	return inputPlayer_;
}

void VController::setDisabledInputKeys(std::span<const unsigned> disabledKeys)
{
	for(auto &e : gpElements)
	{
		visit(overloaded
		{
			[&](VControllerButtonGroup &grp)
			{
				for(auto &btn : grp.buttons)
					btn.enabled = !contains(disabledKeys, btn.key);
			},
			[](auto &e){}
		}, e);
	}
	place();
}

void VController::updateKeyboardMapping()
{
	kb.updateKeyboardMapping(system());
}

void VController::setKeyboardImage(Gfx::TextureSpan img)
{
	kb.setImg(renderer(), img);
}

void VController::setButtonAlpha(std::optional<uint8_t> opt)
{
	if(!opt)
		return;
	alpha = *opt;
	alphaF = *opt / 255.f;
}

void VController::setRenderer(Gfx::Renderer &renderer)
{
	renderer_ = &renderer;
}

Gfx::Renderer &VController::renderer()
{
	return *renderer_;
}

void VController::setWindow(const IG::Window &win_)
{
	win = &win_;
	winData = &EmuEx::windowData(win_);
}

IG::ApplicationContext VController::appContext() const
{
	assert(hasWindow());
	return window().appContext();
}

const Gfx::GlyphTextureSet &VController::face() const
{
	return *facePtr;
}

void VController::setFace(const Gfx::GlyphTextureSet &face)
{
	facePtr = &face;
}

bool VController::setButtonSize(std::optional<uint16_t> mm100xOpt, bool placeElements)
{
	if(!mm100xOpt || *mm100xOpt < 300 || *mm100xOpt > 1500)
		return false;
	btnSize = *mm100xOpt;
	if(placeElements)
		place();
	return true;
}

int VController::buttonPixelSize(const IG::Window &win) const
{
	return IG::makeEvenRoundedUp(xMMSizeToPixel(win, buttonSize() / 100.f));
}

void VController::setShowOnTouchInput(std::optional<bool> opt)
{
	if(!opt)
		return;
	showOnTouchInput_ = *opt;
}

bool VController::showOnTouchInput() const
{
	return showOnTouchInput_;
}

bool VController::shouldShowOnTouchInput() const
{
	return gamepadControlsVisibility() == VControllerVisibility::AUTO && showOnTouchInput();
}

void VController::setVibrateOnTouchInput(EmuApp &app, std::optional<bool> opt)
{
	if(!opt || !app.vibrationManager().hasVibrator())
		return;
	vibrateOnTouchInput_ = *opt;
}

bool VController::vibrateOnTouchInput() const
{
	return vibrateOnTouchInput_;
}

void VController::setGamepadControlsVisibility(std::optional<VControllerVisibility> opt)
{
	if(!opt)
		return;
	gamepadControlsVisibility_ = *opt;
	updateAutoOnScreenControlVisible();
}

VControllerVisibility VController::gamepadControlsVisibility() const
{
	return gamepadControlsVisibility_;
}

bool VController::visibilityIsValid(VControllerVisibility vis)
{
	return vis <= VControllerVisibility::AUTO;
}

void VController::setPhysicalControlsPresent(bool present)
{
	if(present != physicalControlsPresent)
	{
		logMsg("Physical controls present:%s", present ? "y" : "n");
	}
	physicalControlsPresent = present;
	updateAutoOnScreenControlVisible();
}

bool VController::updateAutoOnScreenControlVisible()
{
	if(gamepadControlsVisibility() == VControllerVisibility::AUTO)
	{
		if(gamepadIsVisible && physicalControlsPresent)
		{
			logMsg("auto-turning off on-screen controls");
			gamepadIsVisible = false;
			return true;
		}
		else if(!gamepadIsVisible && !physicalControlsPresent)
		{
			logMsg("auto-turning on on-screen controls");
			gamepadIsVisible = true;
			return true;
		}
	}
	else
	{
		gamepadIsVisible = gamepadControlsVisibility() == VControllerVisibility::ON;
	}
	return false;
}

bool VController::readConfig(EmuApp &app, MapIO &io, unsigned key, size_t size)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_TOUCH_CONTROL_ALPHA:
			setButtonAlpha(readOptionValue<uint8_t>(io, size));
			return true;
		case CFGKEY_TOUCH_CONTROL_DISPLAY:
			setGamepadControlsVisibility(readOptionValue<VControllerVisibility>(io, size, visibilityIsValid));
			return true;
		case CFGKEY_TOUCH_CONTROL_SIZE:
			setButtonSize(readOptionValue<uint16_t>(io, size), false);
			return true;
		case CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH:
			setShowOnTouchInput(readOptionValue<bool>(io, size));
			return true;
		case CFGKEY_TOUCH_CONTROL_VIRBRATE:
			setVibrateOnTouchInput(app, readOptionValue<bool>(io, size));
			return true;
		case CFGKEY_VCONTROLLER_ALLOW_PAST_CONTENT_BOUNDS: return readOptionValue(io, size, allowButtonsPastContentBounds_);
	}
}

void VController::writeConfig(FileIO &io) const
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		if(buttonAlpha() != DEFAULT_ALPHA)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_ALPHA, buttonAlpha());
		if(buttonSize() != defaultButtonSize)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_SIZE, buttonSize());
		if(!showOnTouchInput())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH, showOnTouchInput());
		if(gamepadControlsVisibility() != DEFAULT_GAMEPAD_CONTROLS_VISIBILITY)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_DISPLAY, gamepadControlsVisibility());
		if(vibrateOnTouchInput())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_VIRBRATE, vibrateOnTouchInput());
		if(allowButtonsPastContentBounds_)
			writeOptionValue(io, CFGKEY_VCONTROLLER_ALLOW_PAST_CONTENT_BOUNDS, true);
	}
	if(layoutPositionChanged())
	{
		// TODO: write element state to config
	}
}

void VController::configure(IG::Window &win, Gfx::Renderer &renderer, const Gfx::GlyphTextureSet &face)
{
	setWindow(win);
	setRenderer(renderer);
	setFace(face);
}

void VController::applyLayout()
{
	auto &app = this->app();
	auto &winData = windowData();
	if constexpr(VCONTROLS_GAMEPAD)
	{
		for(auto &e : uiElements) { e.updateMeasurements(window()); };
		for(auto &e : gpElements) { e.updateMeasurements(window()); };
	}
	applyButtonSize();
	if(!layoutPositionChanged()) // setup default positions if not provided in config file
		resetPositions();
	setInputPlayer(0);
}

void VController::resetPositions()
{
	auto ctx = appContext();
	auto &win = window();
	logMsg("resetting on-screen controls to default positions & states");
	auto initFastForwardState = (Config::envIsIOS || (Config::envIsAndroid  && !ctx.hasHardwareNavButtons()))
		? VControllerState::SHOWN : VControllerState::OFF;
	auto initMenuState = (Config::envIsAndroid && ctx.hasHardwareNavButtons())
		? VControllerState::HIDDEN : VControllerState::SHOWN;
	auto defaultSidePadding = xMMSizeToPixel(win, 4.);
	for(int leftY{}, prevLeftY{}, rightY{}, prevRightY{}; auto &e : uiElements)
	{
		if(e.layoutOrigin() == RT2DO)
		{
			auto yOffset = std::max(rightY, prevLeftY);
			e.layoutPos[0] = e.layoutPos[1] = {RT2DO, {-defaultSidePadding, yOffset}, initMenuState};
			prevRightY = rightY;
			rightY += e.realBounds().ySize() + (e.realBounds().ySize() / 2);
		}
		else if(e.layoutOrigin() == LT2DO)
		{
			auto yOffset = std::max(leftY, prevRightY);
			e.layoutPos[0] = e.layoutPos[1] = {LT2DO, {defaultSidePadding, yOffset}, initFastForwardState};
			prevLeftY = leftY;
			leftY += e.realBounds().ySize() + (e.realBounds().ySize() / 2);
		}
	}
	int xOffset = xMMSizeToPixel(win, 3.f);
	int xOffsetPortrait = xMMSizeToPixel(win, 1.f);
	int buttonPixels = buttonPixelSize(win);
	for(int leftY{}, prevLeftY{}, centerY{}, rightY{}, prevRightY{}; auto &e : gpElements)
	{
		const auto halfSize = e.bounds().size() / 2;
		if(e.layoutOrigin() == LB2DO)
		{
			auto yOffset = std::max(leftY, prevRightY);
			e.layoutPos[0] = {LB2DO, {xOffset + halfSize.x, -buttonPixels - halfSize.y - yOffset}};
			e.layoutPos[1] = {LB2DO, {xOffsetPortrait + halfSize.x, -buttonPixels - halfSize.y - yOffset}};
			prevLeftY = leftY;
			leftY += e.realBounds().ySize();
		}
		else if(e.layoutOrigin() == CB2DO)
		{
			e.layoutPos[0] = e.layoutPos[1] = {CB2DO, {0, -centerY}};
			centerY += e.realBounds().ySize();
		}
		else if(e.layoutOrigin() == RB2DO)
		{
			auto yOffset = std::max(rightY, prevLeftY);
			e.layoutPos[0] = {RB2DO, {-xOffset - halfSize.x, -buttonPixels - halfSize.y - yOffset}};
			e.layoutPos[1] = {RB2DO, {-xOffsetPortrait - halfSize.x, -buttonPixels - halfSize.y - yOffset}};
			prevRightY = rightY;
			rightY += e.realBounds().ySize();
		}
	}
	setLayoutPositionChanged(false);
}

void VController::resetAllOptions()
{
	gamepadControlsVisibility_ = DEFAULT_GAMEPAD_CONTROLS_VISIBILITY;
	btnSize = defaultButtonSize;
	vibrateOnTouchInput_ = false;
	showOnTouchInput_ = true;
	allowButtonsPastContentBounds_ = false;
	reset(system().inputDeviceDesc(0));
	resetPositions();
	setButtonAlpha(DEFAULT_ALPHA);
	updateAutoOnScreenControlVisible();
	setInputPlayer(0);
}

VControllerLayoutPosition VControllerLayoutPosition::fromPixelPos(IG::WP pos, IG::WP size, IG::WindowRect viewBounds)
{
	IG::WindowRect bound {pos - size/2, pos + size/2};

	const auto &rect = viewBounds;
	IG::WindowRect ltQuadrantRect{{rect.x, rect.y}, rect.center()};
	IG::WindowRect rtQuadrantRect{{rect.xCenter(), rect.y}, {rect.x2, rect.yCenter()}};
	IG::WindowRect lbQuadrantRect{{rect.x, rect.yCenter()}, {rect.xCenter(), rect.y2}};
	IG::WindowRect rbQuadrantRect{rect.center(), {rect.x2, rect.y2}};
	bool ltQuadrant = bound.overlaps(ltQuadrantRect);
	bool rtQuadrant = bound.overlaps(rtQuadrantRect);
	bool lbQuadrant = bound.overlaps(lbQuadrantRect);
	bool rbQuadrant = bound.overlaps(rbQuadrantRect);
	_2DOrigin origin = C2DO;
	if(ltQuadrant && rtQuadrant && lbQuadrant && rbQuadrant) origin = C2DO;
	else if(ltQuadrant && rtQuadrant) origin = CT2DO;
	else if(ltQuadrant && lbQuadrant) origin = LC2DO;
	else if(rtQuadrant && rbQuadrant) origin = RC2DO;
	else if(lbQuadrant && rbQuadrant) origin = CB2DO;
	else if(ltQuadrant) origin = LT2DO;
	else if(rtQuadrant) origin = RT2DO;
	else if(lbQuadrant) origin = LB2DO;
	else if(rbQuadrant) origin = RB2DO;

	int x = (origin.xScaler() == 0) ? pos.x - rect.xSize()/2 :
		(origin.xScaler() == 1) ? pos.x - rect.xSize() : pos.x;
	int y = LT2DO.adjustY(pos.y, rect.ySize(), origin);
	return {origin, {x, y}, VControllerState::SHOWN};
}

IG::WP VControllerLayoutPosition::toPixelPos(IG::WindowRect viewBounds) const
{
	int x = (origin.xScaler() == 0) ? pos.x + viewBounds.xSize() / 2 :
		(origin.xScaler() == 1) ? pos.x + viewBounds.xSize() : pos.x;
	int y = origin.adjustY(pos.y, viewBounds.ySize(), LT2DO);
	return {x, y};
}

bool VController::shouldDraw(VControllerState state, bool showHidden)
{
	return state == VControllerState::SHOWN || (showHidden && state != VControllerState::OFF);
}

bool VController::gamepadIsActive() const
{
	return gamepadIsEnabled() && gamepadControlsVisible();
}

void VController::reset(SystemInputDeviceDesc desc)
{
	gpElements.clear();
	for(const auto &c : desc.components)
	{
		add(c);
	}
}

VControllerElement &VController::add(InputComponentDesc c)
{
	return add(c.keyCodes, c.type, c.layoutOrigin);
}

VControllerElement &VController::add(std::span<const unsigned> keyCodes, InputComponent type, _2DOrigin layoutOrigin)
{
	auto &elem = [&]() -> VControllerElement&
	{
		switch(type)
		{
			case InputComponent::ui:
				return uiElements.emplace_back(std::in_place_type<VControllerUIButtonGroup>, keyCodes, layoutOrigin);
			case InputComponent::dPad:
				assert(keyCodes.size() == 4);
				return gpElements.emplace_back(std::in_place_type<VControllerDPad>, std::span<const unsigned, 4>{keyCodes.data(), 4});
			case InputComponent::button:
			case InputComponent::trigger:
				return gpElements.emplace_back(std::in_place_type<VControllerButtonGroup>, keyCodes, layoutOrigin);
		}
		bug_unreachable("invalid InputComponent");
	}();
	update(elem);
	auto layoutPos = VControllerLayoutPosition::fromPixelPos(layoutBounds().center(), elem.bounds().size(), layoutBounds());
	elem.layoutPos[0] = elem.layoutPos[1] = layoutPos;
	return elem;
}

void VController::update(VControllerElement &elem)
{
	if(!hasWindow())
		return;
	updateTexture(app(), elem);
	elem.updateMeasurements(window());
	applyButtonSize();
}

bool VController::remove(VControllerElement &elemToErase)
{
	return std::erase_if(gpElements, [&](auto &e) { return &e == &elemToErase; }) ||
		std::erase_if(uiElements, [&](auto &e) { return &e == &elemToErase; });
}

WRect VController::layoutBounds() const
{
	return allowButtonsPastContentBounds() ? windowData().windowBounds() : windowData().contentBounds();
}

}
