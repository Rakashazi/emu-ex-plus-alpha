#pragma once

#include <imagine/io/FileIO.hh>

class FileStreamIOWrapper : public FileIO
{
public:
	using FileIO::FileIO;

	FileStreamIOWrapper(const std::string& path, const int mode, const int do_lock = false);
	int get_line(std::string &str);
	bool read_utf8_bom();
	void mswin_utf8_convert_kludge() {}
};
