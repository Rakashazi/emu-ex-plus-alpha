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
#include <emuframework/FilePicker.hh>
#include "EmuOptions.hh"
#include <imagine/base/Base.hh>
#include <imagine/gui/TextTableView.hh>
#include "private.hh"

static FS::PathString savePathStrToDescStr(char *savePathStr)
{
	FS::PathString desc{};
	if(strlen(savePathStr))
	{
		if(string_equal(savePathStr, optionSavePathDefaultToken))
			string_copy(desc, "Default");
		else
		{
			string_copy(desc, FS::basename(optionSavePath).data());
		}
	}
	else
	{
		string_copy(desc, "Same as Game");
	}
	return desc;
}

BiosSelectMenu::BiosSelectMenu(NameString name, ViewAttachParams attach, FS::PathString *biosPathStr_, BiosChangeDelegate onBiosChange_,
	EmuSystem::NameFilterFunc fsFilter_):
	TableView
	{
		std::move(name),
		attach,
		[this](const TableView &)
		{
			return 2;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return selectFile;
				default: return unset;
			}
		}
	},
	selectFile
	{
		"Select File",
		[this](Input::Event e)
		{
			auto startPath = strlen(biosPathStr->data()) ? FS::dirname(*biosPathStr) : lastLoadPath;
			auto fPicker = makeView<EmuFilePicker>(startPath.data(), false, fsFilter, FS::RootPathInfo{}, e);
			fPicker->setOnSelectFile(
				[this](FSPicker &picker, const char* name, Input::Event e)
				{
					*biosPathStr = picker.makePathString(name);
					onBiosChangeD.callSafe();
					dismiss();
					picker.dismiss();
				});
			pushAndShowModal(std::move(fPicker), e);
		}
	},
	unset
	{
		"Unset",
		[this]()
		{
			strcpy(biosPathStr->data(), "");
			auto onBiosChange = onBiosChangeD;
			onBiosChange.callSafe();
			dismiss();
		}
	},
	onBiosChangeD{onBiosChange_},
	biosPathStr{biosPathStr_},
	fsFilter{fsFilter_}
{
	assert(biosPathStr);
}

static void setAutoSaveState(uint val)
{
	optionAutoSaveState = val;
	logMsg("set auto-savestate %d", optionAutoSaveState.val);
}

static std::array<char, 256> makePathMenuEntryStr(PathOption optionSavePath)
{
	return string_makePrintf<256>("Save Path: %s", savePathStrToDescStr(optionSavePath).data());
}

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autoSaveStateItem
	{
		{"Off", [this]() { setAutoSaveState(0); }},
		{"Game Exit", [this]() { setAutoSaveState(1); }},
		{"15mins", [this]() { setAutoSaveState(15); }},
		{"30mins", [this]() { setAutoSaveState(30); }}
	},
	autoSaveState
	{
		"Auto-save State",
		[]()
		{
			switch(optionAutoSaveState.val)
			{
				default: return 0;
				case 1: return 1;
				case 15: return 2;
				case 30: return 3;
			}
		}(),
		autoSaveStateItem
	},
	confirmAutoLoadState
	{
		"Confirm Auto-load State",
		(bool)optionConfirmAutoLoadState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmAutoLoadState = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State",
		(bool)optionConfirmOverwriteState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmOverwriteState = item.flipBoolValue(*this);
		}
	},
	savePath
	{
		nullptr,
		[this](TextMenuItem &, View &view, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Save Path", 3);
			multiChoiceView->appendItem("Set Custom Path",
				[this](Input::Event e)
				{
					auto startPath = strlen(optionSavePath) ? optionSavePath : optionLastLoadPath;
					auto fPicker = makeView<EmuFilePicker>(startPath, true,
						EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
					fPicker->setOnClose(
						[this](FSPicker &picker, Input::Event e)
						{
							EmuSystem::savePath_ = picker.path();
							logMsg("set save path %s", (char*)optionSavePath);
							onSavePathChange(optionSavePath);
							dismissPrevious();
							picker.dismiss();
						});
					pushAndShowModal(std::move(fPicker), e);
				});
			multiChoiceView->appendItem("Same as Game",
				[this](View &view)
				{
					strcpy(optionSavePath, "");
					onSavePathChange("");
					view.dismiss();
				});
			multiChoiceView->appendItem("Default",
				[this](View &view)
				{
					strcpy(optionSavePath, optionSavePathDefaultToken);
					onSavePathChange(optionSavePathDefaultToken);
					view.dismiss();
				});
			pushAndShow(std::move(multiChoiceView), e);
			postDraw();
		}
	},
	checkSavePathWriteAccess
	{
		"Check Save Path Write Access",
		(bool)optionCheckSavePathWriteAccess,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionCheckSavePathWriteAccess = item.flipBoolValue(*this);
		}
	},
	fastForwardSpeedItem
	{
		{"2x", [this]() { optionFastForwardSpeed = 2; }},
		{"3x", [this]() { optionFastForwardSpeed = 3; }},
		{"4x", [this]() { optionFastForwardSpeed = 4; }},
		{"5x", [this]() { optionFastForwardSpeed = 5; }},
		{"6x", [this]() { optionFastForwardSpeed = 6; }},
		{"7x", [this]() { optionFastForwardSpeed = 7; }},
	},
	fastForwardSpeed
	{
		"Fast Forward Speed",
		[]() -> int
		{
			if(optionFastForwardSpeed >= MIN_FAST_FORWARD_SPEED && optionFastForwardSpeed <= 7)
			{
				return optionFastForwardSpeed - MIN_FAST_FORWARD_SPEED;
			}
			return 0;
		}(),
		fastForwardSpeedItem
	}
	#if defined __ANDROID__
	,processPriorityItem
	{
		{
			"Normal",
			[this]()
			{
				optionProcessPriority = 0;
				Base::setProcessPriority(optionProcessPriority);
			}
		},
		{
			"High",
			[this]()
			{
				optionProcessPriority = -6;
				Base::setProcessPriority(optionProcessPriority);
			}
		},
		{
			"Very High",
			[this]()
			{
				optionProcessPriority = -14;
				Base::setProcessPriority(optionProcessPriority);
			}
		}
	},
	processPriority
	{
		"Process Priority",
		[]()
		{
			switch(optionProcessPriority.val)
			{
				default: return 0;
				case -6: return 1;
				case -14: return 2;
			}
		}(),
		processPriorityItem
	},
	performanceMode
	{
		"Performance Mode",
		(bool)optionSustainedPerformanceMode,
		"Normal", "Sustained",
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionSustainedPerformanceMode = item.flipBoolValue(*this);
		}
	}
	#endif
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void SystemOptionView::loadStockItems()
{
	item.emplace_back(&autoSaveState);
	item.emplace_back(&confirmAutoLoadState);
	item.emplace_back(&confirmOverwriteState);
	savePath.setName(makePathMenuEntryStr(optionSavePath).data());
	item.emplace_back(&savePath);
	item.emplace_back(&checkSavePathWriteAccess);
	item.emplace_back(&fastForwardSpeed);
	#ifdef __ANDROID__
	item.emplace_back(&processPriority);
	if(!optionSustainedPerformanceMode.isConst)
		item.emplace_back(&performanceMode);
	#endif
}

void SystemOptionView::onSavePathChange(const char *path)
{
	if(string_equal(path, optionSavePathDefaultToken))
	{
		auto defaultPath = EmuSystem::baseDefaultGameSavePath();
		EmuApp::printfMessage(4, false, "Default Save Path:\n%s", defaultPath.data());
	}
	savePath.compile(makePathMenuEntryStr(optionSavePath).data(), renderer(), projP);
	EmuSystem::setupGameSavePath();
	EmuSystem::savePathChanged();
}

void SystemOptionView::onFirmwarePathChange(const char *path, Input::Event e) {}

void SystemOptionView::pushAndShowFirmwarePathMenu(const char *name, Input::Event e, bool allowFiles)
{
	auto multiChoiceView = std::make_unique<TextTableView>(name, attachParams(), allowFiles ? 3 : 2);
	multiChoiceView->appendItem("Set Custom Path",
		[this](Input::Event e)
		{
			auto startPath =  EmuApp::firmwareSearchPath();
			auto fPicker = makeView<EmuFilePicker>(startPath.data(), true,
				EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
			fPicker->setOnClose(
				[this](FSPicker &picker, Input::Event e)
				{
					auto path = picker.path();
					EmuApp::setFirmwareSearchPath(path.data());
					logMsg("set firmware path:%s", path.data());
					onFirmwarePathChange(path.data(), e);
					dismissPrevious();
					picker.dismiss();
				});
			EmuApp::pushAndShowModalView(std::move(fPicker), e);
		});
	if(allowFiles)
	{
		multiChoiceView->appendItem("Set Custom Archive File",
			[this](Input::Event e)
			{
				auto startPath =  EmuApp::firmwareSearchPath();
				auto fPicker = makeView<EmuFilePicker>(startPath.data(), false,
					EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
				fPicker->setOnSelectFile(
					[this](FSPicker &picker, const char *name, Input::Event e)
					{
						auto path = picker.makePathString(name);
						EmuApp::setFirmwareSearchPath(path.data());
						logMsg("set firmware archive file:%s", path.data());
						onFirmwarePathChange(path.data(), e);
						dismissPrevious();
						picker.dismiss();
					});
				EmuApp::pushAndShowModalView(std::move(fPicker), e);
			});
	}
	multiChoiceView->appendItem("Default",
		[this](View &view, Input::Event e)
		{
			EmuApp::setFirmwareSearchPath("");
			onFirmwarePathChange("", e);
			view.dismiss();
		});
	pushAndShow(std::move(multiChoiceView), e);
}

void SystemOptionView::pushAndShowFirmwareFilePathMenu(const char *name, Input::Event e)
{
	pushAndShowFirmwarePathMenu(name, e, true);
}

