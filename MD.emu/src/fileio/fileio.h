#pragma once

#include <imagine/io/IO.hh>

uint hasROMExtension(const char *name);
int loadArchive(void *buff, uint bytes, const char *path, FS::FileString &nameInArchive);
