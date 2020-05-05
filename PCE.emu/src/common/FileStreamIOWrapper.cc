#include "FileStreamIOWrapper.hh"
#include <mednafen/FileStream.h>
#include <mednafen/error.h>

static const uint8 utf8_bom[3] = { 0xEF, 0xBB, 0xBF };

FileStreamIOWrapper::FileStreamIOWrapper(const std::string& path, const int mode, const int do_lock)
{
	assert(mode == FileStream::MODE_READ);
	if(auto ec = open(path.c_str(), IO::AccessHint::SEQUENTIAL);
		ec)
	{
		throw MDFN_Error(0, "Error opening file \"%s\"", path.c_str());
	}
}

int FileStreamIOWrapper::get_line(std::string &str)
{
 uint8 c;

 str.clear();	// or str.resize(0)??

 while(read(&c, sizeof(c)) > 0)
 {
  if(c == '\r' || c == '\n' || c == 0)
   return(c);

  str.push_back(c);
 }

 return(str.length() ? 256 : -1);
}

bool FileStreamIOWrapper::read_utf8_bom(void)
{
	uint8 bom_tmp[sizeof(utf8_bom)];
	int read_count;

	if((read_count = read(bom_tmp, sizeof(bom_tmp))) == sizeof(bom_tmp) && !memcmp(bom_tmp, utf8_bom, sizeof(bom_tmp)))
	{
		//::printf("BOM!\n");
		return true;
	}
	else
	{
		seekC(-read_count);
		return false;
	}
}
