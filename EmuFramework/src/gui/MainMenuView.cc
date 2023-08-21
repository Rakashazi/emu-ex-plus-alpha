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

#include <emuframework/MainMenuView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/InputManagerView.hh>
#include <emuframework/TouchConfigView.hh>
#include <emuframework/BundledGamesView.hh>
#include "RecentContentView.hh"
#include "../EmuOptions.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/FS.hh>
#include <imagine/bluetooth/sys.hh>
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#include <format>

namespace EmuEx
{

class OptionCategoryView : public TableView, public EmuAppHelper<OptionCategoryView>
{
public:
	OptionCategoryView(ViewAttachParams attach, EmuAudio &audio, EmuVideoLayer &videoLayer);

protected:
	TextMenuItem subConfig[7];
};

static void onScanStatus(EmuApp &app, unsigned status, int arg);

template <class ViewT>
static void handledFailedBTAdapterInit(ViewT &view, ViewAttachParams attach, const Input::Event &e)
{
	view.app().postErrorMessage("Unable to initialize Bluetooth adapter");
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	if(!FS::exists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
	{
		view.pushAndShowModal(std::make_unique<YesNoAlertView>(attach, "BTstack not found, open Cydia and install?",
			YesNoAlertView::Delegates
			{
				.onYes = [](View &v){ v.appContext().openURL("cydia://package/ch.ringwald.btstack"); }
			}), e, false);
	}
	#endif
}

MainMenuView::MainMenuView(ViewAttachParams attach, bool customMenu):
	TableView{EmuApp::mainViewName(), attach, item},
	loadGame
	{
		"Open Content", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(FilePicker::forLoading(attachParams(), e), e, false);
		}
	},
	systemActions
	{
		"System Actions", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_ACTIONS), e);
		}
	},
	recentGames
	{
		"Recent Content", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(app().recentContent.size())
			{
				pushAndShow(makeView<RecentContentView>(app().recentContent), e);
			}
		}
	},
	bundledGames
	{
		"Bundled Content", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<BundledGamesView>(), e);
		}
	},
	options
	{
		"Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<OptionCategoryView>(*audio, *videoLayer), e);
		}
	},
	onScreenInputManager
	{
		"On-screen Input Setup", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<TouchConfigView>(app().defaultVController()), e);
		}
	},
	inputManager
	{
		"Key/Gamepad Input Setup", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<InputManagerView>(app().inputManager), e);
		}
	},
	benchmark
	{
		"Benchmark Content", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(FilePicker::forBenchmarking(attachParams(), e), e, false);
		}
	},
	scanWiimotes
	{
		"Scan for Wiimotes/iCP/JS1", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(app().bluetoothAdapter())
			{
				if(Bluetooth::scanForDevices(appContext(), *app().bluetoothAdapter(),
					[this](BluetoothAdapter &, unsigned status, int arg)
					{
						onScanStatus(app(), status, arg);
					}))
				{
					app().postMessage(4, false, "Starting Scan...\n(see website for device-specific help)");
				}
				else
				{
					app().postMessage(1, false, "Still scanning");
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
		[this](const Input::Event &e)
		{
			auto devConnected = Bluetooth::devsConnected(appContext());
			if(devConnected)
			{
				pushAndShowModal(makeView<YesNoAlertView>(std::format("Really disconnect {} Bluetooth device(s)?", devConnected),
					YesNoAlertView::Delegates{.onYes = [this]{ app().closeBluetoothConnections(); }}), e);
			}
		}
	},
	#ifdef CONFIG_BLUETOOTH_SERVER
	acceptPS3ControllerConnection
	{
		"Scan for PS3 Controller", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(app().bluetoothAdapter())
			{
				app().postMessage(4, "Prepare to push the PS button");
				auto startedScan = Bluetooth::listenForDevices(appContext(), *app().bluetoothAdapter(),
					[this](BluetoothAdapter &bta, unsigned status, int arg)
					{
						switch(status)
						{
							case BluetoothAdapter::INIT_FAILED:
							{
								app().postErrorMessage(Config::envIsLinux ? 8 : 2,
									Config::envIsLinux ?
										"Unable to register server, make sure this executable has cap_net_bind_service enabled and bluetoothd isn't running" :
										"Bluetooth setup failed");
								break;
							}
							case BluetoothAdapter::SCAN_COMPLETE:
							{
								app().postMessage(4, "Push the PS button on your controller\n(see website for pairing help)");
								break;
							}
							default: onScanStatus(app(), status, arg);
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
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<CreditsView>(EmuSystem::creditsViewStr), e);
		}
	},
	exitApp
	{
		"Exit", &defaultFace(),
		[this]()
		{
			appContext().exit();
		}
	}
{
	if(!customMenu)
	{
		reloadItems();
	}
}

static void onScanStatus(EmuApp &app, unsigned status, int arg)
{
	switch(status)
	{
		case BluetoothAdapter::INIT_FAILED:
		{
			if(Config::envIsIOS)
			{
				app.postErrorMessage("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
			}
			break;
		}
		case BluetoothAdapter::SCAN_FAILED:
		{
			app.postErrorMessage("Scan failed");
			break;
		}
		case BluetoothAdapter::SCAN_NO_DEVS:
		{
			app.postMessage("No devices found");
			break;
		}
		case BluetoothAdapter::SCAN_PROCESSING:
		{
			app.postMessage(2, 0, std::format("Checking {} device(s)...", arg));
			break;
		}
		case BluetoothAdapter::SCAN_NAME_FAILED:
		{
			app.postErrorMessage("Failed reading a device name");
			break;
		}
		case BluetoothAdapter::SCAN_COMPLETE:
		{
			int devs = Bluetooth::pendingDevs();
			if(devs)
			{
				app.postMessage(2, 0, std::format("Connecting to {} device(s)...", devs));
				Bluetooth::connectPendingDevs(app.bluetoothAdapter());
			}
			else
			{
				app.postMessage("Scan complete, no recognized devices");
			}
			break;
		}
		/*case BluetoothAdapter::SOCKET_OPEN_FAILED:
		{
			app.postErrorMessage("Failed opening a Bluetooth connection");
		}*/
	}
};

void MainMenuView::onShow()
{
	TableView::onShow();
	logMsg("refreshing main menu state");
	recentGames.setActive(app().recentContent.size());
	systemActions.setActive(system().hasContent());
	bluetoothDisconnect.setActive(Bluetooth::devsConnected(appContext()));
}

void MainMenuView::loadFileBrowserItems()
{
	item.emplace_back(&loadGame);
	item.emplace_back(&recentGames);
	if(EmuSystem::hasBundledGames && app().showsBundledGames())
	{
		item.emplace_back(&bundledGames);
	}
}

void MainMenuView::loadStandardItems()
{
	item.emplace_back(&systemActions);
	item.emplace_back(&onScreenInputManager);
	item.emplace_back(&inputManager);
	item.emplace_back(&options);
	if(used(scanWiimotes) && app().showsBluetoothScanItems())
	{
		item.emplace_back(&scanWiimotes);
		#ifdef CONFIG_BLUETOOTH_SERVER
		item.emplace_back(&acceptPS3ControllerConnection);
		#endif
		item.emplace_back(&bluetoothDisconnect);
	}
	item.emplace_back(&benchmark);
	item.emplace_back(&about);
	item.emplace_back(&exitApp);
}

void MainMenuView::setAudioVideo(EmuAudio &audio_, EmuVideoLayer &videoLayer_)
{
	audio = &audio_;
	videoLayer = &videoLayer_;
}

void MainMenuView::reloadItems()
{
	item.clear();
	loadFileBrowserItems();
	loadStandardItems();
}

OptionCategoryView::OptionCategoryView(ViewAttachParams attach, EmuAudio &audio, EmuVideoLayer &videoLayer):
	TableView
	{
		"Options",
		attach,
		[this](const TableView &) { return EmuApp::hasGooglePlayStoreFeatures() ? std::size(subConfig) : std::size(subConfig)-1; },
		[this](const TableView &, size_t idx) -> MenuItem& { return subConfig[idx]; }
	},
	subConfig
	{
		{
			"Video", &defaultFace(),
			[this, &videoLayer](const Input::Event &e)
			{
				auto view = EmuApp::makeView(attachParams(), EmuApp::ViewID::VIDEO_OPTIONS);
				static_cast<VideoOptionView*>(view.get())->setEmuVideoLayer(videoLayer);
				pushAndShow(std::move(view), e);
			}
		},
		{
			"Audio", &defaultFace(),
			[this, &audio](const Input::Event &e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::AUDIO_OPTIONS), e);
			}
		},
		{
			"System", &defaultFace(),
			[this](const Input::Event &e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_OPTIONS), e);
			}
		},
		{
			"File Paths", &defaultFace(),
			[this](const Input::Event &e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::FILE_PATH_OPTIONS), e);
			}
		},
		{
			"GUI", &defaultFace(),
			[this](const Input::Event &e)
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::GUI_OPTIONS), e);
			}
		},
		{
			"Online Documentation", &defaultFace(),
			[this]
			{
				appContext().openURL("https://www.explusalpha.com/contents/emuex/documentation");
			}
		}
	}
{
	if(EmuApp::hasGooglePlayStoreFeatures())
	{
		subConfig[std::size(subConfig)-1] =
		{
			"Beta Testing Opt-in/out", &defaultFace(),
			[this]()
			{
				appContext().openURL(std::format("https://play.google.com/apps/testing/{}", appContext().applicationId));
			}
		};
	}
}

}
