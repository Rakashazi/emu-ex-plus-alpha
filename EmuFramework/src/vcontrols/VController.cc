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

#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/AppKeyCode.hh>
#include <emuframework/EmuOptions.hh>
#include "../WindowData.hh"
#include <emuframework/Option.hh>
#include <imagine/util/math.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/base/Window.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"VController"};
constexpr uint8_t defaultAlpha = 255. * .5;

VController::VController(IG::ApplicationContext ctx):
	appCtx{ctx},
	alphaF{defaultAlpha / 255.},
	defaultButtonSize
	{
		#ifdef CONFIG_OS_IOS
		int16_t(ctx.deviceIsIPad() ? 1400 : 850)
		#else
		850
		#endif
	},
	btnSize{defaultButtonSize},
	alpha{defaultAlpha}
{
	// set dummy buttons to indicate uninitialized state
	gpElements.emplace_back(std::in_place_type<VControllerButtonGroup>);
	uiElements.emplace_back(std::in_place_type<VControllerUIButtonGroup>);
}

int VController::xMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.widthMMInPixels(mm);
}

int VController::yMMSizeToPixel(const IG::Window &win, float mm) const
{
	return win.heightMMInPixels(mm);
}

static void updateTexture(const EmuApp &app, VControllerElement &e, Gfx::RendererTask &task, const Gfx::IndexBuffer<uint8_t> &fanQuadIdxs)
{
	e.visit(overloaded
	{
		[&](VControllerDPad &dpad){ dpad.setImage(task, app.asset(app.vControllerAssetDesc(0)), fanQuadIdxs); },
		[&](VControllerButtonGroup &grp)
		{
			for(auto &btn : grp.buttons)
			{
				auto desc = app.vControllerAssetDesc(btn.key);
				btn.setImage(app.asset(desc), desc.aspectRatio.y);
			}
			grp.setTask(task);
		},
		[&](VControllerUIButtonGroup &grp)
		{
			for(auto &btn : grp.buttons)
			{
				btn.setImage([&]
				{
					using enum AppKeyCode;
					switch(AppKeyCode(btn.key.codes[0]))
					{
						case openMenu: return app.asset(AssetID::more);
						case openContent: return app.asset(AssetID::openFile);
						case closeContent: return app.asset(AssetID::close);
						case saveState: return app.asset(AssetID::save);
						case loadState: return app.asset(AssetID::load);
						case decStateSlot: return app.asset(AssetID::leftSwitch);
						case incStateSlot: return app.asset(AssetID::rightSwitch);
						case fastForward:
						case toggleFastForward: return app.asset(AssetID::fast);
						case takeScreenshot: return app.asset(AssetID::screenshot);
						case openSystemActions: return app.asset(AssetID::menu);
						case turboModifier: return app.asset(AssetID::speed);
						case exitApp: return app.asset(AssetID::close);
						case slowMotion:
						case toggleSlowMotion: return app.asset(AssetID::slow);
						case rewind: return app.asset(AssetID::rewind);
						case softReset:
						case hardReset:
						case resetMenu: return app.asset(AssetID::arrow);
					}
					return app.asset(AssetID::more);
				}());
			}
			grp.setTask(task);
		}
	});
}

void VController::updateTextures()
{
	for(auto &e : gpElements) { updateTexture(app(), e, renderer().mainTask, fanQuadIdxs); }
	for(auto &e : uiElements) { updateTexture(app(), e, renderer().mainTask, fanQuadIdxs); }
}

static void setSize(VControllerElement &elem, int sizePx, Gfx::Renderer &r)
{
	assert(sizePx);
	elem.visit(overloaded
	{
		[&](VControllerDPad &dpad){ dpad.setSize(r, makeEvenRoundedUp(int(sizePx * 2.5f))); },
		[&](VControllerButtonGroup &grp){ grp.setButtonSize(sizePx); },
		[&](VControllerUIButtonGroup &grp){ grp.setButtonSize(sizePx); },
	});
}

void VController::setButtonSizes(int gamepadBtnSizeInPixels, int uiBtnSizeInPixels)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(gamepadBtnSizeInPixels, gamepadBtnSizeInPixels * .75f, layoutBounds());
	for(auto &elem : gpElements) { setSize(elem, gamepadBtnSizeInPixels, renderer()); }
	for(auto &elem : uiElements) { setSize(elem, uiBtnSizeInPixels, renderer()); }
}

void VController::applyButtonSize()
{
	setButtonSizes(emulatedDeviceButtonPixelSize(), uiButtonPixelSize());
}

void VController::resetInput()
{
	for(auto &e : dragTracker.stateList())
	{
		for(auto &vBtn : e.data)
		{
			if(vBtn) // release old key, if any
				app().handleSystemKeyInput(vBtn, Input::Action::RELEASED);
		}
	}
	dragTracker.reset();
}

void VController::updateAltSpeedModeInput(AltSpeedMode mode, bool on)
{
	for(auto &e : uiElements)
	{
		for(auto &b : e.buttons())
		{
			if(b.key.codes[0] == KeyCode(AppKeyCode::fastForward) || b.key.codes[0] == KeyCode(AppKeyCode::toggleFastForward))
			{
				b.updateColor(on && mode == AltSpeedMode::fast ? Gfx::Color{Gfx::ColorName::RED} : Gfx::Color{}, alphaF, e);
			}
			else if(b.key.codes[0] == KeyCode(AppKeyCode::slowMotion) || b.key.codes[0] == KeyCode(AppKeyCode::toggleSlowMotion))
			{
				b.updateColor(on && mode == AltSpeedMode::slow ? Gfx::Color{Gfx::ColorName::RED} : Gfx::Color{}, alphaF, e);
			}
		}
	}
}

void VController::place()
{
	if(!hasWindow())
		return;
	auto &win = window();
	applyButtonSize();
	auto bounds = layoutBounds();
	auto windowBounds = win.bounds();
	bool isPortrait = win.isPortrait();
	for(auto &elem : gpElements)
	{
		elem.place(bounds, windowBounds, isPortrait);
	}
	for(auto &elem : uiElements)
	{
		elem.place(bounds, windowBounds, isPortrait);
	}
	dragTracker.setDragStartPixels(win.widthMMInPixels(1.));
}

void VController::toggleKeyboard()
{
	log.info("toggling keyboard");
	resetInput();
	kbMode ^= true;
	system().onVKeyboardShown(kb, kbMode);
}

std::array<KeyInfo, 2> VController::findGamepadElements(WPt pos)
{
	for(const auto &gpElem : gpElements)
	{
		auto indices = gpElem.visit(overloaded
		{
			[&](const VControllerDPad &dpad) -> std::array<KeyInfo, 2>
			{
				if(!gamepadDPadIsEnabled() || gpElem.state == VControllerState::OFF)
					return {};
				return dpad.getInput(pos);
			},
			[&](const VControllerButtonGroup &grp) -> std::array<KeyInfo, 2>
			{
				if(!gamepadButtonsAreEnabled() || gpElem.state == VControllerState::OFF)
					return {};
				return grp.findButtonIndices(pos);
			},
			[](auto&) -> std::array<KeyInfo, 2> { return {}; }
		});
		if(indices != std::array<KeyInfo, 2>{})
			return indices;
	}
	return {};
}

KeyInfo VController::keyboardKeyFromPointer(const Input::MotionEvent &e)
{
	assert(isInKeyboardMode());
	assumeExpr(e.isPointer());
	if(e.pushed())
	{
		kb.unselectKey();
	}
	int kbIdx = kb.getInput(e.pos());
	if(kbIdx == -1)
		return {};
	if(kb.translateInput(kbIdx) == TOGGLE_KEYBOARD)
	{
		if(!e.pushed())
			return {};
		log.info("dismiss kb");
		toggleKeyboard();
	}
	else if(kb.translateInput(kbIdx) == CHANGE_KEYBOARD_MODE)
	{
		if(!e.pushed())
			return {};
		log.info("switch kb mode");
		kb.cycleMode(system());
		resetInput();
	}
	else
		 return kb.translateInput(kbIdx);
	return {};
}

bool VController::pointerInputEvent(const Input::MotionEvent &e, IG::WindowRect gameRect)
{
	assumeExpr(e.isPointer());
	if(e.pushed() || e.released())
	{
		for(const auto &grp: uiElements)
		{
			if(grp.state == VControllerState::OFF)
				continue;
			for(const auto &btn: grp.uiButtonGroup()->buttons)
			{
				if(btn.bounds().overlaps(e.pos()))
				{
					app().handleKeyInput(btn.key, e);
					return true;
				}
			}
		}
	}
	static constexpr std::array<KeyInfo, 2> nullElems{};
	auto newElems = nullElems;
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
	auto &app = this->app();
	auto &system = this->system();
	auto applyInputActions =
		[&](std::array<KeyInfo, 2> prevElements, std::array<KeyInfo, 2> currElements)
		{
			// release old buttons
			for(auto vBtn : prevElements)
			{
				if(vBtn && !std::ranges::contains(currElements, vBtn))
				{
					//log.info("releasing {}", vBtn[0]);
					app.handleSystemKeyInput(vBtn, Input::Action::RELEASED);
				}
			}
			// push new buttons
			for(auto vBtn : currElements)
			{
				if(vBtn && !std::ranges::contains(prevElements, vBtn))
				{
					//log.info("pushing {}", vBtn[0]);
					app.handleSystemKeyInput(vBtn, Input::Action::PUSHED);
					if(vibrateOnTouchInput())
					{
						app.vibrationManager.vibrate(IG::Milliseconds{32});
					}
				}
			}
		};
	dragTracker.inputEvent(e,
		[&](Input::DragTrackerState dragState, auto &currElems)
		{
			currElems = newElems;
			applyInputActions(nullElems, newElems);
			if(!elementsArePushed)
			{
				elementsArePushed |= system.onPointerInputStart(e, dragState, gameRect);
			}
		},
		[&](Input::DragTrackerState dragState, Input::DragTrackerState prevDragState, auto &currElems)
		{
			auto prevElems = std::exchange(currElems, newElems);
			applyInputActions(prevElems, newElems);
			if(!elementsArePushed)
			{
				elementsArePushed |= system.onPointerInputUpdate(e, dragState, prevDragState, gameRect);
			}
		},
		[&](Input::DragTrackerState dragState, auto &currElems)
		{
			applyInputActions(currElems, nullElems);
			elementsArePushed |= system.onPointerInputEnd(e, dragState, gameRect);
		});
	 if(!elementsArePushed && !gamepadControlsVisible() && shouldShowOnTouchInput()
			&& !isInKeyboardMode() && e.isTouch() && e.pushed()) [[unlikely]]
		{
			log.info("turning on on-screen controls from touch input");
			setGamepadControlsVisible(true);
			app.viewController().placeEmuViews();
		}
	return elementsArePushed;
}

bool VController::keyInput(const Input::KeyEvent &e)
{
	if(!isInKeyboardMode())
		return false;
	return kb.keyInput(*this, e);
}

void VController::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden)
{
	if(alpha == 0.f) [[unlikely]]
		return;
	cmds.set(Gfx::BlendMode::PREMULT_ALPHA);
	if(isInKeyboardMode())
	{
		cmds.setColor(Gfx::Color{alphaF});
		kb.draw(cmds);
	}
	else if(gamepadIsVisible || showHidden)
	{
		auto elementIsEnabled = [&](const VControllerElement &e)
		{
			return !((e.buttonGroup() && gamepadDisabledFlags.buttons) ||
				(e.dPad() && gamepadDisabledFlags.dpad));
		};
		auto activeElements = gpElements | std::views::filter(elementIsEnabled);
		if(!activeElements.empty())
		{
			for(const auto &e : activeElements) { e.drawBounds(cmds, showHidden); }
			cmds.basicEffect().enableTexture(cmds, gamepadTex);
			for(const auto &e : activeElements)
			{
				e.drawButtons(cmds, showHidden);
			}
		}
	}
	if(uiElements.size())
	{
		cmds.basicEffect().enableTexture(cmds, uiTex);
		for(auto &e : uiElements)
		{
			e.drawButtons(cmds, showHidden);
		}
	}
}

void VController::draw(Gfx::RendererCommands &__restrict__ cmds, const VControllerElement &elem, bool showHidden) const
{
	cmds.set(Gfx::BlendMode::PREMULT_ALPHA);
	elem.drawBounds(cmds, showHidden);
	cmds.basicEffect().enableTexture(cmds, elem.uiButtonGroup() ? uiTex : gamepadTex);
	elem.drawButtons(cmds, showHidden);
}

bool VController::isInKeyboardMode() const
{
	return EmuSystem::inputHasKeyboard && kbMode;
}

void VController::setInputPlayer(int8_t player)
{
	inputPlayer_ = player;
	for(auto &e : gpElements)
	{
		e.transposeKeysForPlayer(app().inputManager, player);
	}
}

bool VController::keyIsEnabled(KeyInfo k) const
{
	if(!disabledKeys.size())
		return true;
	return !std::ranges::contains(disabledKeys, k.codes[0])
		&& !std::ranges::contains(disabledKeys, k.codes[1])
		&& !std::ranges::contains(disabledKeys, k.codes[2]);
}

void VController::setDisabledInputKeys(std::span<const KeyCode> disabledKeys_)
{
	disabledKeys = disabledKeys_;
	for(auto &e : gpElements)
	{
		e.visit(overloaded
		{
			[&](VControllerButtonGroup &grp) { updateEnabledButtons(grp); },
			[](auto&){}
		});
	}
	place();
}

void VController::updateEnabledButtons(VControllerButtonGroup &grp) const
{
	for(auto &btn : grp.buttons)
		btn.enabled = keyIsEnabled(btn.key);
}

void VController::updateKeyboardMapping()
{
	kb.updateKeyboardMapping(system());
}

void VController::setKeyboardImage(Gfx::TextureSpan img)
{
	kb.setImg(renderer().mainTask, img);
}

void VController::setButtonAlpha(std::optional<uint8_t> opt)
{
	if(!opt)
		return;
	alpha = *opt;
	alphaF = *opt / 255.f;
	applyButtonAlpha(alphaF);
}

void VController::applySavedButtonAlpha()
{
	applyButtonAlpha(alphaF);
}

void VController::applyButtonAlpha(float alpha)
{
	for(auto &e : gpElements) { e.setAlpha(alpha); }
	for(auto &e : uiElements) { e.setAlpha(alpha); }
}

void VController::setWindow(const IG::Window &win_)
{
	win = &win_;
	winData = &EmuEx::windowData(win_);
}

bool VController::setButtonSize(int16_t mm100xOpt, bool placeElements)
{
	if(mm100xOpt < 300 || mm100xOpt > 3000)
		return false;
	btnSize = mm100xOpt;
	if(placeElements)
		place();
	return true;
}

int VController::emulatedDeviceButtonPixelSize() const
{
	return IG::makeEvenRoundedUp(xMMSizeToPixel(window(), buttonSize() / 100.f));
}

int VController::uiButtonPixelSize() const
{
	return View::navBarHeight(face());
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
	if(!opt || !app.vibrationManager.hasVibrator())
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
		log.info("Physical controls present:{}", present ? "y" : "n");
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
			log.info("auto-turning off on-screen controls");
			gamepadIsVisible = false;
			return true;
		}
		else if(!gamepadIsVisible && !physicalControlsPresent)
		{
			log.info("auto-turning on on-screen controls");
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

static bool readVControllerElement(InputManager &mgr, MapIO &io, std::vector<VControllerElement> &elems, bool readingUIElems)
{
	auto elemType = io.get<uint8_t>();
	if(elemType == 0)
	{
		if(readingUIElems)
		{
			VControllerUIButtonGroup::Config config;
			io.read(config.layout.rowItems);
			config.layout.origin = _2DOrigin::unpack(io.get<_2DOrigin::PackedType>());
			readSizedData<uint8_t>(io, config.keys);
			config.validate(mgr);
			elems.emplace_back(std::in_place_type<VControllerUIButtonGroup>, std::move(config));
		}
		else
		{
			VControllerButtonGroup::Config config;
			io.read(config.layout.rowItems);
			io.read(config.layout.spacingMM);
			io.read(config.layout.xPadding);
			io.read(config.layout.yPadding);
			io.read(config.layout.staggerType);
			config.layout.origin = _2DOrigin::unpack(io.get<_2DOrigin::PackedType>());
			io.read(config.layout.showBoundingArea);
			readSizedData<uint8_t>(io, config.keys);
			config.validate(mgr);
			elems.emplace_back(std::in_place_type<VControllerButtonGroup>, std::move(config));
		}
	}
	else if(elemType == 1 && !readingUIElems)
	{
		VControllerDPad::Config config;
		io.read(config.keys);
		io.read(config.diagonalSensitivity);
		io.read(config.deadzoneMM100x);
		io.read(config.visualizeBounds);
		config.validate(mgr);
		elems.emplace_back(std::in_place_type<VControllerDPad>, std::move(config));
	}
	else
	{
		log.error("bad VControllerElement type from config");
		elems.clear();
		if(readingUIElems)
			elems.emplace_back(std::in_place_type<VControllerUIButtonGroup>);
		else
			elems.emplace_back(std::in_place_type<VControllerButtonGroup>);
		return false;
	}
	auto &elem = elems.back();
	io.read(elem.layoutPos[0].pos);
	elem.layoutPos[0].origin = _2DOrigin::unpack(io.get<_2DOrigin::PackedType>());
	io.read(elem.layoutPos[1].pos);
	elem.layoutPos[1].origin = _2DOrigin::unpack(io.get<_2DOrigin::PackedType>());
	if(auto state = io.get<VControllerState>(); state <= VControllerState::HIDDEN) { elem.state = state; }
	return true;
}

bool VController::readConfig(EmuApp &app, MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_TOUCH_CONTROL_ALPHA:
			setButtonAlpha(readOptionValue<uint8_t>(io));
			return true;
		case CFGKEY_TOUCH_CONTROL_DISPLAY:
			setGamepadControlsVisibility(readOptionValue<VControllerVisibility>(io, visibilityIsValid));
			return true;
		case CFGKEY_TOUCH_CONTROL_SIZE:
			return readOptionValue<int16_t>(io, [&](auto val){setButtonSize(val, false);});
		case CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH:
			setShowOnTouchInput(readOptionValue<bool>(io));
			return true;
		case CFGKEY_TOUCH_CONTROL_VIRBRATE:
			setVibrateOnTouchInput(app, readOptionValue<bool>(io));
			return true;
		case CFGKEY_VCONTROLLER_ALLOW_PAST_CONTENT_BOUNDS: return readOptionValue(io, allowButtonsPastContentBounds_);
		case CFGKEY_VCONTROLLER_HIGHLIGHT_PUSHED_BUTTONS: return readOptionValue(io, highlightPushedButtons);
		case CFGKEY_VCONTROLLER_DEVICE_BUTTONS_V2:
		{
			gpElements.clear();
			[[maybe_unused]] auto emuDeviceId = io.get<uint8_t>(); // reserved for future use
			[[maybe_unused]] auto configId = io.get<uint8_t>(); // reserved for future use
			auto elements = io.get<uint8_t>();
			log.info("read emu device button data ({} bytes) with {} element(s)", io.size(), elements);
			for([[maybe_unused]] auto i : iotaCount(elements))
			{
				if(!readVControllerElement(app.inputManager, io, gpElements, false))
					return false;
			}
			return true;
		}
		case CFGKEY_VCONTROLLER_UI_BUTTONS_V2:
		{
			uiElements.clear();
			[[maybe_unused]] auto configId = io.get<uint8_t>(); // reserved for future use
			auto elements = io.get<uint8_t>();
			log.info("read UI button data ({} bytes) with {} element(s)", io.size(), elements);
			for([[maybe_unused]] auto i : iotaCount(elements))
			{
				if(!readVControllerElement(app.inputManager, io, uiElements, true))
					return false;
			}
			return true;
		}
	}
}

static void writeToConfig(const VControllerElement &e, FileIO &io)
{
	io.put(e.dPad() ? int8_t(1) : int8_t(0));
	e.visit(overloaded
	{
		[&](const VControllerButtonGroup &e)
		{
			auto config = e.config();
			io.put(config.layout.rowItems);
			io.put(config.layout.spacingMM);
			io.put(config.layout.xPadding);
			io.put(config.layout.yPadding);
			io.put(config.layout.staggerType);
			io.put(config.layout.origin.pack());
			io.put(config.layout.showBoundingArea);
			writeSizedData<uint8_t>(io, config.keys);
		},
		[&](const VControllerUIButtonGroup &e)
		{
			auto config = e.config();
			io.put(config.layout.rowItems);
			io.put(config.layout.origin.pack());
			writeSizedData<uint8_t>(io, config.keys);
		},
		[&](const VControllerDPad &e)
		{
			auto config = e.config;
			io.put(config.keys);
			io.put(config.diagonalSensitivity);
			io.put(config.deadzoneMM100x);
			io.put(config.visualizeBounds);
		},
	});
	io.put(e.layoutPos[0].pos);
	io.put(e.layoutPos[0].origin.pack());
	io.put(e.layoutPos[1].pos);
	io.put(e.layoutPos[1].origin.pack());
	io.put(e.state);
}

static size_t configDataSizeBytes(const std::vector<VControllerElement> &elems, bool savingUIElems)
{
	size_t bytes = 2; // config key size
	if(!savingUIElems)
		bytes++; // emulated device index
	bytes++; // config index
	bytes++; // element count
	for(const auto &e : elems)
	{
		bytes++; // element type
		bytes += e.configSize();
	}
	return bytes;
}

void VController::writeDeviceButtonsConfig(FileIO &io) const
{
	auto bytes = configDataSizeBytes(gpElements, false);
	if(bytes > std::numeric_limits<uint16_t>::max())
	{
		log.error("device button data bytes:{} too large, skipped writing to config", bytes);
		return;
	}
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_VCONTROLLER_DEVICE_BUTTONS_V2));
	io.put(int8_t(0));
	io.put(int8_t(0));
	io.put(uint8_t(std::min(gpElements.size(), 255zu)));
	log.info("wrote emu device button data ({} bytes) with {} element(s)", bytes, gpElements.size());
	for(const auto &e : gpElements) { writeToConfig(e, io); }
}

void VController::writeUIButtonsConfig(FileIO &io) const
{
	auto bytes = configDataSizeBytes(uiElements, true);
	if(bytes > std::numeric_limits<uint16_t>::max())
	{
		log.error("UI button data bytes:{} too large, skipped writing to config", bytes);
		return;
	}
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_VCONTROLLER_UI_BUTTONS_V2));
	io.put(int8_t(0));
	io.put(uint8_t(std::min(uiElements.size(), 255zu)));
	log.info("wrote UI button data ({} bytes) with {} element(s)", bytes, uiElements.size());
	for(const auto &e : uiElements) { writeToConfig(e, io); }
}

void VController::writeConfig(FileIO &io) const
{
	if(buttonAlpha() != defaultAlpha)
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
	writeOptionValueIfNotDefault(io, CFGKEY_VCONTROLLER_HIGHLIGHT_PUSHED_BUTTONS, highlightPushedButtons, true);
	writeDeviceButtonsConfig(io);
	writeUIButtonsConfig(io);
}

void VController::configure(IG::Window &win, Gfx::Renderer &renderer, const Gfx::GlyphTextureSet &face)
{
	setWindow(win);
	setRenderer(renderer);
	setFace(face);
	fanQuadIdxs = {renderer.mainTask, {.size = 24}}; // for rendering DPads with FanQuads
	{
		auto indices = fanQuadIdxs.map();
		for(auto i : iotaCount(2))
		{
			std::ranges::copy(Gfx::mapFanQuadIndices(i), indices.begin() + (i * 12));
		}
	}
	gamepadTex = app().asset(AssetID::gamepadOverlay);
	uiTex = app().asset(AssetID::more);
	if(uiElements.size() && uiElements[0].layoutPos[0].pos.x == -1)
	{
		resetUIGroups();
	}
	else
	{
		for(auto &e : uiElements) { update(e); };
	}
	if(gpElements.size() && gpElements[0].layoutPos[0].pos.x == -1)
	{
		resetEmulatedDeviceGroups();
	}
	else
	{
		for(auto &e : gpElements)
		{
			update(e);
			if(e.dPad()) e.dPad()->updateBoundingAreaGfx(renderer);
		};
	}
	setInputPlayer(0);
}

void VController::resetEmulatedDevicePositions(std::vector<VControllerElement> &gpElements) const
{
	auto &win = window();
	log.info("resetting emulated device controls to default positions");
	const int shortSidePadding = xMMSizeToPixel(win, 1);
	const int longSidePadding = xMMSizeToPixel(win, 3);
	const int yBottomPadding = xMMSizeToPixel(win, 3);
	const int yBottom = (win.bounds().y2 - win.contentBounds().y2) + yBottomPadding;
	VControllerElement *prevElem{};
	for(int leftY{yBottom}, centerY{}, rightY{yBottom}; auto &e : gpElements)
	{
		const auto halfSize = e.realBounds().size() / 2;
		if(e.layoutOrigin() == LB2DO || e.layoutOrigin() == RB2DO)
		{
			auto &yOffset = e.layoutOrigin() == RB2DO ? rightY : leftY;
			auto xOffset = e.layoutOrigin() == RB2DO ? -longSidePadding - halfSize.x : longSidePadding + halfSize.x;
			auto xOffsetPortrait = e.layoutOrigin() == RB2DO ? -shortSidePadding - halfSize.x : shortSidePadding + halfSize.x;
			int yAdvance = e.realBounds().ySize();
			if(prevElem && prevElem->layoutOrigin() != e.layoutOrigin()) // line up elements
			{
				const auto prevHalfSize = prevElem->realBounds().size() / 2;
				auto prevElemYOffset = -prevElem->layoutPos[0].pos.y - prevHalfSize.y + (prevHalfSize.y - halfSize.y);
				if(prevElemYOffset > yOffset)
				{
					yOffset = prevElemYOffset;
					yAdvance = prevElem->realBounds().ySize() - (prevHalfSize.y - halfSize.y);
				}
			}
			e.layoutPos[0] = {e.layoutOrigin(), {xOffset, -yOffset - halfSize.y}};
			e.layoutPos[1] = {e.layoutOrigin(), {xOffsetPortrait, -yOffset - halfSize.y - yBottomPadding}};
			yOffset += yAdvance;
			prevElem = &e;
		}
		else if(e.layoutOrigin() == CB2DO)
		{
			e.layoutPos[0] = e.layoutPos[1] = {CB2DO, {0, -centerY - halfSize.y}};
			centerY += e.realBounds().ySize();
		}
	}
}

void VController::resetEmulatedDevicePositions() { resetEmulatedDevicePositions(gpElements); }

void VController::resetEmulatedDeviceGroups()
{
	log.info("setting default emu device button groups");
	gpElements = defaultEmulatedDeviceGroups();
}

std::vector<VControllerElement> VController::defaultEmulatedDeviceGroups() const
{
	std::vector<VControllerElement> gpElements;
	for(const auto &c : system().inputDeviceDesc(0).components)
	{
		if(!c.flags.altConfig)
			add(gpElements, c);
	}
	if(hasWindow())
		resetEmulatedDevicePositions(gpElements);
	return gpElements;
}

void VController::resetUIPositions(std::vector<VControllerElement> &uiElements) const
{
	auto &win = window();
	log.info("resetting UI controls to default positions");
	const auto leftSidePadding = xMMSizeToPixel(win, 4); // slightly more padding on left side to account for FF button on round displays
	const auto rightSidePadding = -xMMSizeToPixel(win, 2);
	const int yTop = win.bounds().y;
	for(int leftY{yTop}, rightY{yTop}; auto &e : uiElements)
	{
		const auto halfSize = e.realBounds().size() / 2;
		if(e.layoutOrigin() == RT2DO || e.layoutOrigin() == LT2DO)
		{
			auto &yOffset = e.layoutOrigin() == RT2DO ? rightY : leftY;
			auto xOffset = e.layoutOrigin() == RT2DO ? rightSidePadding : leftSidePadding;
			e.layoutPos[0] = e.layoutPos[1] = {e.layoutOrigin(), {xOffset, yOffset + halfSize.y}};
			yOffset += e.realBounds().ySize();
		}
	}
}

void VController::resetUIPositions() { resetUIPositions(uiElements); }

void VController::resetUIGroups()
{
	log.info("setting default UI button groups");
	uiElements = defaultUIGroups();
}

std::vector<VControllerElement> VController::defaultUIGroups() const
{
	std::vector<VControllerElement> uiElements;
	add(uiElements, rightUIComponents);
	if(Config::Input::TOUCH_DEVICES)
		add(uiElements, leftUIComponents);
	if(hasWindow())
		resetUIPositions(uiElements);
	return uiElements;
}

VControllerLayoutPosition VControllerLayoutPosition::fromPixelPos(WPt pos, WSize size, IG::WindowRect windowBounds)
{
	IG::WindowRect bound {pos - size/2, pos + size/2};

	const auto &rect = windowBounds;
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

WPt VControllerLayoutPosition::toPixelPos(WRect windowBounds) const
{
	int x = (origin.xScaler() == 0) ? pos.x + windowBounds.xSize() / 2 :
		(origin.xScaler() == 1) ? pos.x + windowBounds.xSize() : pos.x;
	int y = origin.adjustY(int(pos.y), windowBounds.ySize(), LT2DO);
	return {x, y};
}

bool VController::gamepadIsActive() const
{
	return gamepadIsEnabled() && gamepadControlsVisible();
}

static int8_t rowSize(InputComponentDesc c)
{
	auto size = c.flags.rowSize;
	if(size)
		return size;
	else
		return c.keyCodes.size() >= 6 ? 3 : 2;
}

VControllerElement &VController::add(std::vector<VControllerElement> &elems, InputComponentDesc c) const
{
	auto &elem = [&]() -> VControllerElement&
	{
		switch(c.type)
		{
			case InputComponent::ui:
				return elems.emplace_back(std::in_place_type<VControllerUIButtonGroup>, c.keyCodes, c.layoutOrigin);
			case InputComponent::dPad:
				assert(c.keyCodes.size() == 4);
				return elems.emplace_back(std::in_place_type<VControllerDPad>, std::span<const KeyInfo, 4>{c.keyCodes.data(), 4});
			case InputComponent::button:
			case InputComponent::trigger:
			{
				auto &e = elems.emplace_back(std::in_place_type<VControllerButtonGroup>, c.keyCodes, c.layoutOrigin, rowSize(c));
				auto &grp = *e.buttonGroup();
				updateEnabledButtons(grp);
				if(c.flags.staggeredLayout)
					grp.setStaggerType(5);
				return e;
			}
		}
		bug_unreachable("invalid InputComponent");
	}();
	if(hasWindow())
	{
		auto layoutPos = VControllerLayoutPosition::fromPixelPos(layoutBounds().center(), elem.bounds().size(), window().bounds());
		elem.layoutPos[0] = elem.layoutPos[1] = layoutPos;
		update(elem);
	}
	return elem;
}

VControllerElement &VController::add(InputComponentDesc c)
{
	return add(c.type == InputComponent::ui ? uiElements : gpElements, c);
}

void VController::update(VControllerElement &elem) const
{
	if(!hasWindow())
		return;
	updateTexture(app(), elem, renderer_->mainTask, fanQuadIdxs);
	setSize(elem, elem.uiButtonGroup() ? uiButtonPixelSize() : emulatedDeviceButtonPixelSize(), *renderer_);
	elem.updateMeasurements(window());
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

void VController::updateSystemKeys(KeyInfo key, bool isPushed)
{
	if(!highlightPushedButtons)
		return;
	auto stripFlags = [](KeyInfo k)
	{
		k.flags.turbo = 0;
		k.flags.toggle = 0;
		return k;
	};
	for(auto &e : gpElements)
	{
		e.visit(overloaded
		{
			[&](VControllerButtonGroup &grp)
			{
				for(auto &btn : grp.buttons)
				{
					if(stripFlags(btn.key) == key)
					{
						btn.isHighlighted = isPushed;
						btn.setAlpha(alphaF, grp);
					}
				}
			},
			[&](VControllerDPad &dpad)
			{
				bool didUpdate{};
				for(auto &&[i, k] : enumerate(dpad.config.keys))
				{
					if(stripFlags(k) == key)
					{
						dpad.isHighlighted[i] = isPushed;
						didUpdate = true;
					}
				}
				if(didUpdate)
					dpad.setAlpha(alphaF);
			},
			[](auto&){}
		});
	}
}

void VController::resetHighlightedKeys()
{
	for(auto &e : gpElements)
	{
		e.visit(overloaded
		{
			[&](VControllerButtonGroup &grp)
			{
				for(auto &btn : grp.buttons)
				{
					if(btn.isHighlighted)
					{
						btn.isHighlighted = false;
						btn.setAlpha(alphaF, grp);
					}
				}
			},
			[](auto&){}
		});
	}
}

}
