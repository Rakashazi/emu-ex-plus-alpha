#pragma once

void chdirFromFilePath(const char *path);

// used on iOS to allow saves on incorrectly root-owned files/dirs
void fixFilePermissions(const char *path);
