#pragma once

#include <fs/Fs.hh>

static bool isHuCardExtension(const char *name)
{
	return string_hasDotExtension(name, "pce") || string_hasDotExtension(name, "sgx") || string_hasDotExtension(name, "zip");
}

static bool isCDExtension(const char *name)
{
	return string_hasDotExtension(name, "toc") || string_hasDotExtension(name, "cue");
}

static int pceHuCDFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isHuCardExtension(name) || isCDExtension(name);
}

static int pceHuFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isHuCardExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = pceHuCDFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = pceHuFsFilter;
