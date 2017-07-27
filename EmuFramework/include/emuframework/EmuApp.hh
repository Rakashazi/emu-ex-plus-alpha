#pragma once

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

#include <memory>
#include <imagine/base/Base.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/io/IO.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/FileUtils.hh>

class EmuApp
{
public:
	using OnMainMenuOptionChanged = DelegateFunc<void()>;
	using CreateSystemCompleteDelegate = DelegateFunc<void (uint result, Input::Event e)>;

	static bool willCreateSystem(Input::Event e);
	static void createSystemWithMedia(GenericIO io, const char *path, const char *name,
		Input::Event e, CreateSystemCompleteDelegate onComplete);
	static void exitGame(bool allowAutosaveState = true);
	static void reloadGame();
	static void updateAndDrawEmuVideo(); // TODO: systems should not directly draw EmuVideo
	static void pushAndShowNewCollectTextInputView(ViewAttachParams attach, Input::Event e, const char *msgText,
		const char *initialContent, CollectTextInputView::OnTextDelegate onText);
	static void pushAndShowModalView(View &v, Input::Event e);
	static void popModalViews();
	static void popMenuToRoot();
	static void restoreMenuFromGame();
	static void loadGameCompleteFromFilePicker(Gfx::Renderer &r, uint result, Input::Event e);
	static bool hasArchiveExtension(const char *name);
	static void setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func);
	static void refreshCheatViews();
	[[gnu::format(printf, 3, 4)]]
	static void printfMessage(uint secs, bool error, const char *format, ...);
	static void postMessage(const char *msg);
	static void postMessage(bool error, const char *msg);
	static void postMessage(uint secs, bool error, const char *msg);
	static void unpostMessage();
};
