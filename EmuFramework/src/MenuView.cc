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

// TODO remove once header is refactored
#define PROTOTYPES_ONLY
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
extern InputPlayerMapView ipmMenu;
extern StateSlotView ssMenu;
void takeGameScreenshot();

void loadGameHandler(TextMenuItem &, const InputEvent &e)
{
	fPicker.init(!e.isPointer());
	viewStack.useNavView = 0;
	viewStack.pushAndShow(&fPicker);
}

void confirmResetAlert(const InputEvent &e)
{
	EmuSystem::resetGame();
	startGameFromMenu();
}

void resetHandler(TextMenuItem &, const InputEvent &e)
{
	if(EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Reset Game?", !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmResetAlert>();
		ynAlertView.place(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

void confirmLoadStateAlert(const InputEvent &e)
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

void loadStateHandler(TextMenuItem &item, const InputEvent &e)
{
	if(item.active && EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Load State?", !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmLoadStateAlert>();
		ynAlertView.place(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

void recentGamesHandler(TextMenuItem &, const InputEvent &e)
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

void confirmSaveStateAlert(const InputEvent &e)
{
	doSaveState();
}

void saveStateHandler(TextMenuItem &, const InputEvent &e)
{
	if(EmuSystem::gameIsRunning())
	{
		if(!EmuSystem::stateExists(EmuSystem::saveStateSlot))
		{
			doSaveState();
		}
		else
		{
			ynAlertView.init("Really Overwrite State?", !e.isPointer());
			ynAlertView.onYesDelegate().bind<&confirmSaveStateAlert>();
			ynAlertView.place(Gfx::viewportRect());
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

void stateSlotHandler(TextMenuItem &, const InputEvent &e)
{
	ssMenu.init(!e.isPointer());
	viewStack.pushAndShow(&ssMenu);
}

void optionsHandler(TextMenuItem &, const InputEvent &e)
{
	oMenu.init(!e.isPointer());
	viewStack.pushAndShow(&oMenu);
}

void inputPlayerMapHandler(TextMenuItem &, const InputEvent &e)
{
	ipmMenu.init(!e.isPointer());
	viewStack.pushAndShow(&ipmMenu);
}

void benchmarkHandler(TextMenuItem &, const InputEvent &e)
{
	//static BenchmarkFilePicker picker;
	fPicker.initForBenchmark(!e.isPointer());
	fPicker.place(Gfx::viewportRect());
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
			//popup.post("Connecting to devices...");
			//Base::displayNeedsUpdate();
		}
		bcase BluetoothAdapter::SCAN_COMPLETE_NO_DEVS_USED:
		{
			popup.post("Scan complete, no recognized devices");
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

void confirmBluetoothScanAlert(const InputEvent &e)
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

void bluetoothScanHandler(TextMenuItem &, const InputEvent &e)
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
				ynAlertView.place(Gfx::viewportRect());
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

void confirmBluetoothDisconnectAlert(const InputEvent &e)
{
	Bluetooth::closeBT();
}

void bluetoothDisconnectHandler(TextMenuItem &item, const InputEvent &e)
{
	if(Bluetooth::devsConnected())
	{
		static char str[64];
		snprintf(str, sizeof(str), "Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected());
		ynAlertView.init(str, !e.isPointer());
		ynAlertView.onYesDelegate().bind<&confirmBluetoothDisconnectAlert>();
		ynAlertView.place(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

#endif

void aboutHandler(TextMenuItem &, const InputEvent &e)
{
	credits.init();
	viewStack.pushAndShow(&credits);
}

void exitAppHandler(TextMenuItem &, const InputEvent &e)
{
	Base::exit();
}

void screenshotHandler(TextMenuItem &item, const InputEvent &e)
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
	loadGame.init("Load Game"); item[items++] = &loadGame;
	loadGame.selectDelegate().bind<&loadGameHandler>();
	recentGames.init("Recent Games"); item[items++] = &recentGames;
	recentGames.selectDelegate().bind<&recentGamesHandler>();
}

void MenuView::loadStandardItems(MenuItem *item[], uint &items)
{
	reset.init("Reset"); item[items++] = &reset;
	reset.selectDelegate().bind<&resetHandler>();
	loadState.init("Load State"); item[items++] = &loadState;
	loadState.selectDelegate().bind<&loadStateHandler>();
	saveState.init("Save State"); item[items++] = &saveState;
	saveState.selectDelegate().bind<&saveStateHandler>();

	strcpy(stateSlotText, "State Slot (0)"); // TODO do in constructor
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.init(stateSlotText); item[items++] = &stateSlot;
	stateSlot.selectDelegate().bind<&stateSlotHandler>();

	options.init("Options"); item[items++] = &options;
	options.selectDelegate().bind<&optionsHandler>();
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes.init("Scan for Wiimotes/iCP/JS1"); item[items++] = &scanWiimotes;
	scanWiimotes.selectDelegate().bind<&bluetoothScanHandler>();
	bluetoothDisconnect.init("Disconnect Bluetooth"); item[items++] = &bluetoothDisconnect;
	bluetoothDisconnect.selectDelegate().bind<&bluetoothDisconnectHandler>();
	#endif
	if(EmuSystem::maxPlayers > 1)
	{
		inputPlayerMap.init("Input/Player Mapping"); item[items++] = &inputPlayerMap;
		inputPlayerMap.selectDelegate().bind<&inputPlayerMapHandler>();
	}
	benchmark.init("Benchmark Game"); item[items++] = &benchmark;
	benchmark.selectDelegate().bind<&benchmarkHandler>();
	screenshot.init("Game Screenshot"); item[items++] = &screenshot;
	screenshot.selectDelegate().bind<&screenshotHandler>();
	about.init("About"); item[items++] = &about;
	about.selectDelegate().bind<&aboutHandler>();
	exitApp.init("Exit"); item[items++] = &exitApp;
	exitApp.selectDelegate().bind<&exitAppHandler>();
}

void InputPlayerMapView::init(bool highlightFirst)
{
	uint i = 0, iMaps = 0;
	assert(EmuSystem::maxPlayers <= 5);
	#ifdef INPUT_SUPPORTS_POINTER
	inputMap[iMaps].init("Touch Screen", &pointerInputPlayer); item[i] = &inputMap[iMaps++]; i++;
	#endif
	#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3) && defined(INPUT_SUPPORTS_KEYBOARD)
		#ifdef CONFIG_BASE_ANDROID
		if(Base::androidSDK() >= 12)
		{
			iterateTimes(EmuSystem::maxPlayers, p)
			{
				static const char *str[] = { "Keyboard/HID Gamepad 1", "HID Gamepad 2", "HID Gamepad 3", "HID Gamepad 4", "HID Gamepad 5" };
				inputMap[iMaps].init(str[p], &keyboardInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
			}
		}
		else
		#endif
		{
			inputMap[iMaps].init("Keyboard", &keyboardInputPlayer[0]); item[i] = &inputMap[iMaps++]; i++;
		}
	#endif
	#ifdef CONFIG_INPUT_ICADE
	inputMap[iMaps].init("iCade", &iCadeInputPlayer); item[i] = &inputMap[iMaps++]; i++;
	#endif
	#if defined(CONFIG_BASE_PS3)
	iterateTimes((int)EmuSystem::maxPlayers, p)
	{
		static const char *str[] = { "Controller 1", "Controller 2", "Controller 3", "Controller 4", "Controller 5" };
		inputMap[iMaps].init(str[p], &gamepadInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH
	iterateTimes((int)EmuSystem::maxPlayers, p)
	{
		static const char *str[] = { "Wiimote 1", "Wiimote 2", "Wiimote 3", "Wiimote 4", "Wiimote 5" };
		inputMap[iMaps].init(str[p], &wiimoteInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
	}
	iterateTimes((int)EmuSystem::maxPlayers, p)
	{
		static const char *str[] = { "iControlPad 1", "iControlPad 2", "iControlPad 3", "iControlPad 4", "iControlPad 5" };
		inputMap[iMaps].init(str[p], &iControlPadInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
	}
	iterateTimes((int)EmuSystem::maxPlayers, p)
	{
		static const char *str[] = { "Zeemote JS1 1", "Zeemote JS1 2", "Zeemote JS1 3", "Zeemote JS1 4", "Zeemote JS1 5" };
		inputMap[iMaps].init(str[p], &zeemoteInputPlayer[p]); item[i] = &inputMap[iMaps++]; i++;
	}
	#endif
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

void loadGameCompleteFromRecentItem(uint result = 1)
{
	if(result)
	{
		startGameFromMenu();
	}
}

void RecentGameInfo::handleMenuSelection(TextMenuItem &, const InputEvent &e)
{
	FsSys::cPath dir, file;
	dirName(path, dir);
	baseName(path, file);
	FsSys::chdir(dir);
	EmuSystem::loadGameCompleteDelegate().bind<&loadGameCompleteFromRecentItem>();
	if(EmuSystem::loadGame(file))
	{
		loadGameCompleteFromRecentItem();
	}
}

void RecentGameView::clearRecentMenuHandler(TextMenuItem &, const InputEvent &e)
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
	clear.init("Clear List", recentGameList.size); item[i++] = &clear;
	clear.selectDelegate().bind<&clearRecentMenuHandler>();
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

void OptionCategoryView::init(bool highlightFirst)
{
	//logMsg("running option category init");
	uint i = 0;
	static const char *name[] = { "Video", "Audio", "Input", "System", "GUI" };
	forEachInArray(subConfig, e)
	{
		e->init(name[e_i]); item[i++] = e;
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}
