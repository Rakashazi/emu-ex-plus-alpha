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
#include <emuframework/viewUtils.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/variant.hh>
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

static void addCategories(EmuApp&, VControllerElement &elem, auto &&addCategory)
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

class DPadElementConfigView : public TableView, public EmuAppHelper
{
public:
	DPadElementConfigView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_, VControllerElement &elem_):
		TableView{"Edit D-Pad", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		confView{confView_},
		deadzoneItems
		{
			{"1mm",    attach, {.id = 100}},
			{"1.35mm", attach, {.id = 135}},
			{"1.6mm",  attach, {.id = 160}},
			{"Custom Value", attach,
				[this](const Input::Event &e)
				{
					pushAndShowNewCollectValueRangeInputView<float, 1, 3>(attachParams(), e, "Input 1.0 to 3.0", "",
						[this](CollectTextInputView &, auto val)
						{
							int scaledIntVal = val * 100.0;
							elem.dPad()->setDeadzone(renderer(), scaledIntVal, window());
							deadzone.setSelected(MenuId{scaledIntVal}, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, {.id = defaultMenuId}
			},
		},
		deadzone
		{
			"Deadzone", attach,
			MenuId{elem.dPad()->deadzone()},
			deadzoneItems,
			{
				.onSetDisplayString = [this](auto, Gfx::Text& t)
				{
					t.resetString(std::format("{:g}mm", elem.dPad()->deadzone() / 100.));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item) { elem.dPad()->setDeadzone(renderer(), item.id, window()); }
			},
		},
		diagonalSensitivityItems
		{
			{"None",             attach, {.id = 1000}},
			{"33% (Low)",        attach, {.id = 667}},
			{"43% (Medium-Low)", attach, {.id = 570}},
			{"50% (Medium)",     attach, {.id = 500}},
			{"60% (High)",       attach, {.id = 400}},
			{"Custom Value", attach,
				[this](const Input::Event &e)
				{
					pushAndShowNewCollectValueRangeInputView<float, 0, 99>(attachParams(), e, "Input 0 to 99.0", "",
						[this](CollectTextInputView &, auto val)
						{
							val = 100. - val;
							int scaledIntVal = val * 10.0;
							val /= 100.;
							elem.dPad()->setDiagonalSensitivity(renderer(), val);
							diagonalSensitivity.setSelected(MenuId{scaledIntVal}, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, {.id = defaultMenuId}
			},
		},
		diagonalSensitivity
		{
			"Diagonal Sensitivity", attach,
			MenuId{elem.dPad()->diagonalSensitivity() * 1000.f},
			diagonalSensitivityItems,
			{
				.onSetDisplayString = [this](auto, Gfx::Text& t)
				{
					t.resetString(std::format("{:g}%", 100.f - elem.dPad()->diagonalSensitivity() * 100.f));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item) { elem.dPad()->setDiagonalSensitivity(renderer(), float(item.id) / 1000.f); }
			},
		},
		stateItems
		{
			{ctrlStateStr[0], attach, {.id = VControllerState::OFF}},
			{ctrlStateStr[1], attach, {.id = VControllerState::SHOWN}},
			{ctrlStateStr[2], attach, {.id = VControllerState::HIDDEN}},
		},
		state
		{
			"State", attach,
			MenuId{elem.state},
			stateItems,
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.state = VControllerState(item.id.val);
					vCtrl.place();
				}
			},
		},
		showBoundingArea
		{
			"Show Bounding Area", attach,
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
			"Remove This D-Pad", attach,
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
		actionsHeading{"D-Pad Actions", attach},
		actions
		{
			{
				"Up", app().inputManager.toString(elem.dPad()->config.keys[0]), attach,
				[this](const Input::Event &e) { assignAction(0, e); }
			},
			{
				"Right", app().inputManager.toString(elem.dPad()->config.keys[1]), attach,
				[this](const Input::Event &e) { assignAction(1, e); }
			},
			{
				"Down", app().inputManager.toString(elem.dPad()->config.keys[2]), attach,
				[this](const Input::Event &e) { assignAction(2, e); }
			},
			{
				"Left", app().inputManager.toString(elem.dPad()->config.keys[3]), attach,
				[this](const Input::Event &e) { assignAction(3, e); }
			}
		} {}

	void draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const final
	{
		vCtrl.draw(cmds, elem, true);
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
		auto &app = this->app();
		addCategories(app, elem, [&](const KeyCategory &cat)
		{
			for(auto &k : cat.keys)
			{
				multiChoiceView->appendItem(app.inputManager.toString(k),
					[this, k](TextMenuItem &item, View &parentView, const Input::Event &)
					{
						elem.dPad()->config.keys[item.id] = k;
						actions[item.id].set2ndName(this->app().inputManager.toString(k));
						parentView.dismiss();
					}).id = idx;
			}
		});
		pushAndShow(std::move(multiChoiceView), e);
	}
};

class ButtonElementConfigView : public TableView, public EmuAppHelper
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
			"Action", app().inputManager.toString(btn_.key), attach,
			[this](const Input::Event &e)
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Assign Action", 16);
				auto &app = this->app();
				addCategories(app, elem, [&](const KeyCategory &cat)
				{
					for(auto &k : cat.keys)
					{
						multiChoiceView->appendItem(app.inputManager.toString(k),
							[this, k](View &parentView)
							{
								btn.key = k;
								btn.enabled = vCtrl.keyIsEnabled(k);
								key.set2ndName(this->app().inputManager.toString(k));
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
			"Turbo", attach,
			bool(btn_.key.flags.turbo),
			[this](BoolMenuItem &item)
			{
				btn.key.flags.turbo = item.flipBoolValue(*this);
				key.set2ndName(app().inputManager.toString(btn.key));
				key.place2nd();
				onChange.callSafe();
			}
		},
		toggle
		{
			"Toggle", attach,
			bool(btn_.key.flags.toggle),
			[this](BoolMenuItem &item)
			{
				btn.key.flags.toggle = item.flipBoolValue(*this);
				key.set2ndName(app().inputManager.toString(btn.key));
				key.place2nd();
				onChange.callSafe();
			}
		},
		remove
		{
			"Remove This Button", attach,
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

class ButtonGroupElementConfigView : public TableView, public EmuAppHelper
{
public:
	ButtonGroupElementConfigView(ViewAttachParams attach, TouchConfigView &confView_, VController &vCtrl_, VControllerElement &elem_):
		TableView{"Edit Buttons", attach, item},
		vCtrl{vCtrl_},
		elem{elem_},
		confView{confView_},
		stateItems
		{
			{ctrlStateStr[0], attach, {.id = VControllerState::OFF}},
			{ctrlStateStr[1], attach, {.id = VControllerState::SHOWN}},
			{ctrlStateStr[2], attach, {.id = VControllerState::HIDDEN}},
		},
		state
		{
			"State", attach,
			MenuId{elem.state},
			stateItems,
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.state = VControllerState(item.id.val);
					vCtrl.place();
				}
			},
		},
		rowSizeItems
		{
			{"1", attach, {.id = 1}},
			{"2", attach, {.id = 2}},
			{"3", attach, {.id = 3}},
			{"4", attach, {.id = 4}},
			{"5", attach, {.id = 5}},
		},
		rowSize
		{
			"Buttons Per Row", attach,
			MenuId{elem.rowSize()},
			rowSizeItems,
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.setRowSize(item.id);
					vCtrl.place();
				}
			},
		},
		spaceItems
		{
			{"1mm", attach, {.id = 1}},
			{"2mm", attach, {.id = 2}},
			{"3mm", attach, {.id = 3}},
			{"4mm", attach, {.id = 4}},
			{"Custom Value", attach,
				[this](const Input::Event &e)
				{
					pushAndShowNewCollectValueRangeInputView<int, 0, 8>(attachParams(), e, "Input 0 to 8", "",
						[this](CollectTextInputView &, auto val)
						{
							elem.buttonGroup()->setSpacing(val, window());
							vCtrl.place();
							space.setSelected(MenuId{val}, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, {.id = defaultMenuId}
			},
		},
		space
		{
			"Spacing", attach,
			MenuId{elem.buttonGroup() ? elem.buttonGroup()->spacing() : 0},
			spaceItems,
			{
				.onSetDisplayString = [this](auto, Gfx::Text& t)
				{
					t.resetString(std::format("{}mm", elem.buttonGroup()->spacing()));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setSpacing(item.id, window());
					vCtrl.place();
				}
			},
		},
		staggerItems
		{
			{"-0.75x V", attach, {.id = 0}},
			{"-0.5x V",  attach, {.id = 1}},
			{"0",        attach, {.id = 2}},
			{"0.5x V",   attach, {.id = 3}},
			{"0.75x V",  attach, {.id = 4}},
			{"1x H&V",   attach, {.id = 5}},
		},
		stagger
		{
			"Stagger", attach,
			MenuId{elem.buttonGroup() ? elem.buttonGroup()->stagger() : 0},
			staggerItems,
			{
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->setStaggerType(item.id);
					vCtrl.place();
				}
			},
		},
		extraXSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], attach, {.id = touchCtrlExtraBtnSizeMenuVal[0]}},
			{touchCtrlExtraBtnSizeMenuName[1], attach, {.id = touchCtrlExtraBtnSizeMenuVal[1]}},
			{touchCtrlExtraBtnSizeMenuName[2], attach, {.id = touchCtrlExtraBtnSizeMenuVal[2]}},
			{touchCtrlExtraBtnSizeMenuName[3], attach, {.id = touchCtrlExtraBtnSizeMenuVal[3]}},
			{"Custom Value", attach, [this](const Input::Event &e)
				{
					pushAndShowNewCollectValueRangeInputView<int, 0, 30>(attachParams(), e, "Input 0 to 30", "",
						[this](CollectTextInputView &, auto val)
						{
							elem.buttonGroup()->layout.xPadding = val;
							vCtrl.place();
							extraXSize.setSelected(MenuId{val}, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, {.id = defaultMenuId}
			}
		},
		extraXSize
		{
			"Extended H Bounds", attach,
			MenuId{elem.buttonGroup() ? elem.buttonGroup()->layout.xPadding : 0},
			extraXSizeItems,
			{
				.onSetDisplayString = [this](auto idx, Gfx::Text &t)
				{
					if(!idx)
						return false;
					t.resetString(std::format("{}%", elem.buttonGroup()->layout.xPadding));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->layout.xPadding = item.id;
					vCtrl.place();
				}
			},
		},
		extraYSizeItems
		{
			{touchCtrlExtraBtnSizeMenuName[0], attach, {.id = touchCtrlExtraBtnSizeMenuVal[0]}},
			{touchCtrlExtraBtnSizeMenuName[1], attach, {.id = touchCtrlExtraBtnSizeMenuVal[1]}},
			{touchCtrlExtraBtnSizeMenuName[2], attach, {.id = touchCtrlExtraBtnSizeMenuVal[2]}},
			{touchCtrlExtraBtnSizeMenuName[3], attach, {.id = touchCtrlExtraBtnSizeMenuVal[3]}},
			{"Custom Value", attach, [this](const Input::Event &e)
				{
					pushAndShowNewCollectValueRangeInputView<int, 0, 30>(attachParams(), e, "Input 0 to 30", "",
						[this](CollectTextInputView &, auto val)
						{
							elem.buttonGroup()->layout.yPadding = val;
							vCtrl.place();
							extraYSize.setSelected(MenuId{val}, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, {.id = defaultMenuId}
			}
		},
		extraYSize
		{
			"Extended V Bounds", attach,
			MenuId{elem.buttonGroup() ? elem.buttonGroup()->layout.yPadding : 0},
			extraYSizeItems,
			{
				.onSetDisplayString = [this](auto idx, Gfx::Text &t)
				{
					if(!idx)
						return false;
					t.resetString(std::format("{}%", elem.buttonGroup()->layout.yPadding));
					return true;
				},
				.defaultItemOnSelect = [this](TextMenuItem &item)
				{
					elem.buttonGroup()->layout.yPadding = item.id;
					vCtrl.place();
				}
			},
		},
		showBoundingArea
		{
			"Show Bounding Area", attach,
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
			"Add Button To This Group", attach,
			[this](const Input::Event &e)
			{
				auto multiChoiceView = makeViewWithName<TextTableView>("Add Button", 16);
				auto &app = this->app();
				addCategories(app, elem, [&](const KeyCategory &cat)
				{
					for(auto &k : cat.keys)
					{
						multiChoiceView->appendItem(app.inputManager.toString(k),
							[this, k](View &parentView, const Input::Event&)
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
			"Remove This Button Group", attach,
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
		buttonsHeading{"Buttons In Group", attach}
	{
		reloadItems();
	}

	void draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const final
	{
		vCtrl.draw(cmds, elem, true);
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
	TextMenuItem extraXSizeItems[5];
	MultiChoiceMenuItem extraXSize;
	TextMenuItem extraYSizeItems[5];
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
				btn.name(app().inputManager), attachParams(),
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

class AddNewButtonView : public TableView, public EmuAppHelper
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
				c.name, attach,
				[this, &c]{ add(c); });
		}
		buttons.emplace_back(
			rightUIComponents.name, attach,
			[this]{ add(rightUIComponents); });
		buttons.emplace_back(
			leftUIComponents.name, attach,
			[this]{ add(leftUIComponents); });
		buttons.emplace_back(
			rewindUIComponents.name, attach,
			[this]{ add(rewindUIComponents); });
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

void TouchConfigView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
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
	alpha.setSelected(MenuId{vController.buttonAlpha()}, *this);
	touchCtrl.setSelected((int)vController.gamepadControlsVisibility(), *this);
	if(EmuSystem::maxPlayers > 1)
		player.setSelected((int)vController.inputPlayer(), *this);
	size.setSelected(MenuId{vController.buttonSize()}, *this);
	if(app().vibrationManager.hasVibrator())
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
		{"Off",  attach, {.id = VControllerVisibility::OFF}},
		{"On",   attach, {.id = VControllerVisibility::ON}},
		{"Auto", attach, {.id = VControllerVisibility::AUTO}}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", attach,
		int(vCtrl.gamepadControlsVisibility()),
		touchCtrlItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setGamepadControlsVisibility(VControllerVisibility(item.id.val)); }
		},
	},
	playerItems
	{
		[&] -> DynArray<TextMenuItem>
		{
			if(EmuSystem::maxPlayers == 1)
				return {};
			DynArray<TextMenuItem> items{size_t(EmuSystem::maxPlayers)};
			for(auto i : iotaCount(EmuSystem::maxPlayers))
			{
				items[i] = {playerNumStrings[i], attach, {.id = i}};
			}
			return items;
		}()
	},
	player
	{
		"Virtual Gamepad Player", attach,
		int(vCtrl.inputPlayer()),
		playerItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setInputPlayer(item.id); }
		},
	},
	sizeItem
	{
		{"6.5mm", attach, {.id = 650}},
		{"7mm",   attach, {.id = 700}},
		{"7.5mm", attach, {.id = 750}},
		{"8mm",   attach, {.id = 800}},
		{"8.5mm", attach, {.id = 850}},
		{"9mm",   attach, {.id = 900}},
		{"10mm",  attach, {.id = 1000}},
		{"12mm",  attach, {.id = 1200}},
		{"14mm",  attach, {.id = 1400}},
		{"15mm",  attach, {.id = 1500}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<float, 3, 30>(attachParams(), e, "Input 3.0 to 30.0", "",
					[this](CollectTextInputView &, auto val)
					{
						int scaledIntVal = val * 100.0;
						vController.setButtonSize(scaledIntVal);
						size.setSelected(MenuId{scaledIntVal}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	size
	{
		"Button Size", attach,
		MenuId{vController.buttonSize()},
		sizeItem,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(std::format("{:g}mm", vController.buttonSize() / 100.));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setButtonSize(item.id); }
		},
	},
	vibrate
	{
		"Vibration", attach,
		vController.vibrateOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController.setVibrateOnTouchInput(app(), item.flipBoolValue(*this));
		}
	},
	showOnTouch
	{
		"Show Gamepad If Screen Touched", attach,
		vController.showOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController.setShowOnTouchInput(item.flipBoolValue(*this));
		}
	},
	highlightPushedButtons
	{
		"Highlight Pushed Buttons", attach,
		vController.highlightPushedButtons,
		[this](BoolMenuItem &item)
		{
			vController.highlightPushedButtons = item.flipBoolValue(*this);
		}
	},
	alphaItem
	{
		{"0%",  attach, {.id = 0}},
		{"10%", attach, {.id = int(255. * .1)}},
		{"25%", attach, {.id = int(255. * .25)}},
		{"50%", attach, {.id = int(255. * .5)}},
		{"65%", attach, {.id = int(255. * .65)}},
		{"75%", attach, {.id = int(255. * .75)}},
	},
	alpha
	{
		"Blend Amount", attach,
		MenuId{vController.buttonAlpha()},
		alphaItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item){ vController.setButtonAlpha(item.id); }
		},
	},
	btnPlace
	{
		"Set Button Positions", attach,
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<PlaceVControlsView>(vController), e);
		}
	},
	placeVideo
	{
		"Set Video Position", attach,
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			pushAndShowModal(makeView<PlaceVideoView>(app().videoLayer, vController), e);
		}
	},
	addButton
	{
		"Add New Button Group", attach,
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<AddNewButtonView>(*this, vController), e);
		}
	},
	allowButtonsPastContentBounds
	{
		"Allow Buttons In Display Cutout Area", attach,
		vController.allowButtonsPastContentBounds(),
		[this](BoolMenuItem &item)
		{
			vController.setAllowButtonsPastContentBounds(item.flipBoolValue(*this));
			vController.place();
		}
	},
	resetEmuPositions
	{
		"Reset Emulated Device Positions", attach,
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
		"Reset Emulated Device Groups", attach,
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
		"Reset UI Positions", attach,
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
		"Reset UI Groups", attach,
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
		"Emulated Device Button Groups", attach
	},
	uiButtonsHeading
	{
		"UI Button Groups", attach
	},
	otherHeading
	{
		"Other Options", attach
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
		item.emplace_back(&player);
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
			elem.name(app().inputManager), attachParams(),
			[this, &elem](const Input::Event &e)
			{
				elem.visit(overloaded
				{
					[&](VControllerDPad &){ pushAndShow(makeView<DPadElementConfigView>(*this, vController, elem), e); },
					[&](VControllerButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController, elem), e); },
					[](auto &){}
				});
			});
		item.emplace_back(&i);
	}
	item.emplace_back(&uiButtonsHeading);
	for(auto &elem : vController.guiElements())
	{
		auto &i = elementItems.emplace_back(
			elem.name(app().inputManager), attachParams(),
			[this, &elem](const Input::Event &e)
			{
				elem.visit(overloaded
				{
					[&](VControllerUIButtonGroup &){ pushAndShow(makeView<ButtonGroupElementConfigView>(*this, vController, elem), e); },
					[](auto &){}
				});
			});
		item.emplace_back(&i);
	}
	item.emplace_back(&otherHeading);
	item.emplace_back(&addButton);
	if(used(allowButtonsPastContentBounds) && appContext().hasDisplayCutout())
	{
		item.emplace_back(&allowButtonsPastContentBounds);
	}
	if(app().vibrationManager.hasVibrator())
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
