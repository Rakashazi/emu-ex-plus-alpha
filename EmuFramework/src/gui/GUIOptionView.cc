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

#include <emuframework/GUIOptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/viewUtils.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>
#include <format>

namespace EmuEx
{

static constexpr bool USE_MOBILE_ORIENTATION_NAMES = Config::envIsAndroid || Config::envIsIOS;
static const char *landscapeName = USE_MOBILE_ORIENTATION_NAMES ? "Landscape" : "90° Left";
static const char *landscape2Name = USE_MOBILE_ORIENTATION_NAMES ? "Landscape 2" : "90° Right";
static const char *portraitName = USE_MOBILE_ORIENTATION_NAMES ? "Portrait" : "Standard";
static const char *portrait2Name = USE_MOBILE_ORIENTATION_NAMES ? "Portrait 2" : "Upside Down";

GUIOptionView::GUIOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"GUI Options", attach, item},
	pauseUnfocused
	{
		"Pause if unfocused", attach,
		app().pauseUnfocused,
		[this](BoolMenuItem &item)
		{
			app().pauseUnfocused = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2",  attach, {.id = 2000}},
		{"3",  attach, {.id = 3000}},
		{"4",  attach, {.id = 4000}},
		{"5",  attach, {.id = 5000}},
		{"6",  attach, {.id = 6000}},
		{"7",  attach, {.id = 7000}},
		{"8",  attach, {.id = 8000}},
		{"9",  attach, {.id = 9000}},
		{"10", attach, {.id = 10000}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<float, 2, 10>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](CollectTextInputView &, auto val)
					{
						int scaledIntVal = val * 1000.0;
						app().setFontSize(scaledIntVal);
						fontSize.setSelected(MenuId{scaledIntVal}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	fontSize
	{
		"Font Size", attach,
		MenuId{app().fontSize},
		fontSizeItem,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(std::format("{:g}", app().fontSize / 1000.));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setFontSize(item.id); }
		},
	},
	notificationIcon
	{
		"Suspended App Icon", attach,
		app().showsNotificationIcon,
		[this](BoolMenuItem &item)
		{
			app().showsNotificationIcon = item.flipBoolValue(*this);
		}
	},
	statusBarItem
	{
		{"Off",    attach, MenuItem::Config{.id = InEmuTristate::Off}},
		{"In Emu", attach, MenuItem::Config{.id = InEmuTristate::InEmu}},
		{"On",     attach, MenuItem::Config{.id = InEmuTristate::On}}
	},
	statusBar
	{
		"Hide Status Bar", attach,
		MenuId(InEmuTristate(app().hidesStatusBar.value())),
		statusBarItem,
		MultiChoiceMenuItem::Config
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setHideStatusBarMode(InEmuTristate(item.id.val)); }
		},
	},
	lowProfileOSNavItem
	{
		{"Off",    attach, MenuItem::Config{.id = InEmuTristate::Off}},
		{"In Emu", attach, MenuItem::Config{.id = InEmuTristate::InEmu}},
		{"On",     attach, MenuItem::Config{.id = InEmuTristate::On}}
	},
	lowProfileOSNav
	{
		"Dim OS UI", attach,
		MenuId(InEmuTristate(app().lowProfileOSNav.value())),
		lowProfileOSNavItem,
		MultiChoiceMenuItem::Config
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setLowProfileOSNavMode(InEmuTristate(item.id.val)); }
		},
	},
	hideOSNavItem
	{
		{"Off",    attach, MenuItem::Config{.id = InEmuTristate::Off}},
		{"In Emu", attach, MenuItem::Config{.id = InEmuTristate::InEmu}},
		{"On",     attach, MenuItem::Config{.id = InEmuTristate::On}}
	},
	hideOSNav
	{
		"Hide OS Navigation", attach,
		MenuId(InEmuTristate(app().hidesOSNav.value())),
		hideOSNavItem,
		MultiChoiceMenuItem::Config
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setHideOSNavMode(InEmuTristate(item.id.val)); }
		},
	},
	idleDisplayPowerSave
	{
		"Allow Screen Timeout In Emulation", attach,
		app().idleDisplayPowerSave,
		[this](BoolMenuItem &item)
		{
			app().setIdleDisplayPowerSave(item.flipBoolValue(*this));
		}
	},
	navView
	{
		"Title Bar", attach,
		app().showsTitleBar,
		[this](BoolMenuItem &item)
		{
			app().setShowsTitleBar(item.flipBoolValue(*this));
		}
	},
	backNav
	{
		"Title Back Navigation", attach,
		attach.viewManager.needsBackControl,
		[this](BoolMenuItem &item)
		{
			manager().needsBackControl = item.flipBoolValue(*this);
			app().viewController().setShowNavViewBackButton(manager().needsBackControl);
			app().viewController().placeElements();
		}
	},
	systemActionsIsDefaultMenu
	{
		"Default Menu", attach,
		app().systemActionsIsDefaultMenu,
		"Last Used", "System Actions",
		[this](BoolMenuItem &item)
		{
			app().systemActionsIsDefaultMenu = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Content", attach,
		app().showsBundledGames,
		[this](BoolMenuItem &item)
		{
			app().setShowsBundledGames(item.flipBoolValue(*this));
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items", attach,
		app().showsBluetoothScan,
		[this](BoolMenuItem &item)
		{
			app().setShowsBluetoothScanItems(item.flipBoolValue(*this));
		}
	},
	showHiddenFiles
	{
		"Show Hidden Files", attach,
		app().showHiddenFilesInPicker,
		[this](BoolMenuItem &item)
		{
			app().showHiddenFilesInPicker = item.flipBoolValue(*this);
		}
	},
	maxRecentContent
	{
		"Max Recent Content Items", std::to_string(app().recentContent.maxRecentContent), attach,
		[this](const Input::Event &e)
		{
			pushAndShowNewCollectValueRangeInputView<int, 1, 100>(attachParams(), e,
				"Input 1 to 100", std::to_string(app().recentContent.maxRecentContent),
				[this](CollectTextInputView &, auto val)
				{
					app().recentContent.maxRecentContent = val;
					maxRecentContent.set2ndName(std::to_string(val));
					return true;
				});
		}
	},
	orientationHeading
	{
		"Orientation", attach
	},
	menuOrientationItem
	{
		{"Auto",         attach, {.id = Orientations{}}},
		{landscapeName,  attach, {.id = Orientations{.landscapeRight = 1}}},
		{landscape2Name, attach, {.id = Orientations{.landscapeLeft = 1}}},
		{portraitName,   attach, {.id = Orientations{.portrait = 1}}},
		{portrait2Name,  attach, {.id = Orientations{.portraitUpsideDown = 1}}},
	},
	menuOrientation
	{
		"In Menu", attach,
		MenuId{uint8_t(app().menuOrientation.value())},
		menuOrientationItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setMenuOrientation(std::bit_cast<Orientations>(uint8_t(item.id))); }
		},
	},
	emuOrientationItem
	{
		{"Auto",         attach, {.id = Orientations{}}},
		{landscapeName,  attach, {.id = Orientations{.landscapeRight = 1}}},
		{landscape2Name, attach, {.id = Orientations{.landscapeLeft = 1}}},
		{portraitName,   attach, {.id = Orientations{.portrait = 1}}},
		{portrait2Name,  attach, {.id = Orientations{.portraitUpsideDown = 1}}},
	},
	emuOrientation
	{
		"In Emu", attach,
		MenuId{uint8_t(app().emuOrientation.value())},
		emuOrientationItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setEmuOrientation(std::bit_cast<Orientations>(uint8_t(item.id))); }
		},
	},
	layoutBehindSystemUI
	{
		"Display Behind OS UI", attach,
		app().doesLayoutBehindSystemUI(),
		[this](BoolMenuItem &item)
		{
			app().setLayoutBehindSystemUI(item.flipBoolValue(*this));
		}
	},
	setWindowSize
	{
		"Set Window Size", attach,
		[this](const Input::Event &e)
		{
			pushAndShowNewCollectValuePairRangeInputView<int, 320, 8192, 240, 8192>(attachParams(), e,
				"Input Width & Height", "",
				[this](CollectTextInputView &, auto val)
				{
					app().emuWindow().setSize({val.first, val.second});
					return true;
				});
		}
	},
	toggleFullScreen
	{
		"Toggle Full Screen", attach,
		[this]{ app().emuWindow().toggleFullScreen(); }
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void GUIOptionView::loadStockItems()
{
	if(used(pauseUnfocused))
	{
		item.emplace_back(&pauseUnfocused);
	}
	if(app().canShowNotificationIcon(appContext()))
	{
		item.emplace_back(&notificationIcon);
	}
	if(used(navView))
	{
		item.emplace_back(&navView);
	}
	if(ViewManager::needsBackControlIsMutable)
	{
		item.emplace_back(&backNav);
	}
	item.emplace_back(&systemActionsIsDefaultMenu);
	item.emplace_back(&fontSize);
	if(used(setWindowSize))
	{
		item.emplace_back(&setWindowSize);
	}
	if(used(toggleFullScreen))
	{
		item.emplace_back(&toggleFullScreen);
	}
	item.emplace_back(&idleDisplayPowerSave);
	if(used(lowProfileOSNav))
	{
		item.emplace_back(&lowProfileOSNav);
	}
	if(used(hideOSNav))
	{
		item.emplace_back(&hideOSNav);
	}
	if(used(statusBar))
	{
		item.emplace_back(&statusBar);
	}
	if(used(layoutBehindSystemUI) && appContext().hasTranslucentSysUI())
	{
		item.emplace_back(&layoutBehindSystemUI);
	}
	if(EmuSystem::hasBundledGames)
	{
		item.emplace_back(&showBundledGames);
	}
	if(used(showBluetoothScan))
		item.emplace_back(&showBluetoothScan);
	item.emplace_back(&showHiddenFiles);
	item.emplace_back(&maxRecentContent);
	item.emplace_back(&orientationHeading);
	item.emplace_back(&emuOrientation);
	item.emplace_back(&menuOrientation);
}

}
