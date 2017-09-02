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
#include <emuframework/MenuView.hh>
#include <emuframework/Recent.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/EmuOptions.hh>
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

char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_unreachable("slot == %d", slot); return 0;
	}
}

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
		modalViewController.pushAndShow(ynAlertView, e);
	}
	#endif
}

#endif

void MenuView::onShow()
{
	logMsg("refreshing main menu state");
	recentGames.setActive(recentGameList.size());
	cheats.setActive(EmuSystem::gameIsRunning());
	reset.setActive(EmuSystem::gameIsRunning());
	saveState.setActive(EmuSystem::gameIsRunning());
	loadState.setActive(EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot));
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.compile(renderer(), projP);
	screenshot.setActive(EmuSystem::gameIsRunning());
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.setActive(EmuSystem::gameIsRunning());
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothDisconnect.setActive(Bluetooth::devsConnected());
	#endif
}

void MenuView::loadFileBrowserItems()
{
	item.emplace_back(&loadGame);
	item.emplace_back(&recentGames);
	if(EmuSystem::hasBundledGames && optionShowBundledGames)
	{
		item.emplace_back(&bundledGames);
	}
}

void MenuView::loadStandardItems()
{
	if(EmuSystem::hasCheats)
	{
		item.emplace_back(&cheats);
	}
	item.emplace_back(&reset);
	item.emplace_back(&loadState);
	item.emplace_back(&saveState);
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	item.emplace_back(&stateSlot);
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
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	item.emplace_back(&addLauncherIcon);
	#endif
	item.emplace_back(&benchmark);
	item.emplace_back(&screenshot);
	item.emplace_back(&about);
	item.emplace_back(&exitApp);
}

MenuView::MenuView(ViewAttachParams attach, bool customMenu):
	TableView{appViewTitle(), attach, item},
	loadGame
	{
		"Load Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &fPicker = *EmuFilePicker::makeForLoading(attachParams());
			pushAndShow(fPicker, e, false);
		}
	},
	cheats
	{
		"Cheats",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::LIST_CHEATS);
				viewStack.pushAndShow(cheatsMenu, e);
			}
		}
	},
	reset
	{
		"Reset",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(EmuSystem::hasResetModes)
				{
					auto &resetAlertView = *new ResetAlertView{attachParams(), "Really Reset?"};
					modalViewController.pushAndShow(resetAlertView, e);
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really Reset?"};
					ynAlertView.setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							EmuSystem::reset(EmuSystem::RESET_SOFT);
							startGameFromMenu();
						});
					modalViewController.pushAndShow(ynAlertView, e);
				}
			}
		}
	},
	loadState
	{
		"Load State",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active() && EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really Load State?"};
				ynAlertView.setOnYes(
					[](TextMenuItem &, View &view, Input::Event e)
					{
						view.dismiss();
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							popup.printf(4, true, "Load State: %s", err->what());
						}
						else
							startGameFromMenu();
					});
				modalViewController.pushAndShow(ynAlertView, e);
			}
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
	saveState
	{
		"Save State",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				static auto doSaveState =
					[]()
					{
						if(auto err = EmuApp::saveStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							popup.printf(4, true, "Save State: %s", err->what());
						}
						else
							startGameFromMenu();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really Overwrite State?"};
					ynAlertView.setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							doSaveState();
						});
					modalViewController.pushAndShow(ynAlertView, e);
				}
			}
		}
	},
	stateSlot
	{
		stateSlotText,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &ssMenu = *new StateSlotView{attachParams()};
			pushAndShow(ssMenu, e);
		}
	},
	stateSlotText // Can't init with string literal due to GCC bug #43453
		{'S', 't', 'a', 't', 'e', ' ', 'S', 'l', 'o', 't', ' ', '(', '0', ')' },
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
			modalViewController.pushAndShow(*EmuFilePicker::makeForBenchmarking(attachParams()), e);
		}
	},
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon
	{
		"Add Game Shortcut to Launcher",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(!strlen(EmuSystem::gamePath()))
				{
					// shortcuts to bundled games not yet supported
					return;
				}
				EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e, "Shortcut Name", EmuSystem::fullGameName().data(),
					[this](CollectTextInputView &view, const char *str)
					{
						if(str && strlen(str))
						{
							Base::addLauncherIcon(str, EmuSystem::fullGamePath());
							popup.printf(2, false, "Added shortcut:\n%s", str);
						}
						view.dismiss();
						return 0;
					});
			}
			else
			{
				popup.post("Load a game first");
			}
		}
	},
	#endif
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
				modalViewController.pushAndShow(ynAlertView, e);
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
	},
	screenshot
	{
		"Game Screenshot",
		[](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				emuVideo.takeGameScreenshot();
				EmuSystem::runFrame(emuVideo, false, true, false);
			}
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
	TableView{"Options", attach, subConfig},
	subConfig
	{
		{
			"Video",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::VIDEO_OPTIONS);
				viewStack.pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"Audio",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::AUDIO_OPTIONS);
				viewStack.pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"System",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::SYSTEM_OPTIONS);
				viewStack.pushAndShow(oCategoryMenu, e);
			}
		},
		{
			"GUI",
			[this](TextMenuItem &, View &, Input::Event e)
			{
				auto &oCategoryMenu = *EmuSystem::makeView(attachParams(), EmuSystem::ViewID::GUI_OPTIONS);
				viewStack.pushAndShow(oCategoryMenu, e);
			}
		}
	}
{}

static void loadGameCompleteConfirmYesAutoLoadState(Input::Event e)
{
	loadGameComplete(1, 0);
}

static void loadGameCompleteConfirmNoAutoLoadState(Input::Event e)
{
	loadGameComplete(0, 0);
}

void loadGameCompleteFromRecentItem(Gfx::Renderer &r, uint result, Input::Event e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(r, e, false))
	{
		loadGameComplete(1, 0);
	}
}

void RecentGameInfo::handleMenuSelection(Gfx::Renderer &r, TextMenuItem &, Input::Event e)
{
	EmuApp::createSystemWithMedia({}, path.data(), "", e,
		[&r](uint result, Input::Event e)
		{
			loadGameCompleteFromRecentItem(r, result, e);
		});
}

RecentGameView::RecentGameView(ViewAttachParams attach):
	TableView
	{
		"Recent Games",
		attach,
		[this](const TableView &)
		{
			return 1 + recentGame.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return idx < recentGame.size() ? recentGame[idx] : clear;
		}
	},
	clear
	{
		"Clear List",
		[this](TextMenuItem &t, View &, Input::Event e)
		{
			recentGameList.clear();
			dismiss();
		}
	}
{
	name_ = appViewTitle();
	recentGame.reserve(recentGameList.size());
	for(auto &e : recentGameList)
	{
		recentGame.emplace_back(e.name.data(),
			[&e](TextMenuItem &t, View &view, Input::Event ev)
			{
				e.handleMenuSelection(view.renderer(), t, ev);
			});
		recentGame.back().setActive(FS::exists(e.path.data()));
	}
	clear.setActive(recentGameList.size());
}
