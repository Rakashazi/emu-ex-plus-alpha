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

#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextEntry.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/RecentGameView.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/OptionView.hh>
#include "EmuOptions.hh"
#include <emuframework/InputManagerView.hh>
#include <emuframework/TouchConfigView.hh>
#include <emuframework/BundledGamesView.hh>
#include "private.hh"
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
#ifdef CONFIG_BLUETOOTH
extern BluetoothAdapter *bta;
#endif

class ResetAlertView : public BaseAlertView
{
public:
	ResetAlertView(ViewAttachParams attach, const char *label):
		BaseAlertView(attach, label,
			[this](const TableView &)
			{
				return 3;
			},
			[this](const TableView &, int idx) -> MenuItem&
			{
				switch(idx)
				{
					default: bug_unreachable("idx == %d", idx); [[fallthrough]];
					case 0: return soft;
					case 1: return hard;
					case 2: return cancel;
				}
			}),
		soft
		{
			"Soft Reset",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				dismiss();
				EmuSystem::reset(EmuSystem::RESET_SOFT);
				startGameFromMenu();
			}
		},
		hard
		{
			"Hard Reset",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				dismiss();
				EmuSystem::reset(EmuSystem::RESET_HARD);
				startGameFromMenu();
			}
		},
		cancel
		{
			"Cancel",
			[this](TextMenuItem &, View &view, Input::Event e)
			{
				dismiss();
			}
		}
	{}

protected:
	TextMenuItem soft, hard, cancel;
};

#ifdef CONFIG_BLUETOOTH

static bool initBTAdapter()
{
	if(bta)
	{
		return true;
	}

	logMsg("initializing Bluetooth");
	bta = BluetoothAdapter::defaultAdapter();
	return bta;
}

static auto onScanStatus =
	[](BluetoothAdapter &, uint status, int arg)
	{
		switch(status)
		{
			bcase BluetoothAdapter::INIT_FAILED:
			{
				if(Config::envIsIOS)
				{
					popup.postError("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
				}
			}
			bcase BluetoothAdapter::SCAN_FAILED:
			{
				popup.postError("Scan failed");
			}
			bcase BluetoothAdapter::SCAN_NO_DEVS:
			{
				popup.post("No devices found");
			}
			bcase BluetoothAdapter::SCAN_PROCESSING:
			{
				popup.printf(2, 0, "Checking %d device(s)...", arg);
			}
			bcase BluetoothAdapter::SCAN_NAME_FAILED:
			{
				popup.postError("Failed reading a device name");
			}
			bcase BluetoothAdapter::SCAN_COMPLETE:
			{
				int devs = Bluetooth::pendingDevs();
				if(devs)
				{
					popup.printf(2, 0, "Connecting to %d device(s)...", devs);
					Bluetooth::connectPendingDevs(bta);
				}
				else
				{
					popup.post("Scan complete, no recognized devices");
				}
			}
			/*bcase BluetoothAdapter::SOCKET_OPEN_FAILED:
			{
				popup.postError("Failed opening a Bluetooth connection");
			}*/
		}
	};

static void handledFailedBTAdapterInit(ViewAttachParams attach, Input::Event e)
{
	popup.postError("Unable to initialize Bluetooth adapter");
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	if(!FS::exists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
	{
		auto &ynAlertView = *new YesNoAlertView{attach, "BTstack not found, open Cydia and install?"};
		ynAlertView.setOnYes(
			[](TextMenuItem &, View &view, Input::Event)
			{
				view.dismiss();
				logMsg("launching Cydia");
				Base::openURL("cydia://package/ch.ringwald.btstack");
			});
		modalViewController.pushAndShow(ynAlertView, e, false);
	}
	#endif
}

#endif

void EmuMainMenuView::onShow()
{
	logMsg("refreshing main menu state");
	recentGames.setActive(recentGameList.size());
	systemActions.setActive(EmuSystem::gameIsRunning());
	#ifdef CONFIG_BLUETOOTH
	bluetoothDisconnect.setActive(Bluetooth::devsConnected());
	#endif
}

void EmuMainMenuView::loadFileBrowserItems()
{
	item.emplace_back(&loadGame);
	item.emplace_back(&recentGames);
	if(EmuSystem::hasBundledGames && optionShowBundledGames)
	{
		item.emplace_back(&bundledGames);
	}
}

void EmuMainMenuView::loadStandardItems()
{
	item.emplace_back(&systemActions);
	if(!Config::MACHINE_IS_OUYA)
	{
		item.emplace_back(&onScreenInputManager);
	}
	item.emplace_back(&inputManager);
	item.emplace_back(&options);
	#ifdef CONFIG_BLUETOOTH
	if(optionShowBluetoothScan)
	{
		item.emplace_back(&scanWiimotes);
		#ifdef CONFIG_BLUETOOTH_SERVER
		item.emplace_back(&acceptPS3ControllerConnection);
		#endif
		item.emplace_back(&bluetoothDisconnect);
	}
	#endif
	item.emplace_back(&benchmark);
	item.emplace_back(&about);
	item.emplace_back(&exitApp);
}

EmuMainMenuView::EmuMainMenuView(ViewAttachParams attach, bool customMenu):
	TableView{appViewTitle(), attach, item},
	loadGame
	{
		"Load Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &fPicker = *EmuFilePicker::makeForLoading(attachParams(), e);
			pushAndShow(fPicker, e, false);
		}
	},
	systemActions
	{
		"System Actions",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(!EmuSystem::gameIsRunning())
				return;
			pushAndShow(*makeView(attachParams(), EmuApp::ViewID::SYSTEM_ACTIONS), e);
		}
	},
	recentGames
	{
		"Recent Games",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(recentGameList.size())
			{
				auto &rMenu = *new RecentGameView{attachParams()};
				pushAndShow(rMenu, e);
			}
		}
	},
	bundledGames
	{
		"Bundled Games",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &bMenu = *new BundledGamesView{attachParams()};
			pushAndShow(bMenu, e);
		}
	},
	options
	{
		"Options",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &oMenu = *new OptionCategoryView{attachParams()};
			pushAndShow(oMenu, e);
		}
	},
	onScreenInputManager
	{
		"On-screen Input Setup",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &tcMenu = *new TouchConfigView{attachParams(), EmuSystem::inputFaceBtnName, EmuSystem::inputCenterBtnName};
			pushAndShow(tcMenu, e);
		}
	},
	inputManager
	{
		"Key/Gamepad Input Setup",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &menu = *new InputManagerView{attachParams()};
			pushAndShow(menu, e);
		}
	},
	benchmark
	{
		"Benchmark Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &fPicker = *EmuFilePicker::makeForBenchmarking(attachParams(), e);
			pushAndShow(fPicker, e, false);
		}
	},
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes
	{
		"Scan for Wiimotes/iCP/JS1",
		[this](TextMenuItem &t, View &, Input::Event e)
		{
			if(initBTAdapter())
			{
				if(Bluetooth::scanForDevices(*bta, onScanStatus))
				{
					popup.post("Starting Scan...\n(see website for device-specific help)", 4);
				}
				else
				{
					popup.post("Still scanning", 1);
				}
			}
			else
			{
				handledFailedBTAdapterInit(attachParams(), e);
			}
			postDraw();
		}
	},
	bluetoothDisconnect
	{
		"Disconnect Bluetooth",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(Bluetooth::devsConnected())
			{
				string_printf(bluetoothDisconnectStr, "Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected());
				auto &ynAlertView = *new YesNoAlertView{attachParams(), bluetoothDisconnectStr.data()};
				ynAlertView.setOnYes(
					[](TextMenuItem &, View &view, Input::Event e)
					{
						view.dismiss();
						Bluetooth::closeBT(bta);
					});
				modalViewController.pushAndShow(ynAlertView, e, false);
			}
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SERVER
	acceptPS3ControllerConnection
	{
		"Scan for PS3 Controller",
		[this](TextMenuItem &t, View &, Input::Event e)
		{
			if(initBTAdapter())
			{
				popup.post("Prepare to push the PS button", 4);
				auto startedScan = Bluetooth::listenForDevices(*bta,
					[this](BluetoothAdapter &bta, uint status, int arg)
					{
						switch(status)
						{
							bcase BluetoothAdapter::INIT_FAILED:
							{
								popup.postError(Config::envIsLinux ? "Unable to register server, make sure this executable has cap_net_bind_service enabled and bluetoothd isn't running" :
									"Bluetooth setup failed", Config::envIsLinux ? 8 : 2);
							}
							bcase BluetoothAdapter::SCAN_COMPLETE:
							{
								popup.post("Push the PS button on your controller\n(see website for pairing help)", 4);
							}
							bdefault: onScanStatus(bta, status, arg);
						}
					});
				if(!startedScan)
				{
					popup.post("Still scanning", 1);
				}
			}
			else
			{
				handledFailedBTAdapterInit(attachParams(), e);
			}
			postDraw();
		}
	},
	#endif
	about
	{
		"About",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &credits = *new CreditsView{EmuSystem::creditsViewStr, attachParams()};
			pushAndShow(credits, e);
		}
	},
	exitApp
	{
		"Exit",
		[](TextMenuItem &, View &, Input::Event e)
		{
			Base::exit();
		}
	}
{
	if(!customMenu)
	{
		loadFileBrowserItems();
		loadStandardItems();
		EmuApp::setOnMainMenuItemOptionChanged(
			[this]()
			{
				item.clear();
				loadFileBrowserItems();
				loadStandardItems();
			});
	}
}

OptionCategoryView::OptionCategoryView(ViewAttachParams attach):
	TableView
	{
		"Options",
		attach,
		[this](const TableView &) { return hasGooglePlayStoreFeatures() ? IG::size(subConfig) : IG::size(subConfig)-1; },
		[this](const TableView &, uint idx) -> MenuItem& { return subConfig[idx]; }
	},
	subConfig
	{
		{
			"Video",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *makeView(attachParams(), EmuApp::ViewID::VIDEO_OPTIONS);
				pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"Audio",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *makeView(attachParams(), EmuApp::ViewID::AUDIO_OPTIONS);
				pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"System",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *makeView(attachParams(), EmuApp::ViewID::SYSTEM_OPTIONS);
				pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"GUI",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *makeView(attachParams(), EmuApp::ViewID::GUI_OPTIONS);
				pushAndShow(oCategoryMenu, e);
			}
		}
	}
{
	if(hasGooglePlayStoreFeatures())
	{
		subConfig[IG::size(subConfig)-1] =
		{
			"Beta Testing Opt-in/out",
			[this]()
			{
				Base::openURL(string_makePrintf<96>("https://play.google.com/apps/testing/%s", appID()).data());
			}
		};
	}
}

View *makeView(ViewAttachParams attach, EmuApp::ViewID id)
{
	auto view = EmuApp::makeCustomView(attach, id);
	if(view)
		return view;
	switch(id)
	{
		case EmuApp::ViewID::MAIN_MENU: return new EmuMainMenuView(attach);
		case EmuApp::ViewID::SYSTEM_ACTIONS: return new EmuSystemActionsView(attach);
		case EmuApp::ViewID::VIDEO_OPTIONS: return new VideoOptionView(attach);
		case EmuApp::ViewID::AUDIO_OPTIONS: return new AudioOptionView(attach);
		case EmuApp::ViewID::SYSTEM_OPTIONS: return new SystemOptionView(attach);
		case EmuApp::ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		default:
			bug_unreachable("Tried to make non-existing view ID:%d", (int)id);
			return nullptr;
	}
}
