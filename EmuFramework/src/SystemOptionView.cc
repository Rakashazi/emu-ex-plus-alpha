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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

static FS::PathString savePathStrToDescStr(std::string_view savePathStr)
{
	if(savePathStr.size())
	{
		if(savePathStr == optionSavePathDefaultToken)
			return "Default";
		else
		{
			return FS::basename(savePathStr);
		}
	}
	else
	{
		return "Content Path";
	}
}

BiosSelectMenu::BiosSelectMenu(IG::utf16String name, ViewAttachParams attach, FS::PathString *biosPathStr_, BiosChangeDelegate onBiosChange_,
	EmuSystem::NameFilterFunc fsFilter_):
	TableView
	{
		std::move(name),
		attach,
		[this](const TableView &)
		{
			return 2;
		},
		[this](const TableView &, size_t idx) -> MenuItem&
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
		"Select File", &defaultFace(),
		[this](Input::Event e)
		{
			auto startPath = biosPathStr->size() ? FS::dirname(*biosPathStr) : app().mediaSearchPath();
			auto fPicker = makeView<EmuFilePicker>(startPath, false, fsFilter, FS::RootPathInfo{}, e);
			fPicker->setOnSelectFile(
				[this](FSPicker &picker, std::string_view name, Input::Event e)
				{
					*biosPathStr = picker.pathString(name);
					onBiosChangeD.callSafe();
					dismiss();
					picker.dismiss();
				});
			pushAndShowModal(std::move(fPicker), e);
		}
	},
	unset
	{
		"Unset", &defaultFace(),
		[this]()
		{
			biosPathStr->clear();
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

static void setAutoSaveState(unsigned val)
{
	optionAutoSaveState = val;
	logMsg("set auto-savestate %d", optionAutoSaveState.val);
}

static auto makePathMenuEntryStr(std::string_view savePath)
{
	return fmt::format("Save Path: {}", savePathStrToDescStr(savePath));
}

static bool hasWriteAccessToDir(Base::ApplicationContext ctx, IG::CStringView path)
{
	auto hasAccess = FS::access(path, FS::acc::w);
	#ifdef __ANDROID__
	// on Android 4.4 also test file creation since
	// access() can still claim an SD card is writable
	// even though parts are locked-down by the OS
	if(ctx.androidSDK() >= 19)
	{
		auto testFilePath = FS::pathString(path, ".safe-to-delete-me");
		auto testFile = FileIO::create(testFilePath, IO::OPEN_TEST);
		if(!testFile)
		{
			hasAccess = false;
		}
		else
		{
			FS::remove(testFilePath);
		}
	}
	#endif
	return hasAccess;
}

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autoSaveStateItem
	{
		{"Off", &defaultFace(), [this]() { setAutoSaveState(0); }},
		{"Game Exit", &defaultFace(), [this]() { setAutoSaveState(1); }},
		{"15mins", &defaultFace(), [this]() { setAutoSaveState(15); }},
		{"30mins", &defaultFace(), [this]() { setAutoSaveState(30); }}
	},
	autoSaveState
	{
		"Auto-save State", &defaultFace(),
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
		"Confirm Auto-load State", &defaultFace(),
		(bool)optionConfirmAutoLoadState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmAutoLoadState = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State", &defaultFace(),
		(bool)optionConfirmOverwriteState,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionConfirmOverwriteState = item.flipBoolValue(*this);
		}
	},
	savePath
	{
		{}, &defaultFace(),
		[this](TextMenuItem &, View &view, Input::Event e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Save Path", 3);
			multiChoiceView->appendItem("Set Custom Path",
				[this](Input::Event e)
				{
					auto startPath = EmuSystem::userSavePath().size() ? EmuSystem::userSavePath() : app().mediaSearchPath();
					auto fPicker = makeView<EmuFilePicker>(startPath, true,
						EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
					fPicker->setOnClose(
						[this](FSPicker &picker, Input::Event e)
						{
							auto path = picker.path();
							if(!hasWriteAccessToDir(appContext(), path))
							{
								app().postErrorMessage("This directory lacks write access");
								return;
							}
							EmuSystem::setUserSavePath(appContext(), path);
							onSavePathChange(path);
							dismissPrevious();
							picker.dismiss();
						});
					pushAndShowModal(std::move(fPicker), e);
				});
			multiChoiceView->appendItem("Same as Content",
				[this](View &view)
				{
					EmuSystem::setUserSavePath(appContext(), "");
					onSavePathChange("");
					view.dismiss();
				});
			multiChoiceView->appendItem("Default",
				[this](View &view)
				{
					EmuSystem::setUserSavePath(appContext(), optionSavePathDefaultToken);
					onSavePathChange(optionSavePathDefaultToken);
					view.dismiss();
				});
			pushAndShow(std::move(multiChoiceView), e);
			postDraw();
		}
	},
	fastForwardSpeedItem
	{
		{"2x", &defaultFace(), [this]() { optionFastForwardSpeed = 2; }},
		{"3x", &defaultFace(), [this]() { optionFastForwardSpeed = 3; }},
		{"4x", &defaultFace(), [this]() { optionFastForwardSpeed = 4; }},
		{"5x", &defaultFace(), [this]() { optionFastForwardSpeed = 5; }},
		{"6x", &defaultFace(), [this]() { optionFastForwardSpeed = 6; }},
		{"7x", &defaultFace(), [this]() { optionFastForwardSpeed = 7; }},
	},
	fastForwardSpeed
	{
		"Fast Forward Speed", &defaultFace(),
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
	,performanceMode
	{
		"Performance Mode", &defaultFace(),
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
	savePath.setName(makePathMenuEntryStr(EmuSystem::userSavePath()));
	item.emplace_back(&savePath);
	item.emplace_back(&fastForwardSpeed);
	#ifdef __ANDROID__
	if(!optionSustainedPerformanceMode.isConst)
		item.emplace_back(&performanceMode);
	#endif
}

void SystemOptionView::onSavePathChange(std::string_view path)
{
	if(path == optionSavePathDefaultToken)
	{
		auto defaultPath = EmuSystem::baseDefaultGameSavePath(appContext());
		app().postMessage(4, false, fmt::format("Default Save Path:\n{}", defaultPath));
	}
	savePath.compile(makePathMenuEntryStr(path), renderer(), projP);
}

void SystemOptionView::onFirmwarePathChange(std::string_view path, Input::Event e) {}

std::unique_ptr<TextTableView> SystemOptionView::makeFirmwarePathMenu(IG::utf16String name, bool allowFiles, unsigned extraItemsHint)
{
	unsigned items = (allowFiles ? 3 : 2) + extraItemsHint;
	auto multiChoiceView = std::make_unique<TextTableView>(std::move(name), attachParams(), items);
	multiChoiceView->appendItem("Set Custom Path",
		[this](Input::Event e)
		{
			auto startPath =  app().firmwareSearchPath();
			auto fPicker = makeView<EmuFilePicker>(startPath, true,
				EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
			fPicker->setOnClose(
				[this](FSPicker &picker, Input::Event e)
				{
					auto path = picker.path();
					app().setFirmwareSearchPath(path);
					logMsg("set firmware path:%s", path.data());
					onFirmwarePathChange(path.data(), e);
					dismissPrevious();
					picker.dismiss();
				});
			app().pushAndShowModalView(std::move(fPicker), e);
		});
	if(allowFiles)
	{
		multiChoiceView->appendItem("Set Custom Archive File",
			[this](Input::Event e)
			{
				auto startPath =  app().firmwareSearchPath();
				auto fPicker = makeView<EmuFilePicker>(startPath, false,
					EmuSystem::NameFilterFunc{}, FS::RootPathInfo{}, e);
				fPicker->setOnSelectFile(
					[this](FSPicker &picker, std::string_view name, Input::Event e)
					{
						auto path = picker.pathString(name);
						app().setFirmwareSearchPath(path);
						logMsg("set firmware archive file:%s", path.data());
						onFirmwarePathChange(path.data(), e);
						dismissPrevious();
						picker.dismiss();
					});
				app().pushAndShowModalView(std::move(fPicker), e);
			});
	}
	multiChoiceView->appendItem("Default",
		[this](View &view, Input::Event e)
		{
			app().setFirmwareSearchPath("");
			onFirmwarePathChange("", e);
			view.dismiss();
		});
	return multiChoiceView;
}

void SystemOptionView::pushAndShowFirmwarePathMenu(IG::utf16String name, Input::Event e, bool allowFiles)
{
	pushAndShow(makeFirmwarePathMenu(std::move(name), allowFiles), e);
}

void SystemOptionView::pushAndShowFirmwareFilePathMenu(IG::utf16String name, Input::Event e)
{
	pushAndShowFirmwarePathMenu(std::move(name), e, true);
}

