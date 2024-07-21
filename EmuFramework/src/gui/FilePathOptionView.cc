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

#include <emuframework/FilePathOptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/UserPathSelectView.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include "../pathUtils.hh"
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"FilePathOptionView"};

static FS::FileString savePathStrToDisplayName(IG::ApplicationContext ctx, std::string_view savePathStr)
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

static auto savesMenuName(IG::ApplicationContext ctx, std::string_view savePath)
{
	return std::format("Saves: {}", savePathStrToDisplayName(ctx, savePath));
}

static auto screenshotsMenuName(IG::ApplicationContext ctx, std::string_view userPath)
{
	return std::format("Screenshots: {}", userPathToDisplayName(ctx, userPath));
}

FilePathOptionView::FilePathOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"File Path Options", attach, item},
	savePath
	{
		savesMenuName(appContext(), system().userSaveDirectory()), attach,
		[this](const Input::Event &e)
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Saves", 4);
			multiChoiceView->appendItem("Select Folder",
				[this](const Input::Event &e)
				{
					auto fPicker = makeView<FilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
					auto userSavePath = system().userSaveDirectory();
					fPicker->setPath(userSavePath.size() && userSavePath != optionSavePathDefaultToken ? userSavePath
						: app().contentSearchPath, e);
					fPicker->setOnSelectPath(
						[this](FSPicker &picker, CStringView path, [[maybe_unused]] std::string_view displayName, const Input::Event&)
						{
							if(!hasWriteAccessToDir(path))
							{
								app().postErrorMessage("This folder lacks write access");
								return;
							}
							system().setUserSaveDirectory(path);
							onSavePathChange(path);
							picker.popTo();
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
				[this](View&, const Input::Event &e)
				{
					pushAndShowModal(makeView<YesNoAlertView>(
						std::format("Please select the \"Game Data/{}\" folder from an old version of the app to use its existing saves "
							"and convert it to a regular save path (this is only needed once)", system().shortSystemName()),
						YesNoAlertView::Delegates
						{
							.onYes = [this](const Input::Event &e)
							{
								auto fPicker = makeView<FilePicker>(FSPicker::Mode::DIR, EmuSystem::NameFilterFunc{}, e);
								fPicker->setPath("");
								fPicker->setOnSelectPath(
									[this](FSPicker &picker, CStringView path, [[maybe_unused]] std::string_view displayName, const Input::Event&)
									{
										auto ctx = appContext();
										if(!hasWriteAccessToDir(path))
										{
											app().postErrorMessage("This folder lacks write access");
											return;
										}
										if(ctx.fileUriDisplayName(path) != system().shortSystemName())
										{
											app().postErrorMessage(std::format("Please select the {} folder", system().shortSystemName()));
											return;
										}
										EmuApp::updateLegacySavePath(ctx, path);
										system().setUserSaveDirectory(path);
										onSavePathChange(path);
										picker.popTo();
										dismissPrevious();
										picker.dismiss();
									});
								pushAndShowModal(std::move(fPicker), e);
							}
						}), e);
				});
			pushAndShow(std::move(multiChoiceView), e);
			postDraw();
		}
	},
	screenshotPath
	{
		screenshotsMenuName(appContext(), app().userScreenshotPath), attach,
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Screenshots", app().screenshotDirectory(),
				[this](CStringView path)
				{
					log.info("set screenshots path:{}", path);
					app().userScreenshotPath = path;
					screenshotPath.compile(screenshotsMenuName(appContext(), path));
				}), e);
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
	item.emplace_back(&screenshotPath);
}

void FilePathOptionView::onSavePathChange(std::string_view path)
{
	if(path == optionSavePathDefaultToken)
	{
		app().postMessage(4, false, std::format("App Folder:\n{}", system().fallbackSaveDirectory()));
	}
	savePath.compile(savesMenuName(appContext(), path));
}

}
