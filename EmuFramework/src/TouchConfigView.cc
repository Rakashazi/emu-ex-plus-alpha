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

#include <emuframework/TouchConfigView.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/Timer.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include <utility>
#include "WindowData.hh"
#include "privateInput.hh"

namespace EmuEx
{

constexpr bool CAN_TURN_OFF_MENU_BTN = !Config::envIsIOS;

constexpr const char *ctrlStateStr[]
{
	"Off", "On", "Hidden"
};

constexpr const char *touchCtrlExtraBtnSizeMenuName[4]
{
	"None", "Gap only", "10%", "25%"
};

constexpr int touchCtrlExtraBtnSizeMenuVal[4]
{
	0, 1, 200, 500
};

class OnScreenInputPlaceView final: public View, public EmuAppHelper<OnScreenInputPlaceView>
{
public:
	OnScreenInputPlaceView(ViewAttachParams attach, VController &vController);
	~OnScreenInputPlaceView() final;
	void place() final;
	bool inputEvent(const Input::Event &e) final;
	void draw(Gfx::RendererCommands &__restrict__ cmds) final;

private:
	struct DragData
	{
		VControllerElement *elem{};
		IG::WP startPos{};
	};
	Gfx::Text text{};
	VController *vControllerPtr;
	IG::InterpolatorValue<float, IG::FrameTime, IG::InterpolatorType::LINEAR> textFade{};
	IG::Timer animationStartTimer{"OnScreenInputPlaceView::animationStartTimer"};
	IG::OnFrameDelegate animate{};
	IG::WindowRect exitBtnRect{};
	Input::DragTracker<DragData> dragTracker{};

	VController &vController() { return *vControllerPtr; }
};

OnScreenInputPlaceView::OnScreenInputPlaceView(ViewAttachParams attach, VController &vController):
	View(attach),
	text{"Click center to go back", &defaultFace()},
	vControllerPtr{&vController},
	animate
	{
		[this](IG::FrameParams params)
		{
			window().setNeedsDraw(true);
			//logMsg("updating fade");
			return textFade.update(params.timestamp());
		}
	}
{
	app().applyOSNavStyle(appContext(), true);
	textFade = {1.};
	animationStartTimer.runIn(IG::Seconds{2}, {},
		[this]()
		{
			logMsg("starting fade");
			textFade = {1., 0., {}, IG::steadyClockTimestamp(), IG::Milliseconds{400}};
			window().addOnFrame(animate);
		});
}

OnScreenInputPlaceView::~OnScreenInputPlaceView()
{
	app().applyOSNavStyle(appContext(), false);
	window().removeOnFrame(animate);
}

void OnScreenInputPlaceView::place()
{
	dragTracker.reset();
	auto exitBtnPos = viewRect().pos(C2DO);
	int exitBtnSize = window().widthMMInPixels(10.);
	exitBtnRect = IG::makeWindowRectRel(exitBtnPos - IG::WP{exitBtnSize/2, exitBtnSize/2}, {exitBtnSize, exitBtnSize});
	text.compile(renderer());
}

bool OnScreenInputPlaceView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &e)
		{
			if(e.pushed() && animationStartTimer.isArmed())
			{
				animationStartTimer.dispatchEarly();
			}
			dragTracker.inputEvent(e,
				[&](Input::DragTrackerState, DragData &d)
				{
					if(d.elem)
						return;
					auto tryGrabElement = [&](VControllerElement &elem)
					{
						if(elem.state() == VControllerState::OFF || !elem.bounds().contains(e.pos()))
							return false;
						for(const auto &state : dragTracker.stateList())
						{
							if(state.data.elem == &elem)
								return false; // element already grabbed
						}
						d.elem = &elem;
						d.startPos = elem.bounds().pos(C2DO);
						return true;
					};
					for(auto &elem : vController().deviceElements())
					{
						if(tryGrabElement(elem))
							return;
					}
					for(auto &elem : vController().guiElements())
					{
						if(tryGrabElement(elem))
							return;
					}
				},
				[&](Input::DragTrackerState state, Input::DragTrackerState, DragData &d)
				{
					if(d.elem)
					{
						auto newPos = d.startPos + state.downPosDiff();
						auto contentBounds = vController().windowData().contentBounds();
						auto bounds = vController().layoutBounds();
						d.elem->setPos(newPos, bounds);
						auto layoutPos = VControllerLayoutPosition::fromPixelPos(d.elem->bounds().pos(C2DO), d.elem->bounds().size(), viewRect());
						//logMsg("set pos %d,%d from %d,%d", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
						auto &vCtrlLayoutPos = d.elem->layoutPos[window().isPortrait()];
						vCtrlLayoutPos.origin = layoutPos.origin;
						vCtrlLayoutPos.pos = layoutPos.pos;
						vController().setLayoutPositionChanged();
						app().viewController().placeEmuViews();
						postDraw();
					}
				},
				[&](Input::DragTrackerState state, DragData &d)
				{
					if(!d.elem && exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
					{
						dismiss();
					}
				});
			return true;
		}
	}, e);
}

void OnScreenInputPlaceView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	vController().draw(cmds, true, .75);
	cmds.setColor(.5, .5, .5);
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	const int lineSize = 1;
	GeomRect::draw(cmds, WRect{{displayRect().x, displayRect().yCenter()},
		{displayRect().x2, displayRect().yCenter() + lineSize}});
	GeomRect::draw(cmds, WRect{{displayRect().xCenter(), displayRect().y},
		{displayRect().xCenter() + lineSize, displayRect().y2}});

	if(textFade != 0.)
	{
		cmds.setColor(0., 0., 0., textFade/2.);
		GeomRect::draw(cmds, Gfx::GCRect::makeRel({-text.width()/2.f - text.spaceWidth(), -text.height()/2.f - text.spaceWidth()},
			{text.width() + text.spaceWidth()*2.f, text.height() + text.spaceWidth()*2.f}));
		cmds.setColor(1., 1., 1., textFade);
		basicEffect.enableAlphaTexture(cmds);
		text.draw(cmds, viewRect().pos(C2DO), C2DO);
	}
}

static void drawVControllerElement(Gfx::RendererCommands &__restrict__ cmds, const VControllerElement &elem)
{
	cmds.set(Gfx::BlendMode::ALPHA);
	Gfx::Color whiteCol{1., 1., 1., .75};
	cmds.setColor(whiteCol);
	elem.draw(cmds);
}

class DPadElementConfigView : public TableView, public EmuAppHelper<DPadElementConfigView>
{
public:
	DPadElementConfigView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_, VControllerElement &elem_):
		TableView{"Edit D-Pad", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		confView{confView_},
		deadzoneItems
		{
			{"1",    &defaultFace(), setDeadzoneDel(), 100},
			{"1.35", &defaultFace(), setDeadzoneDel(), 135},
			{"1.6",  &defaultFace(), setDeadzoneDel(), 160},
		},
		deadzone
		{
			"Deadzone", &defaultFace(),
			MenuItem::Id{elem.dPad()->deadzone()},
			deadzoneItems
		},
		diagonalSensitivityItems
		{
			{"None",  &defaultFace(), setDiagonalSensitivityDel(), 1000},
			{"Low",   &defaultFace(), setDiagonalSensitivityDel(), 1500},
			{"M-Low", &defaultFace(), setDiagonalSensitivityDel(), 1750},
			{"Med.",  &defaultFace(), setDiagonalSensitivityDel(), 2000},
			{"High",  &defaultFace(), setDiagonalSensitivityDel(), 2500},
		},
		diagonalSensitivity
		{
			"Diagonal Sensitivity", &defaultFace(),
			MenuItem::Id(elem.dPad()->diagonalSensitivity() * 1000.f),
			diagonalSensitivityItems
		},
		stateItems
		{
			{ctrlStateStr[0], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::OFF)},
			{ctrlStateStr[1], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::SHOWN)},
			{ctrlStateStr[2], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::HIDDEN)},
		},
		state
		{
			"State", &defaultFace(),
			MenuItem::Id(elem.layoutPos[window().isPortrait()].state),
			stateItems
		},
		showBoundingArea
		{
			"Show Bounding Area", &defaultFace(),
			elem.dPad()->showBounds(),
			[this](BoolMenuItem &item)
			{
				elem.dPad()->setShowBounds(renderer(), item.flipBoolValue());
				vCtrl.place();
				postDraw();
			}
		},
		remove
		{
			"Remove This D-Pad", &defaultFace(),
			[this]()
			{
				vCtrl.remove(elem);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			}
		}
	{
		item.emplace_back(&state);
		item.emplace_back(&deadzone);
		item.emplace_back(&diagonalSensitivity);
		item.emplace_back(&showBoundingArea);
		item.emplace_back(&remove);
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		drawVControllerElement(cmds, elem);
		TableView::draw(cmds);
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	TouchConfigView &confView;
	TextMenuItem deadzoneItems[3];
	MultiChoiceMenuItem deadzone;
	TextMenuItem diagonalSensitivityItems[5];
	MultiChoiceMenuItem diagonalSensitivity;
	TextMenuItem stateItems[3];
	MultiChoiceMenuItem state;
	BoolMenuItem showBoundingArea;
	TextMenuItem remove;
	StaticArrayList<MenuItem*, 5> item;

	TextMenuItem::SelectDelegate setButtonStateDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.layoutPos[window().isPortrait()].state = VControllerState(item.id());
			vCtrl.setLayoutPositionChanged();
			vCtrl.place();
		};
	}

	TextMenuItem::SelectDelegate setDeadzoneDel()
	{
		return [this](TextMenuItem &item){ elem.dPad()->setDeadzone(renderer(), item.id(), window()); };
	}

	TextMenuItem::SelectDelegate setDiagonalSensitivityDel()
	{
		return [this](TextMenuItem &item){ elem.dPad()->setDiagonalSensitivity(renderer(), float(item.id()) / 1000.f); };
	}
};

class ButtonGroupElementConfigView : public TableView, public EmuAppHelper<ButtonGroupElementConfigView>
{
public:
	ButtonGroupElementConfigView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_, VControllerElement &elem_):
		TableView{"Edit Buttons", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		confView{confView_},
		stateItems
		{
			{ctrlStateStr[0], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::OFF)},
			{ctrlStateStr[1], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::SHOWN)},
			{ctrlStateStr[2], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::HIDDEN)},
		},
		state
		{
			"State", &defaultFace(),
			MenuItem::Id(elem.layoutPos[window().isPortrait()].state),
			stateItems
		},
		spaceItems
		{
			{"1", &defaultFace(), setButtonSpaceDel(), 100},
			{"2", &defaultFace(), setButtonSpaceDel(), 200},
			{"3", &defaultFace(), setButtonSpaceDel(), 300},
			{"4", &defaultFace(), setButtonSpaceDel(), 400},
		},
		space
		{
			"Spacing", &defaultFace(),
			MenuItem::Id{elem.buttonGroup()->spacing()},
			spaceItems
		},
		staggerItems
		{
			{"-0.75x V", &defaultFace(), setButtonStaggerDel(), 0},
			{"-0.5x V",  &defaultFace(), setButtonStaggerDel(), 1},
			{"0",        &defaultFace(), setButtonStaggerDel(), 2},
			{"0.5x V",   &defaultFace(), setButtonStaggerDel(), 3},
			{"0.75x V",  &defaultFace(), setButtonStaggerDel(), 4},
			{"1x H&V",   &defaultFace(), setButtonStaggerDel(), 5},
		},
		stagger
		{
			"Stagger", &defaultFace(),
			MenuItem::Id{elem.buttonGroup()->stagger()},
			staggerItems
		},
		extraXSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[0]},
			{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[1]},
			{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[2]},
			{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[3]},
		},
		extraXSize
		{
			"H Bound Overlap", &defaultFace(),
			MenuItem::Id{elem.buttonGroup()->xPadding()},
			extraXSizeItems
		},
		extraYSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[0]},
			{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[1]},
			{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[2]},
			{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[3]},
		},
		extraYSize
		{
			"V Bound Overlap", &defaultFace(),
			MenuItem::Id{elem.buttonGroup()->yPadding()},
			extraYSizeItems
		},
		showBoundingArea
		{
			"Show Bounding Area", &defaultFace(),
			elem.buttonGroup()->showsBounds(),
			[this](BoolMenuItem &item)
			{
				elem.buttonGroup()->setShowBounds(item.flipBoolValue());
				vCtrl.place();
				postDraw();
			}
		},
		remove
		{
			"Remove This Button Group", &defaultFace(),
			[this]()
			{
				vCtrl.remove(elem);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			}
		}
	{
		item.emplace_back(&state);
		item.emplace_back(&space);
		item.emplace_back(&stagger);
		item.emplace_back(&extraXSize);
		item.emplace_back(&extraYSize);
		item.emplace_back(&showBoundingArea);
		item.emplace_back(&remove);
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		drawVControllerElement(cmds, elem);
		TableView::draw(cmds);
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	TouchConfigView &confView;
	TextMenuItem stateItems[3];
	MultiChoiceMenuItem state;
	TextMenuItem spaceItems[4];
	MultiChoiceMenuItem space;
	TextMenuItem staggerItems[6];
	MultiChoiceMenuItem stagger;
	TextMenuItem extraXSizeItems[4];
	MultiChoiceMenuItem extraXSize;
	TextMenuItem extraYSizeItems[4];
	MultiChoiceMenuItem extraYSize;
	BoolMenuItem showBoundingArea;
	TextMenuItem remove;
	StaticArrayList<MenuItem*, 7> item;

	TextMenuItem::SelectDelegate setButtonStateDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.layoutPos[window().isPortrait()].state = VControllerState(item.id());
			vCtrl.setLayoutPositionChanged();
			vCtrl.place();
		};
	}

	TextMenuItem::SelectDelegate setButtonSpaceDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.buttonGroup()->setSpacing(item.id(), window());
			vCtrl.place();
		};
	}

	TextMenuItem::SelectDelegate setButtonExtraXSizeDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.buttonGroup()->setXPadding(item.id());
			vCtrl.place();
		};
	}

	TextMenuItem::SelectDelegate setButtonExtraYSizeDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.buttonGroup()->setYPadding(item.id());
			vCtrl.place();
		};
	}

	TextMenuItem::SelectDelegate setButtonStaggerDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.buttonGroup()->setStaggerType(item.id());
			vCtrl.place();
		};
	}
};

class UIButtonGroupElementConfigView : public TableView, public EmuAppHelper<UIButtonGroupElementConfigView>
{
public:
	UIButtonGroupElementConfigView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_, VControllerElement &elem_):
		TableView{"Edit UI Buttons", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		confView{confView_},
		stateItems
		{
			{ctrlStateStr[0], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::OFF)},
			{ctrlStateStr[1], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::SHOWN)},
			{ctrlStateStr[2], &defaultFace(), setButtonStateDel(), to_underlying(VControllerState::HIDDEN)},
		},
		state
		{
			"State", &defaultFace(),
			MenuItem::Id(elem.layoutPos[window().isPortrait()].state),
			stateItems
		},
		remove
		{
			"Remove This Button Group", &defaultFace(),
			[this]()
			{
				vCtrl.remove(elem);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			}
		}
	{
		item.emplace_back(&state);
		item.emplace_back(&remove);
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		drawVControllerElement(cmds, elem);
		TableView::draw(cmds);
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	TouchConfigView &confView;
	TextMenuItem stateItems[3];
	MultiChoiceMenuItem state;
	TextMenuItem remove;
	StaticArrayList<MenuItem*, 2> item;

	TextMenuItem::SelectDelegate setButtonStateDel()
	{
		return [this](TextMenuItem &item)
		{
			elem.layoutPos[window().isPortrait()].state = VControllerState(item.id());
			vCtrl.setLayoutPositionChanged();
			vCtrl.place();
		};
	}
};

class AddNewButtonView : public TableView, public EmuAppHelper<AddNewButtonView>
{
public:
	AddNewButtonView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_):
		TableView{"Add New Button Group", attach, buttons},
		vCtrl{vCtrl_},
		confView{confView_},
		components{system().inputDeviceDesc(0).components}
	{
		for(const auto &c : components)
		{
			buttons.emplace_back(
				c.name, &defaultFace(),
				[this, &c](const Input::Event &e)
				{
					vCtrl.add(c);
					vCtrl.setLayoutPositionChanged();
					vCtrl.place();
					confView.reloadItems();
					dismiss();
				});
		}
		buttons.emplace_back(
			"Open Menu", &defaultFace(),
			[this](const Input::Event &e)
			{
				vCtrl.add(std::array<unsigned, 1>{guiKeyIdxLastView}, InputComponent::ui, RT2DO);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			});
		buttons.emplace_back(
			"Toggle Slow/Fast Mode", &defaultFace(),
			[this](const Input::Event &e)
			{
				vCtrl.add(std::array<unsigned, 1>{guiKeyIdxFastForward}, InputComponent::ui, LT2DO);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			});
	}

private:
	VController &vCtrl;
	TouchConfigView &confView;
	std::vector<TextMenuItem> buttons;
	std::span<const InputComponentDesc> components;
};

TextMenuItem::SelectDelegate TouchConfigView::setVisibilityDel(VControllerVisibility val)
{
	return [this, val](){ vController().setGamepadControlsVisibility(val); };
}

TextMenuItem::SelectDelegate TouchConfigView::setPointerInputPlayerDel(int val)
{
	return [this, val](){ vController().setInputPlayer(val); };
}

TextMenuItem::SelectDelegate TouchConfigView::setSizeDel()
{
	return [this](TextMenuItem &item){ vController().setButtonSize(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setAlphaDel()
{
	return [this](TextMenuItem &item){ vController().setButtonAlpha(item.id()); };
}

void TouchConfigView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	vController().draw(cmds, true, .75);
	TableView::draw(cmds);
}

void TouchConfigView::place()
{
	refreshTouchConfigMenu();
	TableView::place();
}

void TouchConfigView::refreshTouchConfigMenu()
{
	alpha.setSelected((MenuItem::Id)vController().buttonAlpha(), *this);
	touchCtrl.setSelected((int)vController().gamepadControlsVisibility(), *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.setSelected((int)vController().inputPlayer(), *this);
	size.setSelected((MenuItem::Id)vController().buttonSize(), *this);
	if(app().vibrationManager().hasVibrator())
	{
		vibrate.setBoolValue(vController().vibrateOnTouchInput(), *this);
	}
	showOnTouch.setBoolValue(vController().showOnTouchInput(), *this);
}

TouchConfigView::TouchConfigView(ViewAttachParams attach, VController &vCtrl):
	TableView{"On-screen Input Setup", attach, item},
	vControllerPtr{&vCtrl},
	touchCtrlItem
	{
		{"Off",  &defaultFace(), setVisibilityDel(VControllerVisibility::OFF)},
		{"On",   &defaultFace(), setVisibilityDel(VControllerVisibility::ON)},
		{"Auto", &defaultFace(), setVisibilityDel(VControllerVisibility::AUTO)}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", &defaultFace(),
		(int)vCtrl.gamepadControlsVisibility(),
		touchCtrlItem
	},
	pointerInputItem
	{
		{"1", &defaultFace(), setPointerInputPlayerDel(0)},
		{"2", &defaultFace(), setPointerInputPlayerDel(1)},
		{"3", &defaultFace(), setPointerInputPlayerDel(2)},
		{"4", &defaultFace(), setPointerInputPlayerDel(3)},
		{"5", &defaultFace(), setPointerInputPlayerDel(4)},
	},
	pointerInput
	{
		"Virtual Gamepad Player", &defaultFace(),
		(int)vCtrl.inputPlayer(),
		[this](const MultiChoiceMenuItem &)
		{
			return EmuSystem::maxPlayers;
		},
		[this](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return pointerInputItem[idx];
		}
	},
	sizeItem
	{
		{"6.5", &defaultFace(), setSizeDel(), 650},
		{"7",   &defaultFace(), setSizeDel(), 700},
		{"7.5", &defaultFace(), setSizeDel(), 750},
		{"8",   &defaultFace(), setSizeDel(), 800},
		{"8.5", &defaultFace(), setSizeDel(), 850},
		{"9",   &defaultFace(), setSizeDel(), 900},
		{"10",  &defaultFace(), setSizeDel(), 1000},
		{"12",  &defaultFace(), setSizeDel(), 1200},
		{"14",  &defaultFace(), setSizeDel(), 1400},
		{"15",  &defaultFace(), setSizeDel(), 1500},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 3.0 to 15.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 100.0;
						if(vController().setButtonSize(scaledIntVal))
						{
							size.setSelected((MenuItem::Id)scaledIntVal, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	size
	{
		"Button Size", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{:.2f}", vController().buttonSize() / 100.));
			return true;
		},
		(MenuItem::Id)vController().buttonSize(),
		sizeItem
	},
	vibrate
	{
		"Vibration", &defaultFace(),
		vController().vibrateOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController().setVibrateOnTouchInput(app(), item.flipBoolValue(*this));
		}
	},
	showOnTouch
	{
		"Show Gamepad If Screen Touched", &defaultFace(),
		vController().showOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController().setShowOnTouchInput(item.flipBoolValue(*this));
		}
	},
	alphaItem
	{
		{"0%",  &defaultFace(), setAlphaDel(), 0},
		{"10%", &defaultFace(), setAlphaDel(), int(255. * .1)},
		{"25%", &defaultFace(), setAlphaDel(), int(255. * .25)},
		{"50%", &defaultFace(), setAlphaDel(), int(255. * .5)},
		{"65%", &defaultFace(), setAlphaDel(), int(255. * .65)},
		{"75%", &defaultFace(), setAlphaDel(), int(255. * .75)},
	},
	alpha
	{
		"Blend Amount", &defaultFace(),
		(MenuItem::Id)vController().buttonAlpha(),
		alphaItem
	},
	btnPlace
	{
		"Set Button Positions", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<OnScreenInputPlaceView>(vController()), e);
		}
	},
	addButton
	{
		"Add New Button Group", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<AddNewButtonView>(*this, vController()), e);
		}
	},
	allowButtonsPastContentBounds
	{
		"Allow Buttons In Display Cutout Area", &defaultFace(),
		vController().allowButtonsPastContentBounds(),
		[this](BoolMenuItem &item)
		{
			vController().setAllowButtonsPastContentBounds(item.flipBoolValue(*this));
			vController().place();
		}
	},
	resetControls
	{
		"Reset Positions & States", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset buttons to default positions?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController().resetPositions();
					vController().place();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	resetAllControls
	{
		"Reset To Defaults", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset all on-screen controls and options to default?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController().resetAllOptions();
					vController().place();
					reloadItems();
					refreshTouchConfigMenu();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	devButtonsHeading
	{
		"Emulated Device Button Groups", &defaultBoldFace()
	},
	uiButtonsHeading
	{
		"UI Button Groups", &defaultBoldFace()
	},
	otherHeading
	{
		"Other Options", &defaultBoldFace()
	}
{
	reloadItems();
}

void TouchConfigView::reloadItems()
{
	elementItems.clear();
	item.clear();
	item.emplace_back(&touchCtrl);
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&pointerInput);
	}
	item.emplace_back(&size);
	item.emplace_back(&btnPlace);
	item.emplace_back(&devButtonsHeading);
	elementItems.reserve(vController().deviceElements().size() + vController().guiElements().size());
	for(auto &elem : vController().deviceElements())
	{
		auto &i = elementItems.emplace_back(
			elem.name(app()), &defaultFace(),
			[this, &elem](const Input::Event &e)
			{
				visit(overloaded
				{
					[&](VControllerDPad &){ pushAndShow(makeView<DPadElementConfigView>(*this, vController(), elem), e); },
					[&](VControllerButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController(), elem), e); },
					[](auto &){}
				}, elem);
			});
		item.emplace_back(&i);
	}
	item.emplace_back(&uiButtonsHeading);
	for(auto &elem : vController().guiElements())
	{
		auto &i = elementItems.emplace_back(
			elem.name(app()), &defaultFace(),
			[this, &elem](const Input::Event &e)
			{
				visit(overloaded
				{
					[&](VControllerUIButtonGroup &){ pushAndShow(makeView<UIButtonGroupElementConfigView>(*this, vController(), elem), e); },
					[](auto &){}
				}, elem);
			});
		item.emplace_back(&i);
	}
	item.emplace_back(&otherHeading);
	item.emplace_back(&addButton);
	if(used(allowButtonsPastContentBounds) && appContext().hasDisplayCutout())
	{
		item.emplace_back(&allowButtonsPastContentBounds);
	}
	if(app().vibrationManager().hasVibrator())
	{
		item.emplace_back(&vibrate);
	}
	item.emplace_back(&showOnTouch);
	item.emplace_back(&alpha);
	item.emplace_back(&resetControls);
	item.emplace_back(&resetAllControls);
}

}
