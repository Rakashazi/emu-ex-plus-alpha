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
#include "private.hh"
#include <imagine/base/Base.hh>

static constexpr bool USE_MOBILE_ORIENTATION_NAMES = Config::envIsAndroid || Config::envIsIOS;
static const char *landscapeName = USE_MOBILE_ORIENTATION_NAMES ? "Landscape" : "90 Left";
static const char *landscape2Name = USE_MOBILE_ORIENTATION_NAMES ? "Landscape 2" : "90 Right";
static const char *portraitName = USE_MOBILE_ORIENTATION_NAMES ? "Portrait" : "Standard";
static const char *portrait2Name = USE_MOBILE_ORIENTATION_NAMES ? "Portrait 2" : "Upside Down";

static void setMenuOrientation(uint val, Base::Window &win)
{
	optionMenuOrientation = val;
	emuVideo.renderer().setWindowValidOrientations(win, optionMenuOrientation);
	logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
}

static void setGameOrientation(uint val)
{
	optionGameOrientation = val;
	logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
}

GUIOptionView::GUIOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"GUI Options", attach, item},
	pauseUnfocused
	{
		"Pause if unfocused",
		(bool)optionPauseUnfocused,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionPauseUnfocused = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2", [this]() { setFontSize(2000); }},
		{"3", [this]() { setFontSize(3000); }},
		{"4", [this]() { setFontSize(4000); }},
		{"5", [this]() { setFontSize(5000); }},
		{"6", [this]() { setFontSize(6000); }},
		{"7", [this]() { setFontSize(7000); }},
		{"8", [this]() { setFontSize(8000); }},
		{"9", [this]() { setFontSize(9000); }},
		{"10", [this]() { setFontSize(10000); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](auto val)
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
							EmuApp::postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	fontSize
	{
		"Font Size",
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
		"Suspended App Icon",
		(bool)optionNotificationIcon,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionNotificationIcon = item.flipBoolValue(*this);
		}
	},
	statusBarItem
	{
		{
			"Off",
			[this]()
			{
				optionHideStatusBar = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionHideStatusBar = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionHideStatusBar = 2;
				applyOSNavStyle(false);
			}
		}
	},
	statusBar
	{
		"Hide Status Bar",
		optionHideStatusBar,
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{
			"Off",
			[this]()
			{
				optionLowProfileOSNav = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionLowProfileOSNav = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionLowProfileOSNav = 2;
				applyOSNavStyle(false);
			}
		}
	},
	lowProfileOSNav
	{
		"Dim OS UI",
		optionLowProfileOSNav,
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{
			"Off",
			[this]()
			{
				optionHideOSNav = 0;
				applyOSNavStyle(false);
			}
		},
		{
			"In Game",
			[this]()
			{
				optionHideOSNav = 1;
				applyOSNavStyle(false);
			}
		},
		{
			"On",
			[this]()
			{
				optionHideOSNav = 2;
				applyOSNavStyle(false);
			}
		}
	},
	hideOSNav
	{
		"Hide OS Navigation",
		optionHideOSNav,
		hideOSNavItem
	},
	idleDisplayPowerSave
	{
		"Allow Screen Timeout In Emulation",
		(bool)optionIdleDisplayPowerSave,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionIdleDisplayPowerSave = item.flipBoolValue(*this);
			Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
		}
	},
	navView
	{
		"Title Bar",
		(bool)optionTitleBar,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTitleBar = item.flipBoolValue(*this);
			emuViewController.showNavView(optionTitleBar);
			emuViewController.placeElements();
		}
	},
	backNav
	{
		"Title Back Navigation",
		View::needsBackControl,
		[this](BoolMenuItem &item, Input::Event e)
		{
			View::setNeedsBackControl(item.flipBoolValue(*this));
			emuViewController.setShowNavViewBackButton(View::needsBackControl);
			emuViewController.placeElements();
		}
	},
	systemActionsIsDefaultMenu
	{
		"Default Menu",
		(bool)optionSystemActionsIsDefaultMenu,
		"Last Used", "System Actions",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSystemActionsIsDefaultMenu = item.flipBoolValue(*this);
		}
	},
	showBundledGames
	{
		"Show Bundled Games",
		(bool)optionShowBundledGames,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBundledGames = item.flipBoolValue(*this);
			onMainMenuItemOptionChanged();
		}
	},
	showBluetoothScan
	{
		"Show Bluetooth Menu Items",
		(bool)optionShowBluetoothScan,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionShowBluetoothScan = item.flipBoolValue(*this);
			onMainMenuItemOptionChanged();
		}
	},
	orientationHeading
	{
		"Orientation",
	},
	menuOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", [this](){ setMenuOrientation(Base::VIEW_ROTATE_AUTO, window()); }},
		#endif
		{landscapeName, [this](){ setMenuOrientation(Base::VIEW_ROTATE_90, window()); }},
		{landscape2Name, [this](){ setMenuOrientation(Base::VIEW_ROTATE_270, window()); }},
		{portraitName, [this](){ setMenuOrientation(Base::VIEW_ROTATE_0, window()); }},
		{portrait2Name, [this](){ setMenuOrientation(Base::VIEW_ROTATE_180, window()); }},
	},
	menuOrientation
	{
		"In Menu",
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
		{"Auto", [](){ setGameOrientation(Base::VIEW_ROTATE_AUTO); }},
		#endif
		{landscapeName, [](){ setGameOrientation(Base::VIEW_ROTATE_90); }},
		{landscape2Name, [](){ setGameOrientation(Base::VIEW_ROTATE_270); }},
		{portraitName, [](){ setGameOrientation(Base::VIEW_ROTATE_0); }},
		{portrait2Name, [](){ setGameOrientation(Base::VIEW_ROTATE_180); }},
	},
	gameOrientation
	{
		"In Game",
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
	if(!View::needsBackControlIsConst)
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
	setupFont(renderer(), window());
	emuViewController.placeElements();
}
