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
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ScopeGuard.hh>

namespace EmuEx
{

static FS::PathString savePathStrToDescStr(IG::ApplicationContext ctx, std::string_view savePathStr)
{
	if(savePathStr.size())
	{
		if(savePathStr == optionSavePathDefaultToken)
			return "App Folder";
		else
			return ctx.fileUriDisplayName(savePathStr);
	}
	else
	{
		return "Content Folder";
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
		[this](const Input::Event &e)
		{
			auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::FILE, fsFilter, e);
			fPicker->setPath(biosPathStr->size() ? FS::dirnameUri(*biosPathStr) : app().contentSearchPath(), e);
			fPicker->setOnSelectFile(
				[this](FSPicker &picker, std::string_view path, std::string_view displayName, const Input::Event &e)
				{
					*biosPathStr = path;
					onBiosChangeD.callSafe(displayName);
					popTo(*this);
					dismiss();
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
			onBiosChange.callSafe(std::string_view{});
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

static auto makePathMenuEntryStr(IG::ApplicationContext ctx, std::string_view savePath)
{
	return fmt::format("Save Path: {}", savePathStrToDescStr(ctx, savePath));
}

static bool hasWriteAccessToDir(IG::ApplicationContext ctx, IG::CStringView path)
{
	// on Android test file creation since
	// access() can still claim emulated storage is writable
	// even though parts are locked-down by the OS (like on 4.4+)
	if constexpr(Config::envIsAndroid)
	{
		auto testFilePath = FS::uriString(path, ".safe-to-delete-me");
		auto testFile = ctx.openFileUri(testFilePath, IO::OPEN_CREATE | IO::OPEN_TEST);
		auto removeTestFile = IG::scopeGuard([&]() { if(testFile) ctx.removeFileUri(testFilePath); });
		return (bool)testFile;
	}
	else
	{
		assert(!IG::isUri(path));
		return FS::access(path, FS::acc::w);
	}
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
		[this](BoolMenuItem &item)
		{
			optionConfirmAutoLoadState = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State", &defaultFace(),
		(bool)optionConfirmOverwriteState,
		[this](BoolMenuItem &item)
		{
			optionConfirmOverwriteState = item.flipBoolValue(*this);
		}
	},
	savePath
	{
		{}, &defaultFace(),
		[this](TextMenuItem &, View &view, const Input::Event &e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Save Path", 4);
			multiChoiceView->appendItem("Select Folder",
				[this](const Input::Event &e)
				{
					auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
					auto userSavePath = EmuSystem::userSaveDirectory();
					fPicker->setPath(userSavePath.size() && userSavePath != optionSavePathDefaultToken ? userSavePath
						: app().contentSearchPath(), e);
					fPicker->setOnClose(
						[this](FSPicker &picker, const Input::Event &e)
						{
							if(e.keyEvent() && e.asKeyEvent().pushed(Input::DefaultKey::CANCEL))
							{
								picker.dismiss();
								return;
							}
							auto path = picker.path();
							if(!hasWriteAccessToDir(appContext(), path))
							{
								app().postErrorMessage("This folder lacks write access");
								return;
							}
							EmuSystem::setUserSaveDirectory(appContext(), path);
							onSavePathChange(path);
							dismissPrevious();
							picker.dismiss();
						});
					pushAndShowModal(std::move(fPicker), e);
				});
			multiChoiceView->appendItem("Same As Content",
				[this](View &view)
				{
					EmuSystem::setUserSaveDirectory(appContext(), "");
					onSavePathChange("");
					view.dismiss();
				});
			multiChoiceView->appendItem("App Folder",
				[this](View &view)
				{
					EmuSystem::setUserSaveDirectory(appContext(), optionSavePathDefaultToken);
					onSavePathChange(optionSavePathDefaultToken);
					view.dismiss();
				});
			multiChoiceView->appendItem("Legacy Game Data Folder",
				[this](View &view, const Input::Event &e)
				{
					auto ynAlertView = makeView<YesNoAlertView>(
						fmt::format("Please select the \"Game Data/{}\" folder from an old version of the app to use its existing saves and convert it to a regular save path (this is only needed once)", EmuSystem::shortSystemName()));
					ynAlertView->setOnYes(
						[this](const Input::Event &e)
						{
							auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
							fPicker->setPath("");
							fPicker->setOnClose(
								[this](FSPicker &picker, const Input::Event &e)
								{
									if(e.keyEvent() && e.asKeyEvent().isDefaultCancelButton())
									{
										picker.dismiss();
										return;
									}
									auto path = picker.path();
									auto ctx = appContext();
									if(!hasWriteAccessToDir(ctx, path))
									{
										app().postErrorMessage("This folder lacks write access");
										return;
									}
									if(ctx.fileUriDisplayName(path) != EmuSystem::shortSystemName())
									{
										app().postErrorMessage(fmt::format("Please select the {} folder", EmuSystem::shortSystemName()));
										return;
									}
									EmuApp::updateLegacySavePath(ctx, path);
									EmuSystem::setUserSaveDirectory(appContext(), path);
									onSavePathChange(path);
									dismissPrevious();
									picker.dismiss();
								});
							pushAndShowModal(std::move(fPicker), e);
						});
					pushAndShowModal(std::move(ynAlertView), e);
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
		[this](BoolMenuItem &item)
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
	savePath.setName(makePathMenuEntryStr(appContext(), EmuSystem::userSaveDirectory()));
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
		app().postMessage(4, false, fmt::format("App Folder:\n{}", EmuSystem::fallbackSaveDirectory(appContext())));
	}
	savePath.compile(makePathMenuEntryStr(appContext(), path), renderer(), projP);
}

bool SystemOptionView::onFirmwarePathChange(IG::CStringView path, bool isDir) { return true; }

std::unique_ptr<TextTableView> SystemOptionView::makeFirmwarePathMenu(IG::utf16String name, bool allowFiles, unsigned extraItemsHint)
{
	unsigned items = (allowFiles ? 3 : 2) + extraItemsHint;
	auto multiChoiceView = std::make_unique<TextTableView>(std::move(name), attachParams(), items);
	multiChoiceView->appendItem("Select Folder",
		[this](const Input::Event &e)
		{
			auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
			fPicker->setPath(app().firmwareSearchPath(), e);
			fPicker->setOnClose(
				[this](FSPicker &picker, const Input::Event &e)
				{
					if(e.keyEvent() && e.asKeyEvent().isDefaultCancelButton())
					{
						picker.dismiss();
						return;
					}
					auto path = picker.path();
					logMsg("set firmware path:%s", path.data());
					if(!onFirmwarePathChange(path, true))
						return;
					app().setFirmwareSearchPath(path);
					dismissPrevious();
					picker.dismiss();
				});
			app().pushAndShowModalView(std::move(fPicker), e);
		});
	if(allowFiles)
	{
		multiChoiceView->appendItem("Select Archive File",
			[this](const Input::Event &e)
			{
				auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::FILE, EmuSystem::NameFilterFunc{}, e);
				fPicker->setPath(app().firmwareSearchPath(), e);
				fPicker->setOnSelectFile(
					[this](FSPicker &picker, IG::CStringView path, std::string_view displayName, const Input::Event &e)
					{
						if(!EmuApp::hasArchiveExtension(displayName))
						{
							app().postErrorMessage("File doesn't have a valid extension");
							return;
						}
						logMsg("set firmware archive file:%s", path.data());
						if(!onFirmwarePathChange(path, false))
							return;
						app().setFirmwareSearchPath(path);
						popTo(picker);
						dismissPrevious();
						picker.dismiss();
					});
				app().pushAndShowModalView(std::move(fPicker), e);
			});
	}
	multiChoiceView->appendItem("Unset",
		[this](View &view)
		{
			onFirmwarePathChange("", false);
			app().setFirmwareSearchPath("");
			view.dismiss();
		});
	return multiChoiceView;
}

void SystemOptionView::pushAndShowFirmwarePathMenu(IG::utf16String name, const Input::Event &e, bool allowFiles)
{
	pushAndShow(makeFirmwarePathMenu(std::move(name), allowFiles), e);
}

void SystemOptionView::pushAndShowFirmwareFilePathMenu(IG::utf16String name, const Input::Event &e)
{
	pushAndShowFirmwarePathMenu(std::move(name), e, true);
}

}
