#include "MDFN.hh"
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include <imagine/fs/sys.hh>
#include <imagine/util/strings.h>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

MDFNGI *MDFNGameInfo = &EmulatedPCE_Fast;
extern FsSys::PathString sysCardPath;
extern Byte1Option optionArcadeCard;

namespace PCE_Fast
{
// dummy HES functions
int PCE_HESLoad(const uint8 *buf, uint32 size) { return 0; };
void HES_Draw(MDFN_Surface *surface, MDFN_Rect *DisplayRect, int16 *SoundBuf, int32 SoundBufSize) { }
void HES_Close(void) { }
void HES_Reset(void) { }
uint8 ReadIBP(unsigned int A) { return 0; }
};

#define PCE_MODULE "pce_fast"

uint64 MDFN_GetSettingUI(const char *name)
{
	if(string_equal(PCE_MODULE".ocmultiplier", name))
		return 1;
	if(string_equal(PCE_MODULE".cdspeed", name))
		return 2;
	if(string_equal(PCE_MODULE".cdpsgvolume", name))
		return 100;
	if(string_equal(PCE_MODULE".cddavolume", name))
		return 100;
	if(string_equal(PCE_MODULE".adpcmvolume", name))
		return 100;
	if(string_equal(PCE_MODULE".slstart", name))
		return 12;
	if(string_equal(PCE_MODULE".slend", name))
		return 235;
	bug_exit("unhandled settingUI %s", name);
	return 0;
}

int64 MDFN_GetSettingI(const char *name)
{
	bug_exit("unhandled settingI %s", name);
	return 0;
}

float MDFN_GetSettingF(const char *name)
{
	if(string_equal(PCE_MODULE".mouse_sensitivity", name))
		return 0.50;
	bug_exit("unhandled settingF %s", name);
	return 0;
}

bool MDFN_GetSettingB(const char *name)
{
	if(string_equal("cheats", name))
		return 0;
	if(string_equal("filesys.disablesavegz", name))
		return 0;
	if(string_equal(PCE_MODULE".arcadecard", name))
		return optionArcadeCard;
	if(string_equal(PCE_MODULE".forcesgx", name))
		return 0;
	if(string_equal(PCE_MODULE".nospritelimit", name))
		return 0;
	if(string_equal(PCE_MODULE".forcemono", name))
		return 0;
	if(string_equal(PCE_MODULE".disable_softreset", name))
		return 0;
	if(string_equal(PCE_MODULE".adpcmlp", name))
		return 0;
	if(string_equal(PCE_MODULE".correct_aspect", name))
		return 1;
	if(string_equal("cdrom.lec_eval", name))
		return 1;
	if(string_equal("filesys.untrusted_fip_check", name))
		return 0;
	bug_exit("unhandled settingB %s", name);
	return 0;
}

std::string MDFN_GetSettingS(const char *name)
{
	if(string_equal(PCE_MODULE".cdbios", name))
	{
		return std::string("");
	}
	bug_exit("unhandled settingS %s", name);
	return 0;
}

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	switch(type)
	{
		case MDFNMKF_STATE:
		case MDFNMKF_SAV:
		{
			assert(cd1);
			std::string path(EmuSystem::savePath());
			path += PSS;
			path += EmuSystem::gameName();
			path += ".";
			path += md5_context::asciistr(MDFNGameInfo->MD5, 0);
			path += ".";
			path += cd1;
			if(type == MDFNMKF_SAV) logMsg("created save path %s", path.c_str());
			return path;
		}
		case MDFNMKF_FIRMWARE:
		{
			// pce-specific
			logMsg("getting firmware path %s", sysCardPath.data());
			return std::string(sysCardPath.data());
		}
		case MDFNMKF_AUX:
		{
			std::string path(FsSys::workDir());
			path += PSS;
			path += cd1;
			logMsg("created aux path %s", path.c_str());
			return path;
		}
		default:
			bug_branch("%d", type);
			return 0;
	}
}

void MDFN_DoSimpleCommand(int cmd)
{
	emuSys->DoSimpleCommand(cmd);
}
