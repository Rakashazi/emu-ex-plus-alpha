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
#include <emuframework/AppKeyCode.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include "PlaceVideoView.hh"
#include "PlaceVControlsView.hh"
#include <utility>
#include <vector>
#include <array>
#include <span>
#include <ranges>
#include <format>

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

static void drawVControllerElement(Gfx::RendererCommands &__restrict__ cmds, const VControllerElement &elem, size_t layoutIdx)
{
	cmds.set(Gfx::BlendMode::PREMULT_ALPHA);
	elem.draw(cmds, layoutIdx);
}

static void addCategories(EmuApp &app, VControllerElement &elem, auto &&addCategory)
{
	if(elem.uiButtonGroup())
	{
		addCategory(appKeyCategory);
	}
	else
	{
		for(auto &cat : EmuApp::keyCategories() | std::views::filter([](auto &c){return !c.multiplayerIndex;}))
		{
			addCategory(cat);
		}
	}
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
			{"1mm",    &defaultFace(), 100},
			{"1.35mm", &defaultFace(), 135},
			{"1.6mm",  &defaultFace(), 160},
			{"Custom Value", &defaultFace(),
				[this](const Input::Event &e)
				{
					app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input 1.0 to 3.0", "",
						[this](EmuApp &app, auto val)
						{
							int scaledIntVal = val * 100.0;
							if(elem.dPad()->setDeadzone(renderer(), scaledIntVal, window()))
							{
								deadzone.setSelected((MenuItem::Id)scaledIntVal, *this);
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
		deadzone
		{
			"Deadzone", &defaultFace(),
			{
				.onSetDisplayString = [this](auto idx, Gfx::Text &t)
				{
					t.resetString(std::format("{:g}mm", elem.dPad()->deadzone() / 100.));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item) { elem.dPad()->setDeadzone(renderer(), item.id(), window()); }
			},
			MenuItem::Id{elem.dPad()->deadzone()},
			deadzoneItems
		},
		diagonalSensitivityItems
		{
			{"None",             &defaultFace(), 1000},
			{"33% (Low)",        &defaultFace(), 667},
			{"43% (Medium-Low)", &defaultFace(), 570},
			{"50% (Medium)",     &defaultFace(), 500},
			{"60% (High)",       &defaultFace(), 400},
			{"Custom Value", &defaultFace(),
				[this](const Input::Event &e)
				{
					app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input 0 to 99.0", "",
						[this](EmuApp &app, auto val)
						{
							val = 100. - val;
							int scaledIntVal = val * 10.0;
							val /= 100.;
							if(elem.dPad()->setDiagonalSensitivity(renderer(), val))
							{
								diagonalSensitivity.setSelected((MenuItem::Id)scaledIntVal, *this);
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
		diagonalSensitivity
		{
			"Diagonal Sensitivity", &defaultFace(),
			{
				.onSetDisplayString = [this](auto idx, Gfx::Text &t)
				{
					t.resetString(std::format("{:g}%", 100.f - elem.dPad()->diagonalSensitivity() * 100.f));
					return true;
				},
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
					elem.state = VControllerState(item.id());
					vCtrl.place();
				}
			},
			MenuItem::Id(elem.state),
			stateItems
		},
		showBoundingArea
		{
			"Show Bounding Area", &defaultFace(),
			elem.dPad()->showBounds(),
			[this](BoolMenuItem &item)
			{
				elem.dPad()->setShowBounds(renderer(), item.flipBoolValue(*this));
				vCtrl.place();
				postDraw();
			}
		},
		remove
		{
			"Remove This D-Pad", &defaultFace(),
			[this](const Input::Event &e)
			{
				pushAndShowModal(makeView<YesNoAlertView>("Really remove this d-pad?",
					YesNoAlertView::Delegates
					{
						.onYes = [this]
						{
							vCtrl.remove(elem);
							vCtrl.place();
							confView.reloadItems();
							dismiss();
						}
					}), e);
			}
		},
		actionsHeading{"D-Pad Actions", &defaultBoldFace()},
		actions
		{
			{
				"Up", app().inputManager.toString(elem.dPad()->config.keys[0]), &defaultFace(),
				[this](const Input::Event &e) { assignAction(0, e); }
			},
			{
				"Right", app().inputManager.toString(elem.dPad()->config.keys[1]), &defaultFace(),
				[this](const Input::Event &e) { assignAction(1, e); }
			},
			{
				"Down", app().inputManager.toString(elem.dPad()->config.keys[2]), &defaultFace(),
				[this](const Input::Event &e) { assignAction(2, e); }
			},
			{
				"Left", app().inputManager.toString(elem.dPad()->config.keys[3]), &defaultFace(),
				[this](const Input::Event &e) { assignAction(3, e); }
			}
		} {}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		drawVControllerElement(cmds, elem, window().isPortrait());
		TableView::draw(cmds);
	}

	void onShow() final
	{
		vCtrl.applyButtonAlpha(.75);
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	TouchConfigView &confView;
	TextMenuItem deadzoneItems[4];
	MultiChoiceMenuItem deadzone;
	TextMenuItem diagonalSensitivityItems[6];
	MultiChoiceMenuItem diagonalSensitivity;
	TextMenuItem stateItems[3];
	MultiChoiceMenuItem state;
	BoolMenuItem showBoundingArea;
	TextMenuItem remove;
	TextHeadingMenuItem actionsHeading;
	DualTextMenuItem actions[4];
	std::array<MenuItem*, 10> item{&state, &deadzone, &diagonalSensitivity, &showBoundingArea, &remove,
		&actionsHeading, &actions[0], &actions[1], &actions[2], &actions[3]};

	void assignAction(int idx, const Input::Event &e)
	{
		auto multiChoiceView = makeViewWithName<TextTableView>("Assign Action", 16);
		addCategories(app(), elem, [&](const KeyCategory &cat)
		{
			for(auto &k : cat.keys)
			{
				multiChoiceView->appendItem(app().inputManager.toString(k),
					[this, k](TextMenuItem &item, View &parentView, const Input::Event &)
					{
						elem.dPad()->config.keys[item.id()] = k;
						actions[item.id()].set2ndName(app().inputManager.toString(k));
						parentView.dismiss();
					}).setId(idx);
			}
		});
		pushAndShow(std::move(multiChoiceView), e);
	}
};

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
			"Action", app().inputManager.toString(btn_.key), &defaultFace(),
			[this](const Input::Event &e)
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Assign Action", 16);
				addCategories(app(), elem, [&](const KeyCategory &cat)
				{
					for(auto &k : cat.keys)
					{
						multiChoiceView->appendItem(app().inputManager.toString(k),
							[this, k](View &parentView)
							{
								btn.key = k;
								btn.enabled = vCtrl.keyIsEnabled(k);
								key.set2ndName(app().inputManager.toString(k));
								turbo.setBoolValue(k.flags.turbo, *this);
								toggle.setBoolValue(k.flags.toggle, *this);
								vCtrl.update(elem);
								onChange.callSafe();
								vCtrl.place();
								parentView.dismiss();
							});
					}
				});
				pushAndShow(std::move(multiChoiceView), e);
			}
		},
		turbo
		{
			"Turbo", &defaultFace(),
			bool(btn_.key.flags.turbo),
			[this](BoolMenuItem &item)
			{
				btn.key.flags.turbo = item.flipBoolValue(*this);
				key.set2ndName(app().inputManager.toString(btn.key));
				key.compile2nd(renderer());
				onChange.callSafe();
			}
		},
		toggle
		{
			"Toggle", &defaultFace(),
			bool(btn_.key.flags.toggle),
			[this](BoolMenuItem &item)
			{
				btn.key.flags.toggle = item.flipBoolValue(*this);
				key.set2ndName(app().inputManager.toString(btn.key));
				key.compile2nd(renderer());
				onChange.callSafe();
			}
		},
		remove
		{
			"Remove This Button", &defaultFace(),
			[this](const Input::Event &e)
			{
				pushAndShowModal(makeView<YesNoAlertView>("Really remove this button?",
					YesNoAlertView::Delegates
					{
						.onYes = [this]
						{
							elem.remove(btn);
							onChange.callSafe();
							vCtrl.place();
							dismiss();
						}
					}), e);
			}
		}
	{
		reloadItems();
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	VControllerButton &btn;
	OnChange onChange;
	DualTextMenuItem key;
	BoolMenuItem turbo;
	BoolMenuItem toggle;
	TextMenuItem remove;
	std::vector<MenuItem*> item;

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&key);
		if(!btn.key.flags.appCode)
		{
			item.emplace_back(&turbo);
			item.emplace_back(&toggle);
		}
		item.emplace_back(&remove);
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
					elem.state = VControllerState(item.id());
					vCtrl.place();
				}
			},
			MenuItem::Id(elem.state),
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
					vCtrl.place();
				}
			},
			MenuItem::Id(elem.rowSize()),
			rowSizeItems
		},
		spaceItems
		{
			{"1mm", &defaultFace(), 1},
			{"2mm", &defaultFace(), 2},
			{"3mm", &defaultFace(), 3},
			{"4mm", &defaultFace(), 4},
			{"Custom Value", &defaultFace(),
				[this](const Input::Event &e)
				{
					app().pushAndShowNewCollectValueRangeInputView<int, 0, 8>(attachParams(), e, "Input 0 to 8", "",
						[this](EmuApp &app, auto val)
						{
							elem.buttonGroup()->setSpacing(val, window());
							vCtrl.place();
							space.setSelected(MenuItem::Id(val), *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, MenuItem::DEFAULT_ID
			},
		},
		space
		{
			"Spacing", &defaultFace(),
			{
				.onSetDisplayString = [this](auto idx, Gfx::Text &t)
				{
					t.resetString(std::format("{}mm", elem.buttonGroup()->spacing()));
					return true;
				},
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
					elem.buttonGroup()->layout.xPadding = item.id();
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->layout.xPadding : 0},
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
					elem.buttonGroup()->layout.yPadding = item.id();
					vCtrl.place();
				}
			},
			MenuItem::Id{elem.buttonGroup() ? elem.buttonGroup()->layout.yPadding : 0},
			extraYSizeItems
		},
		showBoundingArea
		{
			"Show Bounding Area", &defaultFace(),
			elem.buttonGroup() ? elem.buttonGroup()->showsBounds() : false,
			[this](BoolMenuItem &item)
			{
				elem.buttonGroup()->setShowBounds(item.flipBoolValue(*this));
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
					for(auto &k : cat.keys)
					{
						multiChoiceView->appendItem(app().inputManager.toString(k),
							[this, k](View &parentView, const Input::Event &e)
							{
								elem.add(k);
								vCtrl.update(elem);
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
				pushAndShowModal(makeView<YesNoAlertView>("Really remove this button group?",
					YesNoAlertView::Delegates
					{
						.onYes = [this]
						{
							vCtrl.remove(elem);
							vCtrl.place();
							confView.reloadItems();
							dismiss();
						}
					}), e);
			}
		},
		buttonsHeading{"Buttons In Group", &defaultBoldFace()}
	{
		reloadItems();
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds) final
	{
		drawVControllerElement(cmds, elem, window().isPortrait());
		TableView::draw(cmds);
	}

	void onShow() final
	{
		vCtrl.applyButtonAlpha(.75);
	}

private:
	VController &vCtrl;
	VControllerElement &elem;
	TouchConfigView &confView;
	TextMenuItem stateItems[3];
	MultiChoiceMenuItem state;
	TextMenuItem rowSizeItems[5];
	MultiChoiceMenuItem rowSize;
	TextMenuItem spaceItems[5];
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
				[this, &c](const Input::Event &e){ add(c); });
		}
		buttons.emplace_back(
			rightUIComponents.name, &defaultFace(),
			[this](const Input::Event &e){ add(rightUIComponents); });
		buttons.emplace_back(
			leftUIComponents.name, &defaultFace(),
			[this](const Input::Event &e){ add(leftUIComponents); });
	}

private:
	VController &vCtrl;
	TouchConfigView &confView;
	std::vector<TextMenuItem> buttons;

	void add(const InputComponentDesc &desc)
	{
		vCtrl.add(desc);
		vCtrl.place();
		confView.reloadItems();
		dismiss();
	}
};

void TouchConfigView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	vController.draw(cmds, true);
	TableView::draw(cmds);
}

void TouchConfigView::place()
{
	refreshTouchConfigMenu();
	TableView::place();
}

void TouchConfigView::refreshTouchConfigMenu()
{
	alpha.setSelected((MenuItem::Id)vController.buttonAlpha(), *this);
	touchCtrl.setSelected((int)vController.gamepadControlsVisibility(), *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.setSelected((int)vController.inputPlayer(), *this);
	size.setSelected((MenuItem::Id)vController.buttonSize(), *this);
	if(app().vibrationManager().hasVibrator())
	{
		vibrate.setBoolValue(vController.vibrateOnTouchInput(), *this);
	}
	showOnTouch.setBoolValue(vController.showOnTouchInput(), *this);
}

TouchConfigView::TouchConfigView(ViewAttachParams attach, VController &vCtrl):
	TableView{"On-screen Input Setup", attach, item},
	vController{vCtrl},
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
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setGamepadControlsVisibility(VControllerVisibility(item.id())); }
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
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setInputPlayer(item.id()); }
		},
		int(vCtrl.inputPlayer()),
		std::span{pointerInputItem, size_t(EmuSystem::maxPlayers)}
	},
	sizeItem
	{
		{"6.5mm", &defaultFace(), 650},
		{"7mm",   &defaultFace(), 700},
		{"7.5mm", &defaultFace(), 750},
		{"8mm",   &defaultFace(), 800},
		{"8.5mm", &defaultFace(), 850},
		{"9mm",   &defaultFace(), 900},
		{"10mm",  &defaultFace(), 1000},
		{"12mm",  &defaultFace(), 1200},
		{"14mm",  &defaultFace(), 1400},
		{"15mm",  &defaultFace(), 1500},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input 3.0 to 30.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 100.0;
						if(vController.setButtonSize(scaledIntVal))
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
				t.resetString(std::format("{:g}mm", vController.buttonSize() / 100.));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setButtonSize(item.id()); }
		},
		(MenuItem::Id)vController.buttonSize(),
		sizeItem
	},
	vibrate
	{
		"Vibration", &defaultFace(),
		vController.vibrateOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController.setVibrateOnTouchInput(app(), item.flipBoolValue(*this));
		}
	},
	showOnTouch
	{
		"Show Gamepad If Screen Touched", &defaultFace(),
		vController.showOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController.setShowOnTouchInput(item.flipBoolValue(*this));
		}
	},
	highlightPushedButtons
	{
		"Highlight Pushed Buttons", &defaultFace(),
		vController.highlightPushedButtons,
		[this](BoolMenuItem &item)
		{
			vController.highlightPushedButtons = item.flipBoolValue(*this);
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
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setButtonAlpha(item.id()); }
		},
		(MenuItem::Id)vController.buttonAlpha(),
		alphaItem
	},
	btnPlace
	{
		"Set Button Positions", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<PlaceVControlsView>(vController), e);
		}
	},
	placeVideo
	{
		"Set Video Position", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			pushAndShowModal(makeView<PlaceVideoView>(app().videoLayer(), vController), e);
		}
	},
	addButton
	{
		"Add New Button Group", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<AddNewButtonView>(*this, vController), e);
		}
	},
	allowButtonsPastContentBounds
	{
		"Allow Buttons In Display Cutout Area", &defaultFace(),
		vController.allowButtonsPastContentBounds(),
		[this](BoolMenuItem &item)
		{
			vController.setAllowButtonsPastContentBounds(item.flipBoolValue(*this));
			vController.place();
		}
	},
	resetEmuPositions
	{
		"Reset Emulated Device Positions", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Reset buttons to default positions?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						vController.resetEmulatedDevicePositions();
						vController.place();
					}
				}), e);
		}
	},
	resetEmuGroups
	{
		"Reset Emulated Device Groups", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Reset buttons groups to default?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						vController.resetEmulatedDeviceGroups();
						vController.place();
						reloadItems();
					}
				}), e);
		}
	},
	resetUIPositions
	{
		"Reset UI Positions", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Reset buttons to default positions?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						vController.resetUIPositions();
						vController.place();
					}
				}), e);
		}
	},
	resetUIGroups
	{
		"Reset UI Groups", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Reset buttons groups to default?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						vController.resetUIGroups();
						vController.place();
						reloadItems();
					}
				}), e);
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
	elementItems.reserve(vController.deviceElements().size() + vController.guiElements().size());
	for(auto &elem : vController.deviceElements())
	{
		auto &i = elementItems.emplace_back(
			elem.name(app()), &defaultFace(),
			[this, &elem](const Input::Event &e)
			{
				visit(overloaded
				{
					[&](VControllerDPad &){ pushAndShow(makeView<DPadElementConfigView>(*this, vController, elem), e); },
					[&](VControllerButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController, elem), e); },
					[](auto &){}
				}, elem);
			});
		item.emplace_back(&i);
	}
	item.emplace_back(&uiButtonsHeading);
	for(auto &elem : vController.guiElements())
	{
		auto &i = elementItems.emplace_back(
			elem.name(app()), &defaultFace(),
			[this, &elem](const Input::Event &e)
			{
				visit(overloaded
				{
					[&](VControllerUIButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController, elem), e); },
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
	item.emplace_back(&highlightPushedButtons);
	item.emplace_back(&alpha);
	item.emplace_back(&resetEmuPositions);
	item.emplace_back(&resetEmuGroups);
	item.emplace_back(&resetUIPositions);
	item.emplace_back(&resetUIGroups);
}

void TouchConfigView::onShow()
{
	vController.applyButtonAlpha(.75);
}

}
