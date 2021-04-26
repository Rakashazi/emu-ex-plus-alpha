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

#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/OptionView.hh>
#include "EmuOptions.hh"
#include <emuframework/InputManagerView.hh>
#include <emuframework/TouchConfigView.hh>
#include <emuframework/BundledGamesView.hh>
#include "private.hh"
#include "RecentGameView.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/base/ApplicationContext.hh>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
#ifdef CONFIG_BLUETOOTH
extern BluetoothAdapter *bta;
#endif

#ifdef CONFIG_BLUETOOTH

static bool initBTAdapter(Base::ApplicationContext ctx)
{
	if(bta)
	{
		return true;
	}

	logMsg("initializing Bluetooth");
	bta = BluetoothAdapter::defaultAdapter(ctx);
	return bta;
}

static void onScanStatus(EmuApp &app, unsigned status, int arg)
	{
		switch(status)
		{
			bcase BluetoothAdapter::INIT_FAILED:
			{
				if(Config::envIsIOS)
				{
					app.postErrorMessage("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
				}
			}
			bcase BluetoothAdapter::SCAN_FAILED:
			{
				app.postErrorMessage("Scan failed");
			}
			bcase BluetoothAdapter::SCAN_NO_DEVS:
			{
				app.postMessage("No devices found");
			}
			bcase BluetoothAdapter::SCAN_PROCESSING:
			{
				app.printfMessage(2, 0, "Checking %d device(s)...", arg);
			}
			bcase BluetoothAdapter::SCAN_NAME_FAILED:
			{
				app.postErrorMessage("Failed reading a device name");
			}
			bcase BluetoothAdapter::SCAN_COMPLETE:
			{
				int devs = Bluetooth::pendingDevs();
				if(devs)
				{
					app.printfMessage(2, 0, "Connecting to %d device(s)...", devs);
					Bluetooth::connectPendingDevs(bta);
				}
				else
				{
					app.postMessage("Scan complete, no recognized devices");
				}
			}
			/*bcase BluetoothAdapter::SOCKET_OPEN_FAILED:
			{
				app.postErrorMessage("Failed opening a Bluetooth connection");
			}*/
		}
	};

template <class ViewT>
static void handledFailedBTAdapterInit(ViewT &view, ViewAttachParams attach, Input::Event e)
{
	view.app().postErrorMessage("Unable to initialize Bluetooth adapter");
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	if(!FS::exists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
	{
		auto ynAlertView = std::make_unique<YesNoAlertView>(attach, "BTstack not found, open Cydia and install?");
		ynAlertView->setOnYes(
			[](View &v)
			{
				logMsg("launching Cydia");
				v.appContext().openURL("cydia://package/ch.ringwald.btstack");
			});
		view.pushAndShowModal(std::move(ynAlertView), e, false);
	}
	#endif
}

#endif

void EmuMainMenuView::onShow()
{
	TableView::onShow();
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
	item.emplace_back(&onScreenInputManager);
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

void EmuMainMenuView::setAudioVideo(EmuAudio &audio_, EmuVideoLayer &videoLayer_)
{
	audio = &audio_;
	videoLayer = &videoLayer_;
}

EmuMainMenuView::EmuMainMenuView(ViewAttachParams attach, bool customMenu):
	TableView{appViewTitle(), attach, item},
	loadGame
	{
		"Load Game", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(EmuFilePicker::makeForLoading(attachParams(), e), e, false);
		}
	},
	systemActions
	{
		"System Actions", &defaultFace(),
		[this](Input::Event e)
		{
			if(!EmuSystem::gameIsRunning())
				return;
			pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_ACTIONS), e);
		}
	},
	recentGames
	{
		"Recent Games", &defaultFace(),
		[this](Input::Event e)
		{
			if(recentGameList.size())
			{
				pushAndShow(makeView<RecentGameView>(recentGameList), e);
			}
		}
	},
	bundledGames
	{
		"Bundled Games", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<BundledGamesView>(), e);
		}
	},
	options
	{
		"Options", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<OptionCategoryView>(*audio, *videoLayer), e);
		}
	},
	onScreenInputManager
	{
		"On-screen Input Setup", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<TouchConfigView>(app().defaultVController(), EmuSystem::inputFaceBtnName, EmuSystem::inputCenterBtnName), e);
		}
	},
	inputManager
	{
		"Key/Gamepad Input Setup", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<InputManagerView>(), e);
		}
	},
	benchmark
	{
		"Benchmark Game", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(EmuFilePicker::makeForBenchmarking(attachParams(), e), e, false);
		}
	},
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes
	{
		"Scan for Wiimotes/iCP/JS1", &defaultFace(),
		[this](Input::Event e)
		{
			if(initBTAdapter(appContext()))
			{
				if(Bluetooth::scanForDevices(appContext(), *bta,
					[this](BluetoothAdapter &, unsigned status, int arg)
					{
						onScanStatus(app(), status, arg);
					}))
				{
					app().postMessage(4, "Starting Scan...\n(see website for device-specific help)");
				}
				else
				{
					app().postMessage(1, "Still scanning");
				}
			}
			else
			{
				handledFailedBTAdapterInit(*this, attachParams(), e);
			}
			postDraw();
		}
	},
	bluetoothDisconnect
	{
		"Disconnect Bluetooth", &defaultFace(),
		[this](Input::Event e)
		{
			if(Bluetooth::devsConnected())
			{
				auto ynAlertView = makeView<YesNoAlertView>(
					string_makePrintf<64>("Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected()).data());
				ynAlertView->setOnYes(
					[]()
					{
						Bluetooth::closeBT(bta);
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SERVER
	acceptPS3ControllerConnection
	{
		"Scan for PS3 Controller", &defaultFace(),
		[this](Input::Event e)
		{
			if(initBTAdapter(appContext()))
			{
				app().postMessage(4, "Prepare to push the PS button");
				auto startedScan = Bluetooth::listenForDevices(appContext(), *bta,
					[this](BluetoothAdapter &bta, unsigned status, int arg)
					{
						switch(status)
						{
							bcase BluetoothAdapter::INIT_FAILED:
							{
								app().postErrorMessage(Config::envIsLinux ? 8 : 2,
									Config::envIsLinux ?
										"Unable to register server, make sure this executable has cap_net_bind_service enabled and bluetoothd isn't running" :
										"Bluetooth setup failed");
							}
							bcase BluetoothAdapter::SCAN_COMPLETE:
							{
								app().postMessage(4, "Push the PS button on your controller\n(see website for pairing help)");
							}
							bdefault: onScanStatus(app(), status, arg);
						}
					});
				if(!startedScan)
				{
					app().postMessage(1, "Still scanning");
				}
			}
			else
			{
				handledFailedBTAdapterInit(*this, attachParams(), e);
			}
			postDraw();
		}
	},
	#endif
	about
	{
		"About", &defaultFace(),
		[this](Input::Event e)
		{
			pushAndShow(makeView<CreditsView>(EmuSystem::creditsViewStr), e);
		}
	},
	exitApp
	{
		"Exit", &defaultFace(),
		[this](Input::Event e)
		{
			appContext().exit();
		}
	}
{
	if(!customMenu)
	{
		loadFileBrowserItems();
		loadStandardItems();
		app().setOnMainMenuItemOptionChanged(
			[this]()
			{
				waitForDrawFinished();
				item.clear();
				loadFileBrowserItems();
				loadStandardItems();
			});
	}
}

OptionCategoryView::OptionCategoryView(ViewAttachParams attach, EmuAudio &audio, EmuVideoLayer &videoLayer):
	TableView
	{
		"Options",
		attach,
		[this](const TableView &) { return hasGooglePlayStoreFeatures() ? std::size(subConfig) : std::size(subConfig)-1; },
		[this](const TableView &, unsigned idx) -> MenuItem& { return subConfig[idx]; }
	},
	subConfig
	{
		{
			"Video", &defaultFace(),
			[this, &videoLayer](Input::Event e)
			{
				auto view = EmuApp::makeView(attachParams(), EmuApp::ViewID::VIDEO_OPTIONS);
				static_cast<VideoOptionView*>(view.get())->setEmuVideoLayer(videoLayer);
				pushAndShow(std::move(view), e);
			}
		},
		{
			"Audio", &defaultFace(),
			[this, &audio](Input::Event e)
			{
				auto view = EmuApp::makeView(attachParams(), EmuApp::ViewID::AUDIO_OPTIONS);
				static_cast<AudioOptionView*>(view.get())->setEmuAudio(audio);
				pushAndShow(std::move(view), e);
			}
		},
		{
			"System", &defaultFace(),
			[this](Input::Event e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_OPTIONS), e);
			}
		},
		{
			"GUI", &defaultFace(),
			[this](Input::Event e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::GUI_OPTIONS), e);
			}
		}
	}
{
	if(hasGooglePlayStoreFeatures())
	{
		subConfig[std::size(subConfig)-1] =
		{
			"Beta Testing Opt-in/out", &defaultFace(),
			[this]()
			{
				appContext().openURL(string_makePrintf<96>("https://play.google.com/apps/testing/%s", appContext().applicationId).data());
			}
		};
	}
}

std::unique_ptr<View> EmuApp::makeView(ViewAttachParams attach, ViewID id)
{
	auto view = makeCustomView(attach, id);
	if(view)
		return view;
	switch(id)
	{
		case ViewID::MAIN_MENU: return std::make_unique<EmuMainMenuView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<EmuSystemActionsView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<VideoOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<AudioOptionView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<SystemOptionView>(attach);
		case ViewID::GUI_OPTIONS: return std::make_unique<GUIOptionView>(attach);
		default:
			bug_unreachable("Tried to make non-existing view ID:%d", (int)id);
			return nullptr;
	}
}
