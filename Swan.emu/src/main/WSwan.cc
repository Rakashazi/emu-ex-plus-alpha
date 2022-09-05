/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/fs/FS.hh>
#include <imagine/util/string.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include "MainSystem.hh"
#include <mednafen/hash/md5.h>
#include <mednafen/general.h>

#define EMU_MODULE "wswan"

namespace Mednafen
{

using namespace EmuEx;

MDFNGI *MDFNGameInfo;

uint64 MDFN_GetSettingUI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<EmuEx::WsSystem&>(EmuEx::gSystem());
	if(EMU_MODULE".bday" == name)
		return sys.userProfile.birthDay;
	if(EMU_MODULE".bmonth" == name)
		return sys.userProfile.birthMonth;
	if(EMU_MODULE".byear" == name)
		return sys.userProfile.birthYear;
	bug_unreachable("unhandled settingUI %s", name_);
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<EmuEx::WsSystem&>(EmuEx::gSystem());
	if("filesys.state_comp_level" == name)
		return 6;
	if(EMU_MODULE".sex" == name)
		return sys.userProfile.sex;
	if(EMU_MODULE".blood" == name)
		return sys.userProfile.bloodType;
	bug_unreachable("unhandled settingI %s", name_);
}

double MDFN_GetSettingF(const char *name)
{
	bug_unreachable("unhandled settingF %s", name);
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<EmuEx::WsSystem&>(EmuEx::gSystem());
	if("cheats" == name)
		return 0;
	if(EMU_MODULE".language" == name)
		return sys.userProfile.languageIsEnglish;
	if(EMU_MODULE".excomm" == name)
		return 0;
	if("filesys.untrusted_fip_check" == name)
		return 0;
	bug_unreachable("unhandled settingB %s", name_);
}

std::string MDFN_GetSettingS(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<EmuEx::WsSystem&>(EmuEx::gSystem());
	if(EMU_MODULE".name" == name)
		return std::string{sys.userName};
	if(EMU_MODULE".excomm.path" == name)
		return {};
	bug_unreachable("unhandled settingS %s", name_);
}

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	switch(type)
	{
		case MDFNMKF_STATE:
		case MDFNMKF_SAV:
		case MDFNMKF_SAVBACK:
		{
			assert(cd1);
			IG::FileString ext{'.'};
			ext += md5_context::asciistr(MDFNGameInfo->MD5, 0);
			ext += '.';
			ext += cd1;
			auto path = EmuEx::gSystem().contentSaveFilePath(ext);
			if(type == MDFNMKF_SAV) logMsg("save path:%s", path.c_str());
			return std::string{path};
		}
		default:
			bug_unreachable("type == %d", type);
	}
}

void MDFN_DoSimpleCommand(int cmd)
{
	MDFNGameInfo->DoSimpleCommand(cmd);
}

void Player_Init(int tsongs, const std::string &album, const std::string &artist, const std::string &copyright, const std::vector<std::string> &snames = std::vector<std::string>(), bool override_gi = true) {}

void Player_Draw(MDFN_Surface *surface, MDFN_Rect *dr, int CurrentSong, int16 *samples, int32 sampcount) {}

}
