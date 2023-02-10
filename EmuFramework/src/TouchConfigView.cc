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
#include <imagine/gui/TextTableView.hh>
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
#include "PlaceVideoView.hh"

namespace EmuEx
{

constexpr bool CAN_TURN_OFF_MENU_BTN = !Config::envIsIOS;

constexpr const char *ctrlStateStr[]
{
	"Off", "On", "Hidden"
};

constexpr const char *touchCtrlExtraBtnSizeMenuName[4]
{
	"None", "10%", "20%", "30%"
};

constexpr int touchCtrlExtraBtnSizeMenuVal[4]
{
	0, 10, 20, 30
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
	Gfx::Text text;
	VController *vControllerPtr;
	IG::InterpolatorValue<float, IG::FrameTime, IG::InterpolatorType::LINEAR> textFade{};
	IG::Timer animationStartTimer{"OnScreenInputPlaceView::animationStartTimer"};
	IG::OnFrameDelegate animate;
	IG::WindowRect exitBtnRect{};
	Input::DragTracker<DragData> dragTracker;

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
						auto bounds = window().bounds();
						d.elem->setPos(newPos, bounds);
						auto layoutPos = VControllerLayoutPosition::fromPixelPos(d.elem->bounds().pos(C2DO), d.elem->bounds().size(), bounds);
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
	GeomRect::draw(cmds, WRect{{viewRect().x, viewRect().yCenter()},
		{viewRect().x2, viewRect().yCenter() + lineSize}});
	GeomRect::draw(cmds, WRect{{viewRect().xCenter(), viewRect().y},
		{viewRect().xCenter() + lineSize, viewRect().y2}});

	if(textFade != 0.)
	{
		cmds.setColor(0., 0., 0., textFade/2.);
		GeomRect::draw(cmds, WRect{viewRect().pos(C2DO) - text.pixelSize() / 2 - text.spaceWidth(),
			viewRect().pos(C2DO) + text.pixelSize() / 2 + text.spaceWidth()});
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
			{"1",    &defaultFace(), 100},
			{"1.35", &defaultFace(), 135},
			{"1.6",  &defaultFace(), 160},
		},
		deadzone
		{
			"Deadzone", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item) { elem.dPad()->setDeadzone(renderer(), item.id(), window()); }
			},
			MenuItem::Id{elem.dPad()->deadzone()},
			deadzoneItems
		},
		diagonalSensitivityItems
		{
			{"None",  &defaultFace(), 1000},
			{"Low",   &defaultFace(), 1500},
			{"M-Low", &defaultFace(), 1750},
			{"Med.",  &defaultFace(), 2000},
			{"High",  &defaultFace(), 2500},
		},
		diagonalSensitivity
		{
			"Diagonal Sensitivity", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item) { elem.dPad()->setDiagonalSensitivity(renderer(), float(item.id()) / 1000.f); }
			},
			MenuItem::Id(elem.dPad()->diagonalSensitivity() * 1000.f),
			diagonalSensitivityItems
		},
		stateItems
		{
			{ctrlStateStr[0], &defaultFace(), to_underlying(VControllerState::OFF)},
			{ctrlStateStr[1], &defaultFace(), to_underlying(VControllerState::SHOWN)},
			{ctrlStateStr[2], &defaultFace(), to_underlying(VControllerState::HIDDEN)},
		},
		state
		{
			"State", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.layoutPos[window().isPortrait()].state = VControllerState(item.id());
					vCtrl.setLayoutPositionChanged();
					vCtrl.place();
				}
			},
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
			[this](const Input::Event &e)
			{
				auto ynAlertView = makeView<YesNoAlertView>("Really remove this d-pad?");
				ynAlertView->setOnYes(
					[this]()
					{
						vCtrl.remove(elem);
						vCtrl.setLayoutPositionChanged();
						vCtrl.place();
						confView.reloadItems();
						dismiss();
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
		} {}

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
	std::array<MenuItem*, 5> item{&state, &deadzone, &diagonalSensitivity, &showBoundingArea, &remove};
};

static void addCategories(EmuApp &app, VControllerElement &elem, auto &&addCategory)
{
	if(elem.uiButtonGroup())
		addCategory(app.inputControlCategories()[0]);
	else
	{
		for(auto &cat : app.inputControlCategories() | std::views::drop(1)
			| std::views::filter([](auto &c){return !c.multiplayerIndex;}))
		{
			addCategory(cat);
		}
	}
}

class ButtonElementConfigView : public TableView, public EmuAppHelper<ButtonElementConfigView>
{
public:
	using OnChange = DelegateFunc<void()>;

	ButtonElementConfigView(ViewAttachParams attach, OnChange onChange_, VController &vCtrl_, VControllerElement &elem_, VControllerButton &btn_):
		TableView{"Edit Button", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		btn{btn_},
		onChange{onChange_},
		key
		{
			"Action", app().systemKeyName(btn_.key), &defaultFace(),
			[this](const Input::Event &e)
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Assign Action", 16);
				addCategories(app(), elem, [&](const KeyCategory &cat)
				{
					for(auto i : iotaCount(cat.keys()))
					{
						multiChoiceView->appendItem(cat.keyName[i],
							[this, keyCode = cat.configOffset + i](View &parentView, const Input::Event &e)
							{
								btn.key = keyCode;
								key.set2ndName(app().systemKeyName(keyCode));
								vCtrl.update(elem);
								onChange.callSafe();
								vCtrl.setLayoutPositionChanged();
								vCtrl.place();
								parentView.dismiss();
							});
					}
				});
				pushAndShow(std::move(multiChoiceView), e);
			}
		},
		remove
		{
			"Remove This Button", &defaultFace(),
			[this](const Input::Event &e)
			{
				auto ynAlertView = makeView<YesNoAlertView>("Really remove this button?");
				ynAlertView->setOnYes(
					[this]()
					{
						elem.remove(btn);
						onChange.callSafe();
						vCtrl.setLayoutPositionChanged();
						vCtrl.place();
						dismiss();
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
		} {}

private:
	VController &vCtrl;
	VControllerElement &elem;
	VControllerButton &btn;
	OnChange onChange;
	DualTextMenuItem key;
	TextMenuItem remove;
	std::array<MenuItem*, 2> item{&key, &remove};
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
			{ctrlStateStr[0], &defaultFace(), to_underlying(VControllerState::OFF)},
			{ctrlStateStr[1], &defaultFace(), to_underlying(VControllerState::SHOWN)},
			{ctrlStateStr[2], &defaultFace(), to_underlying(VControllerState::HIDDEN)},
		},
		state
		{
			"State", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.layoutPos[window().isPortrait()].state = VControllerState(item.id());
					vCtrl.setLayoutPositionChanged();
					vCtrl.place();
				}
			},
			MenuItem::Id(elem.layoutPos[window().isPortrait()].state),
			stateItems
		},
		rowSizeItems
		{
			{"1", &defaultFace(), 1},
			{"2", &defaultFace(), 2},
			{"3", &defaultFace(), 3},
			{"4", &defaultFace(), 4},
			{"5", &defaultFace(), 5},
		},
		rowSize
		{
			"Buttons Per Row", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.setRowSize(item.id());
					vCtrl.setLayoutPositionChanged();
					vCtrl.place();
				}
			},
			MenuItem::Id(elem.rowSize()),
			rowSizeItems
		},
		spaceItems
		{
			{"1", &defaultFace(), 100},
			{"2", &defaultFace(), 200},
			{"3", &defaultFace(), 300},
			{"4", &defaultFace(), 400},
		},
		space
		{
			"Spacing", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setSpacing(item.id(), window());
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->spacing() : 0},
			spaceItems
		},
		staggerItems
		{
			{"-0.75x V", &defaultFace(), 0},
			{"-0.5x V",  &defaultFace(), 1},
			{"0",        &defaultFace(), 2},
			{"0.5x V",   &defaultFace(), 3},
			{"0.75x V",  &defaultFace(), 4},
			{"1x H&V",   &defaultFace(), 5},
		},
		stagger
		{
			"Stagger", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setStaggerType(item.id());
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->stagger() : 0},
			staggerItems
		},
		extraXSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[0]},
			{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[1]},
			{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[2]},
			{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[3]},
		},
		extraXSize
		{
			"Extended H Bounds", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setXPadding(item.id());
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->xPadding() : 0},
			extraXSizeItems
		},
		extraYSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[0]},
			{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[1]},
			{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[2]},
			{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), touchCtrlExtraBtnSizeMenuVal[3]},
		},
		extraYSize
		{
			"Extended V Bounds", &defaultFace(),
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setYPadding(item.id());
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->yPadding() : 0},
			extraYSizeItems
		},
		showBoundingArea
		{
			"Show Bounding Area", &defaultFace(),
			elem.buttonGroup() ? elem.buttonGroup()->showsBounds() : false,
			[this](BoolMenuItem &item)
			{
				elem.buttonGroup()->setShowBounds(item.flipBoolValue());
				vCtrl.place();
				postDraw();
			}
		},
		add
		{
			"Add Button To This Group", &defaultFace(),
			[this](const Input::Event &e)
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Add Button", 16);
				addCategories(app(), elem, [&](const KeyCategory &cat)
				{
					for(auto i : iotaCount(cat.keys()))
					{
						multiChoiceView->appendItem(cat.keyName[i],
							[this, keyCode = cat.configOffset + i](View &parentView, const Input::Event &e)
							{
								elem.add(keyCode);
								vCtrl.update(elem);
								vCtrl.setLayoutPositionChanged();
								vCtrl.place();
								confView.reloadItems();
								reloadItems();
								parentView.dismiss();
							});
					}
				});
				pushAndShow(std::move(multiChoiceView), e);
			}
		},
		remove
		{
			"Remove This Button Group", &defaultFace(),
			[this](const Input::Event &e)
			{
				auto ynAlertView = makeView<YesNoAlertView>("Really remove this button group?");
				ynAlertView->setOnYes(
					[this]()
					{
						vCtrl.remove(elem);
						vCtrl.setLayoutPositionChanged();
						vCtrl.place();
						confView.reloadItems();
						dismiss();
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
		},
		buttonsHeading{"Buttons In Group", &defaultBoldFace()}
	{
		reloadItems();
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
	TextMenuItem rowSizeItems[5];
	MultiChoiceMenuItem rowSize;
	TextMenuItem spaceItems[4];
	MultiChoiceMenuItem space;
	TextMenuItem staggerItems[6];
	MultiChoiceMenuItem stagger;
	TextMenuItem extraXSizeItems[4];
	MultiChoiceMenuItem extraXSize;
	TextMenuItem extraYSizeItems[4];
	MultiChoiceMenuItem extraYSize;
	BoolMenuItem showBoundingArea;
	TextMenuItem add;
	TextMenuItem remove;
	TextHeadingMenuItem buttonsHeading;
	std::vector<TextMenuItem> buttonItems;
	std::vector<MenuItem*> item;

	void reloadItems()
	{
		buttonItems.clear();
		item.clear();
		item.emplace_back(&state);
		if(elem.buttonGroup())
		{
			item.emplace_back(&space);
			item.emplace_back(&stagger);
			item.emplace_back(&extraXSize);
			item.emplace_back(&extraYSize);
			item.emplace_back(&showBoundingArea);
		}
		item.emplace_back(&rowSize);
		item.emplace_back(&add);
		item.emplace_back(&remove);
		item.emplace_back(&buttonsHeading);
		auto buttons = elem.buttons();
		buttonItems.reserve(buttons.size());
		for(auto &btn : buttons)
		{
			auto &i = buttonItems.emplace_back(
				btn.name(app()), &defaultFace(),
				[this, &btn](const Input::Event &e)
				{
					pushAndShow(makeView<ButtonElementConfigView>([this]()
					{
						confView.reloadItems();
						reloadItems();
					}, vCtrl, elem, btn), e);
				});
			item.emplace_back(&i);
		}
	}
};

class AddNewButtonView : public TableView, public EmuAppHelper<AddNewButtonView>
{
public:
	AddNewButtonView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_):
		TableView{"Add New Button Group", attach, buttons},
		vCtrl{vCtrl_},
		confView{confView_}
	{
		for(const auto &c : system().inputDeviceDesc(0).components)
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
			rightUIComponents.name, &defaultFace(),
			[this](const Input::Event &e)
			{
				vCtrl.add(rightUIComponents);
				vCtrl.setLayoutPositionChanged();
				vCtrl.place();
				confView.reloadItems();
				dismiss();
			});
		buttons.emplace_back(
			leftUIComponents.name, &defaultFace(),
			[this](const Input::Event &e)
			{
				vCtrl.add(leftUIComponents);
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
};

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
		{"Off",  &defaultFace(), to_underlying(VControllerVisibility::OFF)},
		{"On",   &defaultFace(), to_underlying(VControllerVisibility::ON)},
		{"Auto", &defaultFace(), to_underlying(VControllerVisibility::AUTO)}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController().setGamepadControlsVisibility(VControllerVisibility(item.id())); }
		},
		int(vCtrl.gamepadControlsVisibility()),
		touchCtrlItem
	},
	pointerInputItem
	{
		{"1", &defaultFace(), 0},
		{"2", &defaultFace(), 1},
		{"3", &defaultFace(), 2},
		{"4", &defaultFace(), 3},
		{"5", &defaultFace(), 4},
	},
	pointerInput
	{
		"Virtual Gamepad Player", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController().setInputPlayer(item.id()); }
		},
		int(vCtrl.inputPlayer()),
		std::span{pointerInputItem, size_t(EmuSystem::maxPlayers)}
	},
	sizeItem
	{
		{"6.5", &defaultFace(), 650},
		{"7",   &defaultFace(), 700},
		{"7.5", &defaultFace(), 750},
		{"8",   &defaultFace(), 800},
		{"8.5", &defaultFace(), 850},
		{"9",   &defaultFace(), 900},
		{"10",  &defaultFace(), 1000},
		{"12",  &defaultFace(), 1200},
		{"14",  &defaultFace(), 1400},
		{"15",  &defaultFace(), 1500},
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
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{:.2f}", vController().buttonSize() / 100.));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController().setButtonSize(item.id()); }
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
		{"0%",  &defaultFace(), 0},
		{"10%", &defaultFace(), int(255. * .1)},
		{"25%", &defaultFace(), int(255. * .25)},
		{"50%", &defaultFace(), int(255. * .5)},
		{"65%", &defaultFace(), int(255. * .65)},
		{"75%", &defaultFace(), int(255. * .75)},
	},
	alpha
	{
		"Blend Amount", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController().setButtonAlpha(item.id()); }
		},
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
	placeVideo
	{
		"Set Video Position", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			pushAndShowModal(makeView<PlaceVideoView>(app().videoLayer(), vController()), e);
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
	placeVideo.setActive(system().hasContent());
	item.emplace_back(&placeVideo);
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
					[&](VControllerUIButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController(), elem), e); },
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
