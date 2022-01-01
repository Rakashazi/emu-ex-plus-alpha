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
#include <imagine/util/format.hh>

static constexpr bool USE_MOBILE_ORIENTATION_NAMES = Config::envIsAndroid || Config::envIsIOS;
static const char *landscapeName = USE_MOBILE_ORIENTATION_NAMES ? "Landscape" : "90 Left";
static const char *landscape2Name = USE_MOBILE_ORIENTATION_NAMES ? "Landscape 2" : "90 Right";
static const char *portraitName = USE_MOBILE_ORIENTATION_NAMES ? "Portrait" : "Standard";
static const char *portrait2Name = USE_MOBILE_ORIENTATION_NAMES ? "Portrait 2" : "Upside Down";

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
		{"2",  &defaultFace(), setFontSizeDel(2000)},
		{"3",  &defaultFace(), setFontSizeDel(3000)},
		{"4",  &defaultFace(), setFontSizeDel(4000)},
		{"5",  &defaultFace(), setFontSizeDel(5000)},
		{"6",  &defaultFace(), setFontSizeDel(6000)},
		{"7",  &defaultFace(), setFontSizeDel(7000)},
		{"8",  &defaultFace(), setFontSizeDel(8000)},
		{"9",  &defaultFace(), setFontSizeDel(9000)},
		{"10", &defaultFace(), setFontSizeDel(10000)},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 1000.0;
						if(optionFontSize.isValidVal(scaledIntVal))
						{
							app.setFontSize(scaledIntVal);
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
			t.setString(fmt::format("{:.2f}", optionFontSize / 1000.));
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
		{"Off",    &defaultFace(), setStatusBarDel(0)},
		{"In Emu", &defaultFace(), setStatusBarDel(1)},
		{"On",     &defaultFace(), setStatusBarDel(2)}
	},
	statusBar
	{
		"Hide Status Bar", &defaultFace(),
		optionHideStatusBar,
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{"Off",    &defaultFace(), setLowProfileOSNavDel(0)},
		{"In Emu", &defaultFace(), setLowProfileOSNavDel(1)},
		{"On",     &defaultFace(), setLowProfileOSNavDel(2)}
	},
	lowProfileOSNav
	{
		"Dim OS UI", &defaultFace(),
		optionLowProfileOSNav,
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{"Off",    &defaultFace(), setHideOSNavDel(0)},
		{"In Emu", &defaultFace(), setHideOSNavDel(1)},
		{"On",     &defaultFace(), setHideOSNavDel(2)}
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
		"Show Bundled Content", &defaultFace(),
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
	showHiddenFiles
	{
		"Show Hidden Files", &defaultFace(),
		app().showHiddenFilesInPicker(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			app().setShowHiddenFilesInPicker(item.flipBoolValue(*this));
		}
	},
	orientationHeading
	{
		"Orientation", &defaultBoldFace()
	},
	menuOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", &defaultFace(), setMenuOrientationDel(Base::VIEW_ROTATE_AUTO)},
		#endif
		{landscapeName,  &defaultFace(), setMenuOrientationDel(Base::VIEW_ROTATE_90)},
		{landscape2Name, &defaultFace(), setMenuOrientationDel(Base::VIEW_ROTATE_270)},
		{portraitName,   &defaultFace(), setMenuOrientationDel(Base::VIEW_ROTATE_0)},
		{portrait2Name,  &defaultFace(), setMenuOrientationDel(Base::VIEW_ROTATE_180)},
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
		{"Auto", &defaultFace(), setGameOrientationDel(Base::VIEW_ROTATE_AUTO)},
		#endif
		{landscapeName,  &defaultFace(), setGameOrientationDel(Base::VIEW_ROTATE_90)},
		{landscape2Name, &defaultFace(), setGameOrientationDel(Base::VIEW_ROTATE_270)},
		{portraitName,   &defaultFace(), setGameOrientationDel(Base::VIEW_ROTATE_0)},
		{portrait2Name,  &defaultFace(), setGameOrientationDel(Base::VIEW_ROTATE_180)},
	},
	gameOrientation
	{
		"In Emu", &defaultFace(),
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
	if(ViewManager::needsBackControlIsMutable)
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
	item.emplace_back(&showHiddenFiles);
	if(!optionGameOrientation.isConst)
	{
		item.emplace_back(&orientationHeading);
		item.emplace_back(&gameOrientation);
		item.emplace_back(&menuOrientation);
	}
}

TextMenuItem::SelectDelegate GUIOptionView::setMenuOrientationDel(int val)
{
	return [this, val]()
		{
			optionMenuOrientation = val;
			renderer().setWindowValidOrientations(window(), optionMenuOrientation);
			logMsg("set menu orientation: %s", Base::orientationToStr(int(optionMenuOrientation)));
		};
}

TextMenuItem::SelectDelegate GUIOptionView::setGameOrientationDel(int val)
{
	return [val]()
		{
			optionGameOrientation = val;
			logMsg("set game orientation: %s", Base::orientationToStr(int(optionGameOrientation)));
		};
}

TextMenuItem::SelectDelegate GUIOptionView::setFontSizeDel(uint16_t val)
{
	return [this, val]() { app().setFontSize(val); };
}

TextMenuItem::SelectDelegate GUIOptionView::setStatusBarDel(int val)
{
	return [this, val]()
		{
			optionHideStatusBar = val;
			app().applyOSNavStyle(appContext(), false);
		};
}

TextMenuItem::SelectDelegate GUIOptionView::setLowProfileOSNavDel(int val)
{
	return [this, val]()
		{
			optionLowProfileOSNav = val;
			app().applyOSNavStyle(appContext(), false);
		};
}

TextMenuItem::SelectDelegate GUIOptionView::setHideOSNavDel(int val)
{
	return [this, val]()
		{
			optionHideOSNav = val;
			app().applyOSNavStyle(appContext(), false);
		};
}
