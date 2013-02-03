/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <libgen.h>
#include <MenuView.hh>
#include <Recent.hh>
#include <gui/AlertView.hh>
#include <MsgPopup.hh>
#include <EmuSystem.hh>
#include <util/gui/ViewStack.hh>
#include <CreditsView.hh>
#include <FilePicker.hh>
#include <StateSlotView.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <InputManagerView.hh>
#include <util/strings.h>
#ifdef CONFIG_BLUETOOTH
	#include <bluetooth/sys.hh>
	#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
extern YesNoAlertView ynAlertView;
extern MsgPopup popup;
extern ViewStack viewStack;
extern CreditsView credits;
extern EmuFilePicker fPicker;
extern OptionCategoryView oMenu;
extern RecentGameView rMenu;
extern InputManagerView imMenu;
extern StateSlotView ssMenu;
void takeGameScreenshot();

void loadGameHandler(TextMenuItem &, const Input::Event &e)
{
	fPicker.init(!e.isPointer());
	viewStack.useNavView = 0;
	viewStack.pushAndShow(&fPicker);
}

void confirmResetAlert(const Input::Event &e)
{
	EmuSystem::resetGame();
	startGameFromMenu();
}

void resetHandler(TextMenuItem &, const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Reset Game?", !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmResetAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

void confirmLoadStateAlert(const Input::Event &e)
{
	int ret = EmuSystem::loadState();
	if(ret != STATE_RESULT_OK)
	{
		if(ret != STATE_RESULT_OTHER_ERROR) // check if we're responsible for posting the error
			popup.postError(stateResultToStr(ret));
	}
	else
		startGameFromMenu();
}

void loadStateHandler(TextMenuItem &item, const Input::Event &e)
{
	if(item.active && EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Load State?", !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmLoadStateAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

void recentGamesHandler(TextMenuItem &, const Input::Event &e)
{
	if(recentGameList.size)
	{
		rMenu.init(!e.isPointer());
		viewStack.pushAndShow(&rMenu);
	}
}

void doSaveState()
{
	int ret = EmuSystem::saveState();
	if(ret != STATE_RESULT_OK)
		popup.postError(stateResultToStr(ret));
	else
		startGameFromMenu();
}

void confirmSaveStateAlert(const Input::Event &e)
{
	doSaveState();
}

void saveStateHandler(TextMenuItem &, const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
	{
		if(!optionConfirmOverwriteState || !EmuSystem::stateExists(EmuSystem::saveStateSlot))
		{
			doSaveState();
		}
		else
		{
			ynAlertView.init("Really Overwrite State?", !e.isPointer());
			ynAlertView.onYesDelegate().bind<&confirmSaveStateAlert>();
			ynAlertView.placeRect(Gfx::viewportRect());
			View::modalView = &ynAlertView;
		}
	}
}


char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void stateSlotHandler(TextMenuItem &, const Input::Event &e)
{
	ssMenu.init(!e.isPointer());
	viewStack.pushAndShow(&ssMenu);
}

void optionsHandler(TextMenuItem &, const Input::Event &e)
{
	oMenu.init(!e.isPointer());
	viewStack.pushAndShow(&oMenu);
}

void inputManagerHandler(TextMenuItem &, const Input::Event &e)
{
	imMenu.init(!e.isPointer());
	viewStack.pushAndShow(&imMenu);
}

void benchmarkHandler(TextMenuItem &, const Input::Event &e)
{
	fPicker.initForBenchmark(!e.isPointer());
	fPicker.placeRect(Gfx::viewportRect());
	View::modalView = &fPicker;
	Base::displayNeedsUpdate();
}

#ifdef CONFIG_BLUETOOTH

void btStatus(uint status, int arg)
{
	switch(status)
	{
		bcase BluetoothAdapter::INIT_FAILED:
		{
			if(Config::envIsIOS)
			{
				popup.postError("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
				Base::displayNeedsUpdate();
			}
		}
		bcase BluetoothAdapter::SCAN_FAILED:
		{
			popup.postError("Scan failed");
			Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SCAN_NO_DEVS:
		{
			popup.post("No devices found");
			Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SCAN_PROCESSING:
		{
			popup.printf(2, 0, "Checking %d device(s)...", arg);
			Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SCAN_NAME_FAILED:
		{
			popup.postError("Failed reading a device name");
			Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SCAN_COMPLETE:
		{
			int devs = Bluetooth::pendingDevs();
			if(devs)
			{
				popup.printf(2, 0, "Connecting to %d device(s)...", devs);
				Bluetooth::connectPendingDevs();
			}
			else
			{
				popup.post("Scan complete, no recognized devices");
			}
			Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SOCKET_OPEN_FAILED:
		{
			popup.postError("Failed opening a Bluetooth connection");
			Base::displayNeedsUpdate();
		}
	}
}

#ifdef CONFIG_BTSTACK

void confirmBluetoothScanAlert(const Input::Event &e)
{
	logMsg("launching Cydia");
	Base::openURL("cydia://package/ch.ringwald.btstack");
}

#endif

#ifdef CONFIG_BASE_IOS_SETUID
namespace CATS
{
	extern char warWasBeginning[];
}
#endif

void bluetoothScanHandler(TextMenuItem &, const Input::Event &e)
{
	#ifdef CONFIG_BASE_IOS_SETUID
		if(FsSys::fileExists(CATS::warWasBeginning))
			return;
	#endif

	if(Bluetooth::initBT() == OK)
	{
		BluetoothAdapter::defaultAdapter()->statusDelegate().bind<&btStatus>();
		if(Bluetooth::startBT())
		{
			popup.post("Starting Scan...\nSee website for device-specific help", 4);
		}
		else
		{
			popup.post("Still scanning", 1);
		}
	}
	else
	{
		#ifdef CONFIG_BTSTACK
			popup.postError("Failed connecting to BT daemon");
			if(!FsSys::fileExists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
			{
				ynAlertView.init("BTstack not found, open Cydia and install?", !e.isPointer());
				ynAlertView.onYesDelegate().bind<&confirmBluetoothScanAlert>();
				ynAlertView.placeRect(Gfx::viewportRect());
				View::modalView = &ynAlertView;
			}
		#elif defined(CONFIG_BASE_ANDROID)
			popup.postError("Bluetooth not accessible, verify it's on and your Android version is compatible");
		#else
			popup.postError("Bluetooth not accessible");
		#endif
	}
	Base::displayNeedsUpdate();
}

void confirmBluetoothDisconnectAlert(const Input::Event &e)
{
	Bluetooth::closeBT();
}

void bluetoothDisconnectHandler(TextMenuItem &item, const Input::Event &e)
{
	if(Bluetooth::devsConnected())
	{
		static char str[64];
		snprintf(str, sizeof(str), "Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected());
		ynAlertView.init(str, !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmBluetoothDisconnectAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

#endif

void aboutHandler(TextMenuItem &, const Input::Event &e)
{
	credits.init();
	viewStack.pushAndShow(&credits);
}

void exitAppHandler(TextMenuItem &, const Input::Event &e)
{
	Base::exit();
}

void screenshotHandler(TextMenuItem &item, const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
		takeGameScreenshot();
}

void MenuView::onShow()
{
	logMsg("refreshing main menu state");
	recentGames.active = recentGameList.size;
	reset.active = EmuSystem::gameIsRunning();
	saveState.active = EmuSystem::gameIsRunning();
	loadState.active = EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot);
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.compile();
	screenshot.active = EmuSystem::gameIsRunning();
	#ifdef CONFIG_BLUETOOTH
		bluetoothDisconnect.active = Bluetooth::devsConnected();
	#endif
}

void MenuView::loadFileBrowserItems(MenuItem *item[], uint &items)
{
	loadGame.init(); item[items++] = &loadGame;
	loadGame.selectDelegate().bind<&loadGameHandler>();
	recentGames.init(); item[items++] = &recentGames;
	recentGames.selectDelegate().bind<&recentGamesHandler>();
}

void MenuView::loadStandardItems(MenuItem *item[], uint &items)
{
	reset.init(); item[items++] = &reset;
	reset.selectDelegate().bind<&resetHandler>();
	loadState.init(); item[items++] = &loadState;
	loadState.selectDelegate().bind<&loadStateHandler>();
	saveState.init(); item[items++] = &saveState;
	saveState.selectDelegate().bind<&saveStateHandler>();

	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.init(stateSlotText); item[items++] = &stateSlot;
	stateSlot.selectDelegate().bind<&stateSlotHandler>();

	options.init(); item[items++] = &options;
	options.selectDelegate().bind<&optionsHandler>();
	inputManager.init(); item[items++] = &inputManager;
	inputManager.selectDelegate().bind<&inputManagerHandler>();
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes.init(); item[items++] = &scanWiimotes;
	scanWiimotes.selectDelegate().bind<&bluetoothScanHandler>();
	bluetoothDisconnect.init(); item[items++] = &bluetoothDisconnect;
	bluetoothDisconnect.selectDelegate().bind<&bluetoothDisconnectHandler>();
	#endif
	benchmark.init(); item[items++] = &benchmark;
	benchmark.selectDelegate().bind<&benchmarkHandler>();
	screenshot.init(); item[items++] = &screenshot;
	screenshot.selectDelegate().bind<&screenshotHandler>();
	about.init(); item[items++] = &about;
	about.selectDelegate().bind<&aboutHandler>();
	exitApp.init(); item[items++] = &exitApp;
	exitApp.selectDelegate().bind<&exitAppHandler>();
}

extern void loadGameComplete(bool tryAutoState, bool addToRecent);

static void loadGameCompleteConfirmYesAutoLoadState(const Input::Event &e)
{
	loadGameComplete(1, 0);
}

static void loadGameCompleteConfirmNoAutoLoadState(const Input::Event &e)
{
	loadGameComplete(0, 0);
}

bool showAutoStateConfirm(const Input::Event &e);

void loadGameCompleteFromRecentItem(uint result, const Input::Event &e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(e))
	{
		loadGameComplete(1, 0);
	}
}

void RecentGameInfo::handleMenuSelection(TextMenuItem &, const Input::Event &e)
{
	FsSys::cPath dirNameTemp;
	strcpy(dirNameTemp, path);
	FsSys::chdir(dirname(dirNameTemp));
	EmuSystem::loadGameCompleteDelegate().bind<&loadGameCompleteFromRecentItem>();
	auto res = EmuSystem::loadGame(path);
	if(res == 1)
	{
		loadGameCompleteFromRecentItem(1, e);
	}
	else if(res == 0)
	{
		EmuSystem::clearGamePaths();
	}
}

void RecentGameView::clearRecentMenuHandler(TextMenuItem &, const Input::Event &e)
{
	recentGameList.removeAll();
	viewStack.popAndShow();
}

void RecentGameView::init(bool highlightFirst)
{
	uint i = 0;
	int rIdx = 0;
	forEachInDLList(&recentGameList, e)
	{
		recentGame[rIdx].init(e.name, FsSys::fileExists(e.path)); item[i++] = &recentGame[rIdx];
		recentGame[rIdx].selectDelegate().bind<RecentGameInfo, &RecentGameInfo::handleMenuSelection>(&e);
		rIdx++;
	}
	clear.init(recentGameList.size); item[i++] = &clear;
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

void OptionCategoryView::init(bool highlightFirst)
{
	//logMsg("running option category init");
	uint i = 0;
	forEachInArray(subConfig, e)
	{
		e->init(); item[i++] = e;
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}
