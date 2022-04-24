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

namespace EmuEx
{

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
		(bool)app().pauseUnfocusedOption(),
		[this](BoolMenuItem &item)
		{
			app().pauseUnfocusedOption() = item.flipBoolValue(*this);
		}
	},
	fontSizeItem
	{
		{"2",  &defaultFace(), setFontSizeDel(), 2000},
		{"3",  &defaultFace(), setFontSizeDel(), 3000},
		{"4",  &defaultFace(), setFontSizeDel(), 4000},
		{"5",  &defaultFace(), setFontSizeDel(), 5000},
		{"6",  &defaultFace(), setFontSizeDel(), 6000},
		{"7",  &defaultFace(), setFontSizeDel(), 7000},
		{"8",  &defaultFace(), setFontSizeDel(), 8000},
		{"9",  &defaultFace(), setFontSizeDel(), 9000},
		{"10", &defaultFace(), setFontSizeDel(), 10000},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 2.0 to 10.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 1000.0;
						if(app.setFontSize(scaledIntVal))
						{
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
		[this](auto idx, Gfx::Text &t)
		{
			t.setString(fmt::format("{:.2f}", app().fontSize() / 1000.));
			return true;
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
		{"Off",    &defaultFace(), setStatusBarDel(), (int)Tristate::OFF},
		{"In Emu", &defaultFace(), setStatusBarDel(), (int)Tristate::IN_EMU},
		{"On",     &defaultFace(), setStatusBarDel(), (int)Tristate::ON}
	},
	statusBar
	{
		"Hide Status Bar", &defaultFace(),
		(MenuItem::Id)app().hideStatusBarMode(),
		statusBarItem
	},
	lowProfileOSNavItem
	{
		{"Off",    &defaultFace(), setLowProfileOSNavDel(), (int)Tristate::OFF},
		{"In Emu", &defaultFace(), setLowProfileOSNavDel(), (int)Tristate::IN_EMU},
		{"On",     &defaultFace(), setLowProfileOSNavDel(), (int)Tristate::ON}
	},
	lowProfileOSNav
	{
		"Dim OS UI", &defaultFace(),
		(MenuItem::Id)app().lowProfileOSNavMode(),
		lowProfileOSNavItem
	},
	hideOSNavItem
	{
		{"Off",    &defaultFace(), setHideOSNavDel(), (int)Tristate::OFF},
		{"In Emu", &defaultFace(), setHideOSNavDel(), (int)Tristate::IN_EMU},
		{"On",     &defaultFace(), setHideOSNavDel(), (int)Tristate::ON}
	},
	hideOSNav
	{
		"Hide OS Navigation", &defaultFace(),
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
		attach.viewManager().needsBackControl(),
		[this](BoolMenuItem &item)
		{
			manager().setNeedsBackControl(item.flipBoolValue(*this));
			app().viewController().setShowNavViewBackButton(manager().needsBackControl());
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
		app().showHiddenFilesInPicker(),
		[this](BoolMenuItem &item)
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
		{"Auto", &defaultFace(), setMenuOrientationDel(), IG::VIEW_ROTATE_AUTO},
		#endif
		{landscapeName,  &defaultFace(), setMenuOrientationDel(), IG::VIEW_ROTATE_90},
		{landscape2Name, &defaultFace(), setMenuOrientationDel(), IG::VIEW_ROTATE_270},
		{portraitName,   &defaultFace(), setMenuOrientationDel(), IG::VIEW_ROTATE_0},
		{portrait2Name,  &defaultFace(), setMenuOrientationDel(), IG::VIEW_ROTATE_180},
	},
	menuOrientation
	{
		"In Menu", &defaultFace(),
		(MenuItem::Id)app().menuOrientation(),
		menuOrientationItem
	},
	emuOrientationItem
	{
		#ifdef CONFIG_BASE_SUPPORTS_ORIENTATION_SENSOR
		{"Auto", &defaultFace(), setEmuOrientationDel(), IG::VIEW_ROTATE_AUTO},
		#endif
		{landscapeName,  &defaultFace(), setEmuOrientationDel(), IG::VIEW_ROTATE_90},
		{landscape2Name, &defaultFace(), setEmuOrientationDel(), IG::VIEW_ROTATE_270},
		{portraitName,   &defaultFace(), setEmuOrientationDel(), IG::VIEW_ROTATE_0},
		{portrait2Name,  &defaultFace(), setEmuOrientationDel(), IG::VIEW_ROTATE_180},
	},
	emuOrientation
	{
		"In Emu", &defaultFace(),
		(MenuItem::Id)app().emuOrientation(),
		emuOrientationItem
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
	if(EmuSystem::hasBundledGames)
	{
		item.emplace_back(&showBundledGames);
	}
	#ifdef CONFIG_BLUETOOTH
	item.emplace_back(&showBluetoothScan);
	#endif
	item.emplace_back(&showHiddenFiles);
	item.emplace_back(&orientationHeading);
	item.emplace_back(&emuOrientation);
	item.emplace_back(&menuOrientation);
}

TextMenuItem::SelectDelegate GUIOptionView::setMenuOrientationDel()
{
	return [this](TextMenuItem &item) { app().setMenuOrientation(item.id()); };
}

TextMenuItem::SelectDelegate GUIOptionView::setEmuOrientationDel()
{
	return [this](TextMenuItem &item) { app().setEmuOrientation(item.id()); };
}

TextMenuItem::SelectDelegate GUIOptionView::setFontSizeDel()
{
	return [this](TextMenuItem &item) { app().setFontSize(item.id()); };
}

TextMenuItem::SelectDelegate GUIOptionView::setStatusBarDel()
{
	return [this](TextMenuItem &item) { app().setHideStatusBarMode((Tristate)item.id()); };
}

TextMenuItem::SelectDelegate GUIOptionView::setLowProfileOSNavDel()
{
	return [this](TextMenuItem &item) { app().setLowProfileOSNavMode((Tristate)item.id()); };
}

TextMenuItem::SelectDelegate GUIOptionView::setHideOSNavDel()
{
	return [this](TextMenuItem &item) { app().setHideOSNavMode((Tristate)item.id()); };
}

}
