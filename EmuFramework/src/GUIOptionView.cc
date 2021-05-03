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

#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include "EmuOptions.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/Renderer.hh>

static constexpr bool USE_MOBILE_ORIENTATION_NAMES = Config::envIsAndroid || Config::envIsIOS;
static const char *landscapeName = USE_MOBILE_ORIENTATION_NAMES ? "Landscape" : "90 Left";
static const char *landscape2Name = USE_MOBILE_ORIENTATION_NAMES ? "Landscape 2" : "90 Right";
static const char *portraitName = USE_MOBILE_ORIENTATION_NAMES ? "Portrait" : "Standard";
static const char *portrait2Name = USE_MOBILE_ORIENTATION_NAMES ? "Portrait 2" : "Upside Down";

static void setMenuOrientation(unsigned val, Base::Window &win, Gfx::Renderer &r)
{
	optionMenuOrientation = val;
	r.setWindowValidOrientations(win, optionMenuOrientation);
	logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
}

static void setGameOrientation(unsigned val)
{
	optionGameOrientation = val;
	logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
}

GUIOptionView::GUIOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"GUI Options", attach, item},
	pauseUnfocused
	{
		"Pause if unfocused", &defaultFace(),
		(bool)optionPauseUnfocused,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionPauseUnfocused = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2", &defaultFace(), [this]() { setFontSize(2000); }},
		{"3", &defaultFace(), [this]() { setFontSize(3000); }},
		{"4", &defaultFace(), [this]() { setFontSize(4000); }},
		{"5", &defaultFace(), [this]() { setFontSize(5000); }},
		{"6", &defaultFace(), [this]() { setFontSize(6000); }},
		{"7", &defaultFace(), [this]() { setFontSize(7000); }},
		{"8", &defaultFace(), [this]() { setFontSize(8000); }},
		{"9", &defaultFace(), [this]() { setFontSize(9000); }},
		{"10", &defaultFace(), [this]() { setFontSize(10000); }},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 1000.0;
						if(optionFontSize.isValidVal(scaledIntVal))
						{
							setFontSize(scaledIntVal);
							fontSize.setSelected(std::size(fontSizeItem) - 1, *this);
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
			}
		},
	},
	fontSize
	{
		"Font Size", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<6>("%.2f", optionFontSize / 1000.).data());
			return true;
		},
		[]()
		{
			switch(optionFontSize)
			{
				case 2000: return 0;
				case 3000: return 1;
				case 4000: return 2;
				case 5000: return 3;
				case 6000: return 4;
				case 7000: return 5;
				case 8000: return 6;
				case 9000: return 7;
				case 10000: return 8;
				default: return 9;
			}
		}(),
		fontSizeItem
	},
	notificationIcon
	{
		"Suspended App Icon", &defaultFace(),
		(bool)optionNotificationIcon,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionNotificationIcon = item.flipBoolValue(*this);
		}
	},
	statusBarItem
	{
		{
			"Off", &defaultFace(),
			[this]()
			{
				optionHideStatusBar = 0;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"In Game", &defaultFace(),
			[this]()
			{
				optionHideStatusBar = 1;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"On", &defaultFace(),
			[this]()
			{
				optionHideStatusBar = 2;
				app().applyOSNavStyle(appContext(), false);
			}
		}
	},
	statusBar
	{
		"Hide Status Bar", &defaultFace(),
		optionHideStatusBar,
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{
			"Off", &defaultFace(),
			[this]()
			{
				optionLowProfileOSNav = 0;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"In Game", &defaultFace(),
			[this]()
			{
				optionLowProfileOSNav = 1;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"On", &defaultFace(),
			[this]()
			{
				optionLowProfileOSNav = 2;
				app().applyOSNavStyle(appContext(), false);
			}
		}
	},
	lowProfileOSNav
	{
		"Dim OS UI", &defaultFace(),
		optionLowProfileOSNav,
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{
			"Off", &defaultFace(),
			[this]()
			{
				optionHideOSNav = 0;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"In Game", &defaultFace(),
			[this]()
			{
				optionHideOSNav = 1;
				app().applyOSNavStyle(appContext(), false);
			}
		},
		{
			"On", &defaultFace(),
			[this]()
			{
				optionHideOSNav = 2;
				app().applyOSNavStyle(appContext(), false);
			}
		}
	},
	hideOSNav
	{
		"Hide OS Navigation", &defaultFace(),
		optionHideOSNav,
		hideOSNavItem
	},
	idleDisplayPowerSave
	{
		"Allow Screen Timeout In Emulation", &defaultFace(),
		(bool)optionIdleDisplayPowerSave,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionIdleDisplayPowerSave = item.flipBoolValue(*this);
			appContext().setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
		}
	},
	navView
	{
		"Title Bar", &defaultFace(),
		(bool)optionTitleBar,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTitleBar = item.flipBoolValue(*this);
			app().viewController().showNavView(optionTitleBar);
			app().viewController().placeElements();
		}
	},
	backNav
	{
		"Title Back Navigation", &defaultFace(),
		attach.viewManager().needsBackControl(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			manager().setNeedsBackControl(item.flipBoolValue(*this));
			app().viewController().setShowNavViewBackButton(manager().needsBackControl());
			app().viewController().placeElements();
		}
	},
	systemActionsIsDefaultMenu
	{
		"Default Menu", &defaultFace(),
		(bool)optionSystemActionsIsDefaultMenu,
		"Last Used", "System Actions",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSystemActionsIsDefaultMenu = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Games", &defaultFace(),
		(bool)optionShowBundledGames,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBundledGames = item.flipBoolValue(*this);
			app().dispatchOnMainMenuItemOptionChanged();
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items", &defaultFace(),
		(bool)optionShowBluetoothScan,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBluetoothScan = item.flipBoolValue(*this);
			app().dispatchOnMainMenuItemOptionChanged();
		}
	},
	orientationHeading
	{
		"Orientation", &defaultBoldFace()
	},
	menuOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", &defaultFace(), [this](){ setMenuOrientation(Base::VIEW_ROTATE_AUTO, window(), renderer()); }},
		#endif
		{landscapeName, &defaultFace(), [this](){ setMenuOrientation(Base::VIEW_ROTATE_90, window(), renderer()); }},
		{landscape2Name, &defaultFace(), [this](){ setMenuOrientation(Base::VIEW_ROTATE_270, window(), renderer()); }},
		{portraitName, &defaultFace(), [this](){ setMenuOrientation(Base::VIEW_ROTATE_0, window(), renderer()); }},
		{portrait2Name, &defaultFace(), [this](){ setMenuOrientation(Base::VIEW_ROTATE_180, window(), renderer()); }},
	},
	menuOrientation
	{
		"In Menu", &defaultFace(),
		[]()
		{
			int itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
			switch(optionMenuOrientation)
			{
				default: return 0;
				case Base::VIEW_ROTATE_90: return 1 - itemOffset;
				case Base::VIEW_ROTATE_270: return 2 - itemOffset;
				case Base::VIEW_ROTATE_0: return 3 - itemOffset;
				case Base::VIEW_ROTATE_180: return 4 - itemOffset;
			}
		}(),
		menuOrientationItem
	},
	gameOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", &defaultFace(), [](){ setGameOrientation(Base::VIEW_ROTATE_AUTO); }},
		#endif
		{landscapeName, &defaultFace(), [](){ setGameOrientation(Base::VIEW_ROTATE_90); }},
		{landscape2Name, &defaultFace(), [](){ setGameOrientation(Base::VIEW_ROTATE_270); }},
		{portraitName, &defaultFace(), [](){ setGameOrientation(Base::VIEW_ROTATE_0); }},
		{portrait2Name, &defaultFace(), [](){ setGameOrientation(Base::VIEW_ROTATE_180); }},
	},
	gameOrientation
	{
		"In Game", &defaultFace(),
		[]()
		{
			int itemOffset = Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 0 : 1;
			switch(optionGameOrientation)
			{
				default: return 0;
				case Base::VIEW_ROTATE_90: return 1 - itemOffset;
				case Base::VIEW_ROTATE_270: return 2 - itemOffset;
				case Base::VIEW_ROTATE_0: return 3 - itemOffset;
				case Base::VIEW_ROTATE_180: return 4 - itemOffset;
			}
		}(),
		gameOrientationItem
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void GUIOptionView::loadStockItems()
{
	if(!optionPauseUnfocused.isConst)
	{
		item.emplace_back(&pauseUnfocused);
	}
	if(!optionNotificationIcon.isConst)
	{
		item.emplace_back(&notificationIcon);
	}
	if(!optionTitleBar.isConst)
	{
		item.emplace_back(&navView);
	}
	if(View::needsBackControlIsMutable)
	{
		item.emplace_back(&backNav);
	}
	item.emplace_back(&systemActionsIsDefaultMenu);
	if(!optionFontSize.isConst)
	{
		item.emplace_back(&fontSize);
	}
	if(!optionIdleDisplayPowerSave.isConst)
	{
		item.emplace_back(&idleDisplayPowerSave);
	}
	if(!optionLowProfileOSNav.isConst)
	{
		item.emplace_back(&lowProfileOSNav);
	}
	if(!optionHideOSNav.isConst)
	{
		item.emplace_back(&hideOSNav);
	}
	if(!optionHideStatusBar.isConst)
	{
		item.emplace_back(&statusBar);
	}
	if(EmuSystem::hasBundledGames)
	{
		item.emplace_back(&showBundledGames);
	}
	#ifdef CONFIG_BLUETOOTH
	item.emplace_back(&showBluetoothScan);
	#endif
	if(!optionGameOrientation.isConst)
	{
		item.emplace_back(&orientationHeading);
		item.emplace_back(&gameOrientation);
		item.emplace_back(&menuOrientation);
	}
}

void GUIOptionView::setFontSize(uint16_t val)
{
	optionFontSize = val;
	setupFont(manager(), renderer(), window());
	app().viewController().placeElements();
}
