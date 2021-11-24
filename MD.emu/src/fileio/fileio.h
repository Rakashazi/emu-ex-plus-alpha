#pragma once

#include <imagine/fs/FSDefs.hh>

bool hasROMExtension(std::string_view name);
int loadArchive(void *buff, unsigned bytes, const char *path, FS::FileString &nameInArchive);
