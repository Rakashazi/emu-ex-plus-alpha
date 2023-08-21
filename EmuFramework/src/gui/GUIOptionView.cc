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
#include "../EmuOptions.hh"
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
		"Pause if unfocused", &defaultFace(),
		(bool)app().pauseUnfocusedOption(),
		[this](BoolMenuItem &item)
		{
			app().pauseUnfocusedOption() = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2",  &defaultFace(), 2000},
		{"3",  &defaultFace(), 3000},
		{"4",  &defaultFace(), 4000},
		{"5",  &defaultFace(), 5000},
		{"6",  &defaultFace(), 6000},
		{"7",  &defaultFace(), 7000},
		{"8",  &defaultFace(), 8000},
		{"9",  &defaultFace(), 9000},
		{"10", &defaultFace(), 10000},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<float>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 1000.0;
						if(app.setFontSize(scaledIntVal))
						{
							fontSize.setSelected((MenuItem::Id)scaledIntVal, *this);
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
	fontSize
	{
		"Font Size", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{:g}", app().fontSize() / 1000.));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setFontSize(item.id()); }
		},
		(MenuItem::Id)app().fontSize(),
		fontSizeItem
	},
	notificationIcon
	{
		"Suspended App Icon", &defaultFace(),
		(bool)app().notificationIconOption().val,
		[this](BoolMenuItem &item)
		{
			app().notificationIconOption() = item.flipBoolValue(*this);
		}
	},
	statusBarItem
	{
		{"Off",    &defaultFace(), to_underlying(Tristate::OFF)},
		{"In Emu", &defaultFace(), to_underlying(Tristate::IN_EMU)},
		{"On",     &defaultFace(), to_underlying(Tristate::ON)}
	},
	statusBar
	{
		"Hide Status Bar", &defaultFace(),
		MultiChoiceMenuItem::Delegates
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setHideStatusBarMode(Tristate(item.id())); }
		},
		(MenuItem::Id)app().hideStatusBarMode(),
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{"Off",    &defaultFace(), to_underlying(Tristate::OFF)},
		{"In Emu", &defaultFace(), to_underlying(Tristate::IN_EMU)},
		{"On",     &defaultFace(), to_underlying(Tristate::ON)}
	},
	lowProfileOSNav
	{
		"Dim OS UI", &defaultFace(),
		MultiChoiceMenuItem::Delegates
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setLowProfileOSNavMode(Tristate(item.id())); }
		},
		(MenuItem::Id)app().lowProfileOSNavMode(),
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{"Off",    &defaultFace(), to_underlying(Tristate::OFF)},
		{"In Emu", &defaultFace(), to_underlying(Tristate::IN_EMU)},
		{"On",     &defaultFace(), to_underlying(Tristate::ON)}
	},
	hideOSNav
	{
		"Hide OS Navigation", &defaultFace(),
		MultiChoiceMenuItem::Delegates
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setHideOSNavMode(Tristate(item.id())); }
		},
		(MenuItem::Id)app().hideOSNavMode(),
		hideOSNavItem
	},
	idleDisplayPowerSave
	{
		"Allow Screen Timeout In Emulation", &defaultFace(),
		app().idleDisplayPowerSave(),
		[this](BoolMenuItem &item)
		{
			app().setIdleDisplayPowerSave(item.flipBoolValue(*this));
		}
	},
	navView
	{
		"Title Bar", &defaultFace(),
		app().showsTitleBar(),
		[this](BoolMenuItem &item)
		{
			app().setShowsTitleBar(item.flipBoolValue(*this));
		}
	},
	backNav
	{
		"Title Back Navigation", &defaultFace(),
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
		"Default Menu", &defaultFace(),
		(bool)app().systemActionsIsDefaultMenuOption().val,
		"Last Used", "System Actions",
		[this](BoolMenuItem &item)
		{
			app().systemActionsIsDefaultMenuOption() = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Content", &defaultFace(),
		app().showsBundledGames(),
		[this](BoolMenuItem &item)
		{
			app().setShowsBundledGames(item.flipBoolValue(*this));
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items", &defaultFace(),
		app().showsBluetoothScanItems(),
		[this](BoolMenuItem &item)
		{
			app().setShowsBluetoothScanItems(item.flipBoolValue(*this));
		}
	},
	showHiddenFiles
	{
		"Show Hidden Files", &defaultFace(),
		app().showHiddenFilesInPicker,
		[this](BoolMenuItem &item)
		{
			app().showHiddenFilesInPicker = item.flipBoolValue(*this);
		}
	},
	maxRecentContent
	{
		"Max Recent Content Items", std::to_string(app().recentContent.maxRecentContent), &defaultFace(),
		[this](const Input::Event &e)
		{
			app().pushAndShowNewCollectValueRangeInputView<int, 1, 100>(attachParams(), e,
				"Input 1 to 100", std::to_string(app().recentContent.maxRecentContent),
				[this](EmuApp &app, auto val)
				{
					app.recentContent.maxRecentContent = val;
					maxRecentContent.set2ndName(std::to_string(val));
					return true;
				});
		}
	},
	orientationHeading
	{
		"Orientation", &defaultBoldFace()
	},
	menuOrientationItem
	{
		{"Auto",         &defaultFace(), Orientations{}},
		{landscapeName,  &defaultFace(), Orientations{.landscapeRight = 1}},
		{landscape2Name, &defaultFace(), Orientations{.landscapeLeft = 1}},
		{portraitName,   &defaultFace(), Orientations{.portrait = 1}},
		{portrait2Name,  &defaultFace(), Orientations{.portraitUpsideDown = 1}},
	},
	menuOrientation
	{
		"In Menu", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setMenuOrientation(std::bit_cast<Orientations>(uint8_t(item.id()))); }
		},
		MenuItem::Id(uint8_t(app().menuOrientation())),
		menuOrientationItem
	},
	emuOrientationItem
	{
		{"Auto",         &defaultFace(), Orientations{}},
		{landscapeName,  &defaultFace(), Orientations{.landscapeRight = 1}},
		{landscape2Name, &defaultFace(), Orientations{.landscapeLeft = 1}},
		{portraitName,   &defaultFace(), Orientations{.portrait = 1}},
		{portrait2Name,  &defaultFace(), Orientations{.portraitUpsideDown = 1}},
	},
	emuOrientation
	{
		"In Emu", &defaultFace(),
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().setEmuOrientation(std::bit_cast<Orientations>(uint8_t(item.id()))); }
		},
		MenuItem::Id(uint8_t(app().emuOrientation())),
		emuOrientationItem
	},
	layoutBehindSystemUI
	{
		"Display Behind OS UI", &defaultFace(),
		app().doesLayoutBehindSystemUI(),
		[this](BoolMenuItem &item)
		{
			app().setLayoutBehindSystemUI(item.flipBoolValue(*this));
		}
	},
	setWindowSize
	{
		"Set Window Size", &defaultFace(),
		[this](const Input::Event &e)
		{
			app().pushAndShowNewCollectValuePairRangeInputView<int, 320, 8192, 240, 8192>(attachParams(), e,
				"Input Width & Height", "",
				[this](EmuApp &app, auto val)
				{
					app.emuWindow().setSize({val.first, val.second});
					return true;
				});
		}
	},
	toggleFullScreen
	{
		"Toggle Full Screen", &defaultFace(),
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
	if(!app().pauseUnfocusedOption().isConst)
	{
		item.emplace_back(&pauseUnfocused);
	}
	if(!app().notificationIconOption().isConst)
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
