#pragma once

#include <imagine/fs/FSDefs.hh>

unsigned hasROMExtension(const char *name);
int loadArchive(void *buff, unsigned bytes, const char *path, FS::FileString &nameInArchive);
