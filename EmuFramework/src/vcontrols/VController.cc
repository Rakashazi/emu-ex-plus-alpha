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
#include <imagine/util/math/int.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/base/Window.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

struct [[gnu::packed]] VControllerLayoutPositionSerialized
{
	uint8_t origin{};
	VControllerState state{};
	int pos[2]{};
};

static constexpr uint8_t DEFAULT_ALPHA = 255. * .5;
static constexpr uint16_t DEFAULT_DPAD_DEADZONE = 135;
static constexpr uint16_t DEFAULT_DPAD_DIAGONAL_SENSITIVITY = 1750;
static constexpr uint16_t DEFAULT_BUTTON_EXTRA_BOUNDS_WIDTH = 200;
static constexpr uint16_t DEFAULT_BUTTON_EXTRA_BOUNDS_HEIGHT = 200;

VController::VController(IG::ApplicationContext ctx, int faceButtons, int centerButtons):
	gp{faceButtons, centerButtons},
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
	buttonXPadding_{DEFAULT_BUTTON_EXTRA_BOUNDS_WIDTH},
	buttonYPadding_{DEFAULT_BUTTON_EXTRA_BOUNDS_HEIGHT},
	dpadDzone{DEFAULT_DPAD_DEADZONE},
	dpadDiagonalSensitivity_{DEFAULT_DPAD_DIAGONAL_SENSITIVITY},
	alpha{DEFAULT_ALPHA}
{}

float VController::xMMSize(float mm) const
{
	return windowData().projection.plane().xMMSize(mm);
}

float VController::yMMSize(float mm) const
{
	return windowData().projection.plane().yMMSize(mm);
}

int VController::xMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.widthMMInPixels(mm);
}

int VController::yMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.heightMMInPixels(mm);
}

bool VController::hasTriggers() const
{
	return EmuSystem::inputHasTriggers();
}

void VController::setImg(Gfx::Texture &pics)
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		gp.setImg(renderer(), pics);
	}
}

void VController::setMenuBtnPos(IG::WP pos)
{
	menuBtn.setPos(pos, windowData().projection.plane());
}

void VController::setFFBtnPos(IG::WP pos)
{
	ffBtn.setPos(pos, windowData().projection.plane());
}

void VController::setButtonSize(unsigned gamepadBtnSizeInPixels, unsigned uiBtnSizeInPixels, Gfx::ProjectionPlane projP)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(projP.unprojectYSize(gamepadBtnSizeInPixels), projP.unprojectYSize(gamepadBtnSizeInPixels * .75), projP);
	if constexpr(VCONTROLS_GAMEPAD)
	{
		IG::WP size{(int)gamepadBtnSizeInPixels, (int)gamepadBtnSizeInPixels};
		IG::WP extraFaceBtnSize
		{
			int(gamepadBtnSizeInPixels * (buttonXPadding() / 1000.f)),
			int(gamepadBtnSizeInPixels * (buttonYPadding() / 1000.f))
		};
		gp.setFaceButtonSize(renderer(), size, extraFaceBtnSize, projP);
		gp.centerButtons().setButtonSize(size, extraFaceBtnSize);
	}
	IG::WP size = {(int)uiBtnSizeInPixels, (int)uiBtnSizeInPixels};
	if(menuBtn.bounds().size() != size)
		logMsg("set UI button size:%d", size.x);
	menuBtn.setSize(size);
	ffBtn.setSize(size);
}

void VController::applyButtonSize()
{
	setButtonSize(buttonPixelSize(window()), face().nominalHeight()*1.75, winData->projection.plane());
}

void VController::inputAction(Input::Action action, unsigned vBtn)
{
	if(isInKeyboardMode())
	{
		system().handleInputAction(&app(), action, kb.translateInput(vBtn));
	}
	else
	{
		assert(vBtn < std::size(map));
		auto turbo = map[vBtn] & TURBO_BIT;
		auto keyCode = map[vBtn] & ACTION_MASK;
		if(turbo)
		{
			if(action == Input::Action::PUSHED)
			{
				app().addTurboInputEvent(keyCode);
			}
			else
			{
				app().removeTurboInputEvent(keyCode);
			}
		}
		system().handleInputAction(&app(), action, keyCode);
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
	auto &winData = windowData();
	auto &win = window();
	if constexpr(VCONTROLS_GAMEPAD)
	{
		auto &gp = gamePad();
		gp.setSpacingPixels(IG::makeEvenRoundedUp(xMMSizeToPixel(win, buttonSpacing() / 100.)));
		gp.setTriggersInline(triggersInline());
		gp.setStaggerType(buttonStagger());
		gp.setBoundingAreaVisible(renderer(), boundingAreaVisible(), winData.projection.plane());
	}
	applyButtonSize();
	auto &layoutPos = layoutPosition()[winData.viewport().isPortrait() ? 1 : 0];
	iterateTimes(numElements(), i)
	{
		setPos(i, layoutToPixelPos(layoutPos[i], winData.viewport()));
		setState(i, layoutPos[i].state);
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
	if constexpr(VCONTROLS_GAMEPAD)
	{
		if(gamepadButtonsAreEnabled())
		{
			if(auto elem = gp.centerButtons().findButtonIndices(pos);
				elem[0] != -1)
			{
				return {C_ELEM + elem[0], elem[1] != -1 ? C_ELEM + elem[1] : -1};
			}
			if(auto elem = gp.faceButtons().findButtonIndices(pos);
				elem[0] != -1)
			{
				return {F_ELEM + elem[0], elem[1] != -1 ? F_ELEM + elem[1] : -1};
			}
		}
		if(gamepadDPadIsEnabled() && gp.dPad().state() != VControllerState::OFF)
		{
			int elem = gp.dPad().getInput(pos);
			if(elem != -1)
			{
				return {D_ELEM + elem, -1};
			}
		}
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
		kb.setMode(renderer(), kb.mode() ^ true);
		resetInput();
	}
	else
		 return kbIdx;
	return -1;
}

bool VController::pointerInputEvent(const Input::MotionEvent &e, IG::WindowRect gameRect)
{
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

void VController::draw(Gfx::RendererCommands &cmds, bool activeFF, bool showHidden)
{
	draw(cmds, activeFF, showHidden, alphaF);
}

void VController::draw(Gfx::RendererCommands &cmds, bool activeFF, bool showHidden, float alpha)
{
	if(alpha == 0.f) [[unlikely]]
		return;
	auto projP = windowData().projection.plane();
	cmds.setBlendMode(Gfx::BLEND_MODE_ALPHA);
	Gfx::Color whiteCol{1., 1., 1., alpha};
	cmds.setColor(whiteCol);
	if(isInKeyboardMode())
		kb.draw(cmds, projP);
	else if(gamepadIsVisible)
	{
		if(gamepadDPadIsEnabled())
			gp.drawDPads(cmds, showHidden, projP);
		if(gamepadButtonsAreEnabled())
			gp.drawButtons(cmds, showHidden, projP);
	}
	menuBtn.draw(cmds, whiteCol, showHidden);
	Gfx::Color redCol{1., 0., 0., alpha};
	ffBtn.draw(cmds, activeFF ? redCol : whiteCol, showHidden);
}

int VController::numElements() const
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		return (EmuSystem::inputHasTriggers() && !gp.triggersInline()) ? 7 : 5;
	}
	else
	{
		return 5;
	}
}

IG::WindowRect VController::bounds(int elemIdx) const
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		switch(elemIdx)
		{
			case 0: return gp.dPad().bounds();
			case 1: return gp.centerButtons().bounds();
			case 2: return gp.faceButtons().bounds();
			case 3: return menuBtn.bounds();
			case 4: return ffBtn.bounds();
			case 5: return gp.lTrigger().bounds();
			case 6: return gp.rTrigger().bounds();
			default: bug_unreachable("elemIdx == %d", elemIdx); return {};
		}
	}
	else
	{
		switch(elemIdx)
		{
			case 0: return {};
			case 1: return {};
			case 2: return {};
			case 3: return menuBtn.bounds();
			case 4: return ffBtn.bounds();
			default: bug_unreachable("elemIdx == %d", elemIdx); return {};
		}
	}
}

void VController::setPos(int elemIdx, IG::WP pos)
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		switch(elemIdx)
		{
			bcase 0: gp.dPad().setPos(pos, windowData().projection.plane());
			bcase 1: gp.centerButtons().setPos(pos, windowData().projection.plane());
			bcase 2: gp.faceButtons().setPos(pos, windowData().projection.plane());
			bcase 3: setMenuBtnPos(pos);
			bcase 4: setFFBtnPos(pos);
			bcase 5: gp.lTrigger().setPos(pos, windowData().projection.plane());
			bcase 6: gp.rTrigger().setPos(pos, windowData().projection.plane());
			bdefault: bug_unreachable("elemIdx == %d", elemIdx);
		}
	}
	else
	{
		switch(elemIdx)
		{
			bcase 0:
			bcase 1:
			bcase 2:
			bcase 3: setMenuBtnPos(pos);
			bcase 4: setFFBtnPos(pos);
			bdefault: bug_unreachable("elemIdx == %d", elemIdx);
		}
	}
}

void VController::setState(int elemIdx, VControllerState state)
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		switch(elemIdx)
		{
			bcase 0: gp.dPad().setState(state);
			bcase 1: gp.centerButtons().setState(state);
			bcase 2: gp.faceButtons().setState(state);
			bcase 3: menuBtn.setState(state);
			bcase 4: ffBtn.setState(state);
			bcase 5: gp.lTrigger().setState(state);
			bcase 6: gp.lTrigger().setState(state);
			bdefault: bug_unreachable("elemIdx == %d", elemIdx);
		}
	}
	else
	{
		switch(elemIdx)
		{
			bcase 0:
			bcase 1:
			bcase 2:
			bcase 3: menuBtn.setState(state);
			bcase 4: ffBtn.setState(state);
			bdefault: bug_unreachable("elemIdx == %d", elemIdx);
		}
	}
}

VControllerState VController::state(int elemIdx) const
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		switch(elemIdx)
		{
			case 0: return gp.dPad().state();
			case 1: return gp.centerButtons().state();
			case 2: return gp.faceButtons().state();
			case 3: return menuBtn.state();
			case 4: return ffBtn.state();
			case 5: return gp.lTrigger().state();
			case 6: return gp.rTrigger().state();
			default: bug_unreachable("elemIdx == %d", elemIdx); return VControllerState::OFF;
		}
	}
	else
	{
		switch(elemIdx)
		{
			case 0: return VControllerState::OFF;
			case 1: return VControllerState::OFF;
			case 2: return VControllerState::OFF;
			case 3: return menuBtn.state();
			case 4: return ffBtn.state();
			default: bug_unreachable("elemIdx == %d", elemIdx); return VControllerState::OFF;
		}
	}
}

bool VController::isInKeyboardMode() const
{
	return EmuSystem::inputHasKeyboard && kbMode;
}

void VController::setInputPlayer(uint8_t player)
{
	inputPlayer_ = player;
	updateMapping();
}

uint8_t VController::inputPlayer() const
{
	return inputPlayer_;
}

void VController::updateMapping()
{
	updateVControllerMapping(inputPlayer(), map);
}

void VController::updateKeyboardMapping()
{
	kb.updateKeyboardMapping();
}

[[gnu::weak]] VController::KbMap updateVControllerKeyboardMapping(unsigned mode) { return {}; }

void VController::setMenuImage(Gfx::TextureSpan img)
{
	menuBtn.setImage(img);
}

void VController::setFastForwardImage(Gfx::TextureSpan img)
{
	ffBtn.setImage(img);
}

void VController::setKeyboardImage(Gfx::TextureSpan img)
{
	kb.setImg(renderer(), img);
}

bool VController::menuHitTest(IG::WP pos)
{
	auto &layoutPos = layoutPosition()[windowData().viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_MENU_IDX].state != VControllerState::OFF && menuBtn.realBounds().overlaps(pos);
}

bool VController::fastForwardHitTest(IG::WP pos)
{
	auto &layoutPos = layoutPosition()[windowData().viewport().isPortrait() ? 1 : 0];
	return layoutPos[VCTRL_LAYOUT_FF_IDX].state != VControllerState::OFF && ffBtn.realBounds().overlaps(pos);
}

void VController::setButtonAlpha(std::optional<uint8_t> opt)
{
	if(!opt)
		return;
	alpha = *opt;
	alphaF = *opt / 255.f;
}

VControllerGamepad &VController::gamePad()
{
	return gp;
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

uint16_t VController::buttonSize() const
{
	return btnSize;
}

float VController::buttonGCSize() const
{
	return xMMSize(buttonSize() / 100.f);
}

int VController::buttonPixelSize(const IG::Window &win) const
{
	return IG::makeEvenRoundedUp(xMMSizeToPixel(win, buttonSize() / 100.f));
}

bool VController::setButtonXPadding(std::optional<uint16_t> opt, bool placeElements)
{
	if(!opt || *opt > 1000)
		return false;
	buttonXPadding_ = *opt;
	if(placeElements)
		place();
	return true;
}

uint16_t VController::buttonXPadding() const
{
	return buttonXPadding_;
}

bool VController::setButtonYPadding(std::optional<uint16_t> opt, bool placeElements)
{
	if(!opt || *opt > 1000)
		return false;
	buttonYPadding_ = *opt;
	if(placeElements)
		place();
	return true;
}

uint16_t VController::buttonYPadding() const
{
	return buttonYPadding_;
}

bool VController::setDpadDeadzone(std::optional<uint16_t> mm100xOpt)
{
	if(!mm100xOpt || *mm100xOpt > 160)
		return false;
	dpadDzone = *mm100xOpt;
	if(hasWindow())
		gamePad().dPad().setDeadzone(renderer(), xMMSizeToPixel(window(), dpadDzone / 100.), windowData().projection.plane());
	return true;
}

uint16_t VController::dpadDeadzone() const
{
	return dpadDzone;
}

bool VController::setDpadDiagonalSensitivity(std::optional<uint16_t> opt)
{
	if(!opt || *opt < 1000 || *opt > 2500)
		return false;
	dpadDiagonalSensitivity_ = *opt;
	if(hasWindow())
		gamePad().dPad().setDiagonalSensitivity(renderer(), dpadDiagonalSensitivity_ / 1000., windowData().projection.plane());
	return true;
}

uint16_t VController::dpadDiagonalSensitivity() const
{
	return dpadDiagonalSensitivity_;
}

void VController::setTriggersInline(std::optional<bool> opt, bool placeElements)
{
	if(!opt)
		return;
	triggersInline_ = *opt;
	if(placeElements)
		place();
}

bool VController::triggersInline() const
{
	return triggersInline_;
}

void VController::setBoundingAreaVisible(std::optional<bool> opt, bool placeElements)
{
	if(!opt)
		return;
	boundingAreaVisible_ = *opt;
	if(placeElements)
		place();
}

bool VController::boundingAreaVisible() const
{
	return boundingAreaVisible_;
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

bool VController::setButtonSpacing(std::optional<uint16_t> mm100xOpt, bool placeElements)
{
	if(!mm100xOpt || *mm100xOpt > 400)
		return false;
	buttonSpacing_ = *mm100xOpt;
	if(placeElements)
		place();
	return true;
}

void VController::setDefaultButtonSpacing(uint16_t mm100x)
{
	defaultButtonSpacing_ = mm100x;
	setButtonSpacing(mm100x, false);
}

uint16_t VController::buttonSpacing() const
{
	return buttonSpacing_;
}

bool VController::setButtonStagger(std::optional<uint16_t> mm100xOpt, bool placeElements)
{
	if(!mm100xOpt || *mm100xOpt > 5)
		return false;
	buttonStagger_ = *mm100xOpt;
	if(placeElements)
		place();
	return true;
}

void VController::setDefaultButtonStagger(uint16_t mm100x)
{
	defaultButtonStagger_ = mm100x;
	buttonStagger_ = mm100x;
}

uint16_t VController::buttonStagger() const
{
	return buttonStagger_;
}

void VController::setGamepadControlsVisible(bool on)
{
	gamepadIsVisible = on;
}

bool VController::gamepadControlsVisible() const
{
	return gamepadIsVisible;
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

bool VController::readConfig(IO &io, unsigned key, unsigned size)
{
	switch(key)
	{
		default: return false;
		bcase CFGKEY_TOUCH_CONTROL_ALPHA: setButtonAlpha(readOptionValue<uint8_t>(io, size));
		bcase CFGKEY_TOUCH_CONTROL_DISPLAY: setGamepadControlsVisibility(readOptionValue<VControllerVisibility>(io, size, visibilityIsValid));
		bcase CFGKEY_TOUCH_CONTROL_SIZE: setButtonSize(readOptionValue<uint16_t>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE: setButtonSpacing(readOptionValue<uint16_t>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER: setButtonStagger(readOptionValue<uint16_t>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE: setDpadDeadzone(readOptionValue<uint16_t>(io, size));
		bcase CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS: setTriggersInline(readOptionValue<bool>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY: setDpadDiagonalSensitivity(readOptionValue<uint16_t>(io, size));
		bcase CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE: setButtonXPadding(readOptionValue<uint16_t>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE: setButtonYPadding(readOptionValue<uint16_t>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES: setBoundingAreaVisible(readOptionValue<bool>(io, size), false);
		bcase CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH: setShowOnTouchInput(readOptionValue<bool>(io, size));
		bcase CFGKEY_VCONTROLLER_LAYOUT_POS: readSerializedLayoutPositions(io, size);
	}
	return true;
}

void VController::writeConfig(IO &io) const
{
	if constexpr(VCONTROLS_GAMEPAD)
	{
		if(buttonAlpha() != DEFAULT_ALPHA)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_ALPHA, buttonAlpha());
		if(buttonSize() != defaultButtonSize)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_SIZE, buttonSize());
		if(buttonXPadding() != DEFAULT_BUTTON_EXTRA_BOUNDS_WIDTH)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE, buttonXPadding());
		if(buttonYPadding() != DEFAULT_BUTTON_EXTRA_BOUNDS_HEIGHT)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE, buttonYPadding());
		if(buttonSpacing() != defaultButtonSpacing_)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE, buttonSpacing());
		if(buttonStagger() != defaultButtonStagger_)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER, buttonStagger());
		if(dpadDeadzone() != DEFAULT_DPAD_DEADZONE)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE, dpadDeadzone());
		if(dpadDiagonalSensitivity() != DEFAULT_DPAD_DIAGONAL_SENSITIVITY)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY, dpadDiagonalSensitivity());
		if(triggersInline())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS, triggersInline());
		if(boundingAreaVisible())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES, boundingAreaVisible());
		if(!showOnTouchInput())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH, showOnTouchInput());
		if(gamepadControlsVisibility() != DEFAULT_GAMEPAD_CONTROLS_VISIBILITY)
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_DISPLAY, gamepadControlsVisibility());
		if(vibrateOnTouchInput())
			writeOptionValue(io, CFGKEY_TOUCH_CONTROL_VIRBRATE, vibrateOnTouchInput());
	}
	if(layoutPositionChanged())
	{
		logMsg("writing vcontroller positions");
		writeOptionValueHeader(io, CFGKEY_VCONTROLLER_LAYOUT_POS, serializedLayoutPositionsSize());
		for(auto &posArr : layoutPosition())
		{
			for(auto &e : posArr)
			{
				io.write(VControllerLayoutPositionSerialized{(uint8_t)e.origin, e.state, {e.pos.x, e.pos.y}});
			}
		}
	}
}

void VController::readSerializedLayoutPositions(IO &io, unsigned size)
{
	if(size < serializedLayoutPositionsSize())
	{
		logErr("expected layout position size:%u, got size:%u", serializedLayoutPositionsSize(), size);
		return;
	}
	for(auto &posArr : layoutPosition())
	{
		for(auto &e : posArr)
		{
			auto layoutPos = io.get<VControllerLayoutPositionSerialized>();
			_2DOrigin origin{layoutPos.origin};
			if(!origin.isValid())
			{
				logWarn("invalid v-controller origin from config file");
			}
			else
				e.origin = origin;
			if((int)layoutPos.state > 2)
			{
				logWarn("invalid v-controller state from config file");
			}
			else
				e.state = layoutPos.state;
			e.pos.x = layoutPos.pos[0];
			e.pos.y = layoutPos.pos[1];
			setLayoutPositionChanged();
		}
	}
}

unsigned VController::serializedLayoutPositionsSize() const
{
	unsigned positions = std::size(layoutPosition()[0]) * std::size(layoutPosition());
	return positions * sizeof(VControllerLayoutPositionSerialized);
}

void VController::configure(IG::Window &win, Gfx::Renderer &renderer, const Gfx::GlyphTextureSet &face)
{
	setWindow(win);
	setRenderer(renderer);
	setFace(face);
	auto &app = this->app();
	auto &winData = windowData();
	if constexpr(VCONTROLS_GAMEPAD)
	{
		auto &gp = gamePad();
		gp.dPad().setDeadzone(renderer, xMMSizeToPixel(win, dpadDeadzone() / 100.), winData.projection.plane());
		gp.dPad().setDiagonalSensitivity(renderer, dpadDiagonalSensitivity() / 1000., winData.projection.plane());
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
	auto initGamepadState = VCONTROLS_GAMEPAD && (Config::envIsAndroid || Config::envIsIOS || gamepadControlsVisibility() == VControllerVisibility::ON) ? VControllerState::SHOWN : VControllerState::OFF;
	bool isLandscape = true;
	for(auto &e : layoutPosition())
	{
		auto defaultSidePadding = xMMSizeToPixel(win, 4.);
		int xOffset = isLandscape ? xMMSizeToPixel(win, 2.) : xMMSizeToPixel(win, .5);
		if constexpr(VCONTROLS_GAMEPAD)
		{
			e[VCTRL_LAYOUT_DPAD_IDX] = {LB2DO, {xOffset + bounds(0).xSize()/2, (int)(-buttonPixelSize(win)) - bounds(0).ySize()/2}, initGamepadState};
			e[VCTRL_LAYOUT_CENTER_BTN_IDX] = {CB2DO, {0, 0}, initGamepadState};
			e[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX] = {RB2DO, {-xOffset - bounds(2).xSize()/2, (int)(-buttonPixelSize(win)) - bounds(2).ySize()/2}, initGamepadState};
		}
		e[VCTRL_LAYOUT_MENU_IDX] = {RT2DO, {-defaultSidePadding, 0}, initMenuState};
		e[VCTRL_LAYOUT_FF_IDX] = {LT2DO, {defaultSidePadding, 0}, initFastForwardState};
		if(VCONTROLS_GAMEPAD && EmuSystem::inputHasTriggers())
		{
			int y = std::min(e[0].pos.y - bounds(0).ySize()/2, e[2].pos.y - bounds(2).ySize()/2);
			y -= bounds(5).ySize()/2 + yMMSizeToPixel(win, 1.);
			e[VCTRL_LAYOUT_L_IDX] = {LB2DO, {xOffset + bounds(5).xSize()/2, y}, initGamepadState};
			e[VCTRL_LAYOUT_R_IDX] = {RB2DO, {-xOffset - bounds(5).xSize()/2, y}, initGamepadState};
		}
		isLandscape = false;
	};
	setLayoutPositionChanged(false);
}

void VController::resetOptions()
{
	resetPositions();
	buttonSpacing_ = defaultButtonSpacing_;
	buttonStagger_ = defaultButtonStagger_;
}

void VController::resetAllOptions()
{
	gamepadControlsVisibility_ = DEFAULT_GAMEPAD_CONTROLS_VISIBILITY;
	btnSize = defaultButtonSize;
	dpadDzone = DEFAULT_DPAD_DEADZONE;
	dpadDiagonalSensitivity_ = DEFAULT_DPAD_DIAGONAL_SENSITIVITY;
	buttonXPadding_ = DEFAULT_BUTTON_EXTRA_BOUNDS_WIDTH;
	buttonYPadding_ = DEFAULT_BUTTON_EXTRA_BOUNDS_HEIGHT;
	triggersInline_ = false;
	boundingAreaVisible_ = false;
	vibrateOnTouchInput_ = false;
	showOnTouchInput_ = true;
	resetOptions();
	setButtonAlpha(DEFAULT_ALPHA);
	updateAutoOnScreenControlVisible();
	setInputPlayer(0);
}

VControllerLayoutPosition VController::pixelToLayoutPos(IG::WP pos, IG::WP size, IG::WindowRect viewBounds)
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
	return {origin, {x, y}};
}

IG::WP VController::layoutToPixelPos(VControllerLayoutPosition lPos, Gfx::Viewport viewport)
{
	int x = (lPos.origin.xScaler() == 0) ? lPos.pos.x + viewport.width()/2 :
		(lPos.origin.xScaler() == 1) ? lPos.pos.x + viewport.width() : lPos.pos.x;
	int y = lPos.origin.adjustY(lPos.pos.y, (int)viewport.height(), LT2DO);
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

}
