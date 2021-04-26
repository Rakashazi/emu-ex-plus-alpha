#define LOGTAG "fileio"
#include "shared.h"
#include <imagine/io/FileIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>

unsigned hasROMExtension(const char *name)
{
	return string_hasDotExtension(name, "bin") || string_hasDotExtension(name, "smd") ||
		string_hasDotExtension(name, "md") || string_hasDotExtension(name, "gen")
		#ifndef NO_SYSTEM_PBC
		|| string_hasDotExtension(name, "sms")
		#endif
		;
}

int loadArchive(void *buff, unsigned bytes, const char *path, FS::FileString &nameInArchive)
{
	if(EmuApp::hasArchiveExtension(path))
	{
		std::error_code ec{};
		for(auto &entry : FS::ArchiveIterator{path, ec})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			logMsg("archive file entry:%s", name);
			if(hasROMExtension(name))
			{
				string_copy(nameInArchive, name);
				auto io = entry.moveIO();
				return io.read(buff, bytes);
			}
		}
		if(ec)
		{
			logErr("error opening archive:%s", path);
			return -1;
		}
		logErr("no recognized file extensions in archive:%s", path);
		return -1;
	}
	else
	{
		FileIO file;
		file.open(path, IO::AccessHint::ALL);
		if(!file)
		{
			return -1;
		}
		return file.read(buff, bytes);
	}
}
