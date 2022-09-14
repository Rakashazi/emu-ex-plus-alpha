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
#include <imagine/util/format.hh>
#include "pathUtils.hh"

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

BiosSelectMenu::BiosSelectMenu(UTF16String name, ViewAttachParams attach, FS::PathString *biosPathStr_, BiosChangeDelegate onBiosChange_,
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
			fPicker->setOnSelectPath(
				[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
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

TextMenuItem::SelectDelegate SystemOptionView::setAutoSaveStateDel()
{
	return [this](TextMenuItem &item)
	{
		app().autoSaveStateOption() = item.id();
		logMsg("set auto-savestate:%u", item.id());
	};
}

TextMenuItem::SelectDelegate SystemOptionView::setFastSlowModeSpeedDel()
{
	return [this](TextMenuItem &item) { app().fastSlowModeSpeedOption() = item.id(); };
}

static auto savesMenuEntryStr(IG::ApplicationContext ctx, std::string_view savePath)
{
	return fmt::format("Saves: {}", savePathStrToDescStr(ctx, savePath));
}

SystemOptionView::SystemOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"System Options", attach, item},
	autoSaveStateItem
	{
		{"Off",       &defaultFace(), setAutoSaveStateDel(), 0},
		{"Game Exit", &defaultFace(), setAutoSaveStateDel(), 1},
		{"15mins",    &defaultFace(), setAutoSaveStateDel(), 15},
		{"30mins",    &defaultFace(), setAutoSaveStateDel(), 30},
	},
	autoSaveState
	{
		"Auto-save State", &defaultFace(),
		(MenuItem::Id)app().autoSaveStateOption().val,
		autoSaveStateItem
	},
	confirmAutoLoadState
	{
		"Confirm Auto-load State", &defaultFace(),
		(bool)app().confirmAutoLoadStateOption(),
		[this](BoolMenuItem &item)
		{
			app().confirmAutoLoadStateOption() = item.flipBoolValue(*this);
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State", &defaultFace(),
		(bool)app().confirmOverwriteStateOption(),
		[this](BoolMenuItem &item)
		{
			app().confirmOverwriteStateOption() = item.flipBoolValue(*this);
		}
	},
	fastSlowModeSpeedItem
	{
		{"0.25x", &defaultFace(), setFastSlowModeSpeedDel(), 25},
		{"0.50x", &defaultFace(), setFastSlowModeSpeedDel(), 50},
		{"1.5x",  &defaultFace(), setFastSlowModeSpeedDel(), 150},
		{"2x",    &defaultFace(), setFastSlowModeSpeedDel(), 200},
		{"4x",    &defaultFace(), setFastSlowModeSpeedDel(), 400},
		{"8x",    &defaultFace(), setFastSlowModeSpeedDel(), 800},
		{"16x",   &defaultFace(), setFastSlowModeSpeedDel(), 1600},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 0.05 to 20.0", "",
					[this](EmuApp &app, auto val)
					{
						if(val >= MIN_RUN_SPEED && val <= MAX_RUN_SPEED)
						{
							auto valAsInt = std::round(val * 100.);
							app.fastSlowModeSpeedOption() = valAsInt;
							fastSlowModeSpeed.setSelected((MenuItem::Id)valAsInt, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	fastSlowModeSpeed
	{
		"Fast/Slow Mode Speed", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{:.2f}x", app().fastSlowModeSpeedAsDouble()));
			return true;
		},
		(MenuItem::Id)app().fastSlowModeSpeedOption().val,
		fastSlowModeSpeedItem
	},
	performanceMode
	{
		"Performance Mode", &defaultFace(),
		(bool)app().sustainedPerformanceModeOption(),
		"Normal", "Sustained",
		[this](BoolMenuItem &item)
		{
			app().sustainedPerformanceModeOption() = item.flipBoolValue(*this);
		}
	}
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
	item.emplace_back(&fastSlowModeSpeed);
	if(used(performanceMode))
		item.emplace_back(&performanceMode);
}

FilePathOptionView::FilePathOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"File Path Options", attach, item},
	savePath
	{
		savesMenuEntryStr(appContext(), system().userSaveDirectory()), &defaultFace(),
		[this](const Input::Event &e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Saves", 4);
			multiChoiceView->appendItem("Select Folder",
				[this](const Input::Event &e)
				{
					auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
					auto userSavePath = system().userSaveDirectory();
					fPicker->setPath(userSavePath.size() && userSavePath != optionSavePathDefaultToken ? userSavePath
						: app().contentSearchPath(), e);
					fPicker->setOnSelectPath(
						[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
						{
							if(!hasWriteAccessToDir(path))
							{
								app().postErrorMessage("This folder lacks write access");
								return;
							}
							system().setUserSaveDirectory(path);
							onSavePathChange(path);
							dismissPrevious();
							picker.dismiss();
						});
					pushAndShowModal(std::move(fPicker), e);
				});
			multiChoiceView->appendItem("Same As Content",
				[this](View &view)
				{
					system().setUserSaveDirectory("");
					onSavePathChange("");
					view.dismiss();
				});
			multiChoiceView->appendItem("App Folder",
				[this](View &view)
				{
					system().setUserSaveDirectory(optionSavePathDefaultToken);
					onSavePathChange(optionSavePathDefaultToken);
					view.dismiss();
				});
			multiChoiceView->appendItem("Legacy Game Data Folder",
				[this](View &view, const Input::Event &e)
				{
					auto ynAlertView = makeView<YesNoAlertView>(
						fmt::format("Please select the \"Game Data/{}\" folder from an old version of the app to use its existing saves and convert it to a regular save path (this is only needed once)", system().shortSystemName()));
					ynAlertView->setOnYes(
						[this](const Input::Event &e)
						{
							auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
							fPicker->setPath("");
							fPicker->setOnSelectPath(
								[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
								{
									auto ctx = appContext();
									if(!hasWriteAccessToDir(path))
									{
										app().postErrorMessage("This folder lacks write access");
										return;
									}
									if(ctx.fileUriDisplayName(path) != system().shortSystemName())
									{
										app().postErrorMessage(fmt::format("Please select the {} folder", system().shortSystemName()));
										return;
									}
									EmuApp::updateLegacySavePath(ctx, path);
									system().setUserSaveDirectory(path);
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
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void FilePathOptionView::loadStockItems()
{
	item.emplace_back(&savePath);
}

void FilePathOptionView::onSavePathChange(std::string_view path)
{
	if(path == optionSavePathDefaultToken)
	{
		app().postMessage(4, false, fmt::format("App Folder:\n{}", system().fallbackSaveDirectory()));
	}
	savePath.compile(savesMenuEntryStr(appContext(), path), renderer(), projP);
}

bool FilePathOptionView::onFirmwarePathChange(IG::CStringView path, bool isDir) { return true; }

std::unique_ptr<TextTableView> FilePathOptionView::makeFirmwarePathMenu(UTF16String name, bool allowFiles, int extraItemsHint)
{
	int items = (allowFiles ? 3 : 2) + extraItemsHint;
	auto multiChoiceView = std::make_unique<TextTableView>(std::move(name), attachParams(), items);
	multiChoiceView->appendItem("Select Folder",
		[this](const Input::Event &e)
		{
			auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
			fPicker->setPath(app().firmwareSearchPath(), e);
			fPicker->setOnSelectPath(
				[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
				{
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
				fPicker->setOnSelectPath(
					[this](FSPicker &picker, CStringView path, std::string_view displayName, const Input::Event &e)
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

}
