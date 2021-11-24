#define LOGTAG "fileio"
#include "shared.h"
#include <imagine/io/FileIO.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>

bool hasROMExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".bin", ".smd", ".md", ".gen"
		#ifndef NO_SYSTEM_PBC
		, ".sms"
		#endif
		);
}

int loadArchive(void *buff, unsigned bytes, const char *path, FS::FileString &nameInArchive)
{
	if(EmuApp::hasArchiveExtension(path))
	{
		try
		{
			for(auto &entry : FS::ArchiveIterator{path})
			{
				if(entry.type() == FS::file_type::directory)
				{
					continue;
				}
				auto name = entry.name();
				logMsg("archive file entry:%s", name.data());
				if(hasROMExtension(name))
				{
					nameInArchive = name;
					auto io = entry.moveIO();
					return io.read(buff, bytes);
				}
			}
			logErr("no recognized file extensions in archive:%s", path);
			return -1;
		}
		catch(...)
		{
			logErr("error opening archive:%s", path);
			return -1;
		}
	}
	else
	{
		FileIO file{path, IO::AccessHint::ALL, IO::OPEN_TEST};
		if(!file)
		{
			return -1;
		}
		return file.read(buff, bytes);
	}
}
