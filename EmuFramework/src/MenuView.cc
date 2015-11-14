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
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/InputManagerView.hh>
#include <emuframework/TouchConfigView.hh>
#include <emuframework/BundledGamesView.hh>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
extern const char *creditsViewStr;
#ifdef CONFIG_BLUETOOTH
extern BluetoothAdapter *bta;
#endif

class ResetAlertView : public AlertView
{
public:
	ResetAlertView(Base::Window &win, const char *label):
		AlertView(win, label, menuItem, 3),
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
	{
		soft.init();
		hard.init();
		cancel.init();
	}

	void deinit() override
	{
		AlertView::deinit();
	}

protected:
	MenuItem *menuItem[3]{&soft, &hard, &cancel};
	TextMenuItem soft, hard, cancel;
};

char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
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

static void handledFailedBTAdapterInit(Input::Event e)
{
	popup.postError("Unable to initialize Bluetooth adapter");
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	if(!FS::exists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
	{
		auto &ynAlertView = *new YesNoAlertView{mainWin.win, "BTstack not found, open Cydia and install?"};
		ynAlertView.onYes() =
			[](Input::Event e)
			{
				logMsg("launching Cydia");
				Base::openURL("cydia://package/ch.ringwald.btstack");
			};
		modalViewController.pushAndShow(ynAlertView, e);
	}
	#endif
}

#endif

void MenuView::onShow()
{
	logMsg("refreshing main menu state");
	recentGames.active = recentGameList.size();
	reset.active = EmuSystem::gameIsRunning();
	saveState.active = EmuSystem::gameIsRunning();
	loadState.active = EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot);
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.compile(projP);
	screenshot.active = EmuSystem::gameIsRunning();
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.active = EmuSystem::gameIsRunning();
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothDisconnect.active = Bluetooth::devsConnected();
	#endif
}

void MenuView::loadFileBrowserItems(MenuItem *item[], uint &items)
{
	loadGame.init(); item[items++] = &loadGame;
	recentGames.init(); item[items++] = &recentGames;
	if(EmuSystem::hasBundledGames && optionShowBundledGames)
	{
		bundledGames.init(); item[items++] = &bundledGames;
	}
}

void MenuView::loadStandardItems(MenuItem *item[], uint &items)
{
	reset.init(); item[items++] = &reset;
	loadState.init(); item[items++] = &loadState;
	saveState.init(); item[items++] = &saveState;
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.init(stateSlotText); item[items++] = &stateSlot;
	if(!Config::MACHINE_IS_OUYA)
	{
		onScreenInputManager.init(); item[items++] = &onScreenInputManager;
	}
	inputManager.init(); item[items++] = &inputManager;
	options.init(); item[items++] = &options;
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes.init(); item[items++] = &scanWiimotes;
		#ifdef CONFIG_BLUETOOTH_SERVER
		acceptPS3ControllerConnection.init(); item[items++] = &acceptPS3ControllerConnection;
		#endif
	bluetoothDisconnect.init(); item[items++] = &bluetoothDisconnect;
	#endif
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.init(); item[items++] = &addLauncherIcon;
	#endif
	benchmark.init(); item[items++] = &benchmark;
	screenshot.init(); item[items++] = &screenshot;
	about.init(); item[items++] = &about;
	exitApp.init(); item[items++] = &exitApp;
}

MenuView::MenuView(Base::Window &win):
	TableView{win},
	loadGame
	{
		"Load Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &fPicker = *new EmuFilePicker{window()};
			fPicker.init(false);
			pushAndShow(fPicker, e, false);
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
					auto &resetAlertView = *new ResetAlertView{window(), "Really Reset?"};
					modalViewController.pushAndShow(resetAlertView, e);
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{window(), "Really Reset?"};
					ynAlertView.onYes() =
						[](Input::Event e)
						{
							EmuSystem::reset(EmuSystem::RESET_SOFT);
							startGameFromMenu();
						};
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
			if(item.active && EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *new YesNoAlertView{window(), "Really Load State?"};
				ynAlertView.onYes() =
					[](Input::Event e)
					{
						int ret = EmuSystem::loadState();
						if(ret != STATE_RESULT_OK)
						{
							if(ret != STATE_RESULT_OTHER_ERROR) // check if we're responsible for posting the error
								popup.postError(stateResultToStr(ret));
						}
						else
							startGameFromMenu();
					};
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
				auto &rMenu = *new RecentGameView{window()};
				rMenu.init();
				pushAndShow(rMenu, e);
			}
		}
	},
	bundledGames
	{
		"Bundled Games",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &bMenu = *new BundledGamesView{window()};
			bMenu.init();
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
						int ret = EmuSystem::saveState();
						if(ret != STATE_RESULT_OK)
							popup.postError(stateResultToStr(ret));
						else
							startGameFromMenu();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{window(), "Really Overwrite State?"};
					ynAlertView.onYes() =
						[](Input::Event e)
						{
							doSaveState();
						};
					modalViewController.pushAndShow(ynAlertView, e);
				}
			}
		}
	},
	stateSlot
	{
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &ssMenu = *new StateSlotView{window()};
			ssMenu.init();
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
			auto &oMenu = *new OptionCategoryView{window()};
			oMenu.init();
			pushAndShow(oMenu, e);
		}
	},
	onScreenInputManager
	{
		"On-screen Input Setup",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &tcMenu = *new TouchConfigView{window(), EmuSystem::inputFaceBtnName, EmuSystem::inputCenterBtnName};
			tcMenu.init();
			pushAndShow(tcMenu, e);
		}
	},
	inputManager
	{
		"Key/Gamepad Input Setup",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &menu = *new InputManagerView{window()};
			menu.init();
			pushAndShow(menu, e);
		}
	},
	benchmark
	{
		"Benchmark Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &fPicker = *new EmuFilePicker{window()};
			fPicker.initForBenchmark();
			modalViewController.pushAndShow(fPicker, e);
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
				auto &textInputView = *new CollectTextInputView{window()};
				textInputView.init("Shortcut Name", EmuSystem::fullGameName().data(),
						getCollectTextCloseAsset());
				textInputView.onText() =
					[this](CollectTextInputView &view, const char *str)
					{
						if(str && strlen(str))
						{
							Base::addLauncherIcon(str, EmuSystem::fullGamePath());
							popup.printf(2, false, "Added shortcut:\n%s", str);
						}
						view.dismiss();
						return 0;
					};
				modalViewController.pushAndShow(textInputView, e);
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
				handledFailedBTAdapterInit(e);
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
				static char str[64];
				snprintf(str, sizeof(str), "Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected());
				auto &ynAlertView = *new YesNoAlertView{window(), str};
				ynAlertView.onYes() =
					[](Input::Event e)
					{
						Bluetooth::closeBT(bta);
					};
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
				handledFailedBTAdapterInit(e);
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
			auto &credits = *new CreditsView{creditsViewStr, window()};
			credits.init();
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
				emuVideo.takeGameScreenshot();
		}
	}
{}

void MenuView::init()
{
	name_ = appViewTitle();
	uint items = 0;
	loadFileBrowserItems(item, items);
	loadStandardItems(item, items);
	assert(items <= IG::size(item));
	TableView::init(item, items);
}

void OptionCategoryView::init()
{
	//logMsg("running option category init");
	uint i = 0;
	for(auto &e : subConfig)
	{
		// TODO: Emulated system input options moved to on-screen & key input option views,
		// rework how option views are initialized
		auto idx = &e - subConfig;
		if(idx == 2)
			continue;
		e.init(); item[i++] = &e;
		e.onSelect() =
		[this, idx](TextMenuItem &, View &, Input::Event e)
		{
			auto &oCategoryMenu = *makeOptionCategoryMenu(window(), idx);
			viewStack.pushAndShow(oCategoryMenu, e);
		};
	}
	assert(i <= IG::size(item));
	TableView::init(item, i);
}

static void loadGameCompleteConfirmYesAutoLoadState(Input::Event e)
{
	loadGameComplete(1, 0);
}

static void loadGameCompleteConfirmNoAutoLoadState(Input::Event e)
{
	loadGameComplete(0, 0);
}

bool showAutoStateConfirm(Input::Event e, bool addToRecent);

void loadGameCompleteFromRecentItem(uint result, Input::Event e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(e, false))
	{
		loadGameComplete(1, 0);
	}
}

void RecentGameInfo::handleMenuSelection(TextMenuItem &, Input::Event e)
{
	FS::current_path(FS::dirname(path));
	EmuSystem::onLoadGameComplete() =
		[](uint result, Input::Event e)
		{
			loadGameCompleteFromRecentItem(result, e);
		};
	auto res = EmuSystem::loadGameFromPath(path);
	if(res == 1)
	{
		loadGameCompleteFromRecentItem(1, e);
	}
	else if(res == 0)
	{
		EmuSystem::clearGamePaths();
	}
}

void RecentGameView::init()
{
	name_ = appViewTitle();
	uint i = 0;
	int rIdx = 0;
	for(auto &e : recentGameList)
	{
		recentGame[rIdx].init(e.name.data(), FS::exists(e.path.data())); item[i++] = &recentGame[rIdx];
		recentGame[rIdx].onSelect() = [&e](TextMenuItem &t, View &, Input::Event ev) {e.handleMenuSelection(t,ev);};
		rIdx++;
	}
	clear.init(recentGameList.size()); item[i++] = &clear;
	assert(i <= IG::size(item));
	TableView::init(item, i);
}

RecentGameView::RecentGameView(Base::Window &win):
	TableView{"Recent Games", win},
	clear
	{
		"Clear List",
		[this](TextMenuItem &t, View &, Input::Event e)
		{
			recentGameList.clear();
			dismiss();
		}
	}
{}
