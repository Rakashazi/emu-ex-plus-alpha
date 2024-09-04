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

#include <meta.h>
#include <imagine/config/version.h>
#include <main/MainApp.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/input/android/MogaManager.hh>

const char *const IG::ApplicationContext::applicationName{CONFIG_APP_NAME};
const char *const IG::ApplicationContext::applicationId{CONFIG_APP_ID};

namespace EmuEx
{

class EmuSystem;

std::u16string_view EmuApp::mainViewName()
{
	return u"" CONFIG_APP_NAME " " IMAGINE_VERSION;
}

bool EmuApp::hasGooglePlayStoreFeatures()
{
	#if defined __ANDROID__ && defined CONFIG_GOOGLE_PLAY_STORE
	return true;
	#else
	return false;
	#endif
}

EmuSystem &EmuApp::system() { return static_cast<MainApp*>(this)->system(); }

const EmuSystem &EmuApp::system() const { return static_cast<const MainApp*>(this)->system(); }

bool EmuApp::willCreateSystem(ViewAttachParams attach, const Input::Event &e)
{
	if(&MainApp::willCreateSystem != &EmuApp::willCreateSystem)
		return static_cast<MainApp*>(this)->willCreateSystem(attach, e);
	return true;
}

bool EmuApp::allowsTurboModifier(KeyCode c)
{
	if(&MainApp::allowsTurboModifier != &EmuApp::allowsTurboModifier)
		return MainApp::allowsTurboModifier(c);
	return true;
}

std::unique_ptr<View> EmuApp::makeEditCheatsView(ViewAttachParams attach, CheatsView& view)
{
	if(&MainApp::makeEditCheatsView != &EmuApp::makeEditCheatsView)
		return static_cast<MainApp*>(this)->makeEditCheatsView(attach, view);
	return {};
}

std::unique_ptr<View> EmuApp::makeEditCheatView(ViewAttachParams attach, Cheat& c, BaseEditCheatsView& baseView)
{
	if(&MainApp::makeEditCheatView != &EmuApp::makeEditCheatView)
		return static_cast<MainApp*>(this)->makeEditCheatView(attach, c, baseView);
	return {};
}

AssetDesc EmuApp::vControllerAssetDesc(KeyInfo key) const
{
	return static_cast<const MainApp*>(this)->vControllerAssetDesc(key);
}

std::span<const KeyCategory> EmuApp::keyCategories()
{
	return MainApp::keyCategories();
}

std::span<const KeyConfigDesc> EmuApp::defaultKeyConfigs()
{
	return MainApp::defaultKeyConfigs();
}

std::string_view EmuApp::systemKeyCodeToString(KeyCode c)
{
	return MainApp::systemKeyCodeToString(c);
}

}

namespace IG
{

void ApplicationContext::onInit(ApplicationInitParams initParams)
{
	auto &app = initApplication<EmuEx::MainApp>(initParams, *this);
	app.mainInitCommon(initParams, *this);
}

}
