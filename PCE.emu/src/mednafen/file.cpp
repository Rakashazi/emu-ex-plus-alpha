/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mednafen.h"
#include <mednafen/FileStream.h>
#include <mednafen/compress/GZFileStream.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/IPSPatcher.h>

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <trio/trio.h>

//#include "compress/unzip.h"

#include "file.h"
#include "general.h"

static const uint64 MaxROMImageSize = (int64)1 << 26; // 2 ^ 26 = 64MiB

#if 0
static const char *unzErrorString(int error_code)
{
 if(error_code == UNZ_OK)
  return("ZIP OK");
 else if(error_code == UNZ_END_OF_LIST_OF_FILE)
  return("ZIP End of file list");
 else if(error_code == UNZ_EOF)
  return("ZIP EOF");
 else if(error_code == UNZ_PARAMERROR)
  return("ZIP Parameter error");
 else if(error_code == UNZ_BADZIPFILE)
  return("ZIP file bad");
 else if(error_code == UNZ_INTERNALERROR)
  return("ZIP Internal error");
 else if(error_code == UNZ_CRCERROR)
  return("ZIP CRC error");
 else if(error_code == UNZ_ERRNO)
  return(strerror(errno));
 else
  return("ZIP Unknown");
}

void MDFNFILE::ApplyIPS(Stream *ips)
{
 //
 // If the stream is not a MemoryStream, turn it into one.
 //
 //if(!(str->attributes() & Stream::ATTRIBUTE_WRITEABLE))
 if(dynamic_cast<MemoryStream*>(str.get()) == nullptr)
 {
  str.reset(new MemoryStream(str.release(), MaxROMImageSize));
 }  

 {
  MDFN_AutoIndent aind(1);
  uint32 count;

  count = IPSPatcher::Apply(ips, str.get());

  MDFN_printf(_("IPS EOF:  Did %u patches\n\n"), count);

  if(ips->tell() < ips->size())
  {
   MDFN_AutoIndent aindw(1);

   MDFN_printf(_("Warning:  trailing unused data in IPS file.\n"));
  }
 }
}

MDFNFILE::MDFNFILE(const char *path, const FileExtensionSpecStruct *known_ext, const char *purpose) : ext((const std::string&)f_ext), fbase((const std::string&)f_fbase)
{
 Open(path, known_ext, purpose);
}

MDFNFILE::~MDFNFILE()
{
 Close();
}


void MDFNFILE::Open(const char *path, const FileExtensionSpecStruct *known_ext, const char *purpose)
{
 unzFile tz = NULL;

 try
 {
  //
  // Try opening it as a zip file first
  //
  if((tz = unzOpen(path)))
  {
   char tempu[1024];
   int errcode;

   if((errcode = unzGoToFirstFile(tz)) != UNZ_OK)
   {
    throw MDFN_Error(0, _("Could not seek to first file in ZIP archive: %s"), unzErrorString(errcode));
   }

   if(known_ext)
   {
    bool FileFound = FALSE;
    while(!FileFound)
    {
     size_t tempu_strlen;
     const FileExtensionSpecStruct *ext_search = known_ext;

     if((errcode = unzGetCurrentFileInfo(tz, 0, tempu, 1024, 0, 0, 0, 0)) != UNZ_OK)
     {
      throw MDFN_Error(0, _("Could not get file information in ZIP archive: %s"), unzErrorString(errcode));
     }

     tempu[1023] = 0;
     tempu_strlen = strlen(tempu);

     while(ext_search->extension && !FileFound)
     {
      size_t ttmeow = strlen(ext_search->extension);
      if(tempu_strlen >= ttmeow)
      {
       if(!strcasecmp(tempu + tempu_strlen - ttmeow, ext_search->extension))
        FileFound = TRUE;
      }
      ext_search++;
     }

     if(FileFound)
      break;

     if((errcode = unzGoToNextFile(tz)) != UNZ_OK)
     { 
      if(errcode != UNZ_END_OF_LIST_OF_FILE)
      {
       throw MDFN_Error(0, _("Error seeking to next file in ZIP archive: %s"), unzErrorString(errcode));
      }

      if((errcode = unzGoToFirstFile(tz)) != UNZ_OK)
      {
       throw MDFN_Error(0, _("Could not seek to first file in ZIP archive: %s"), unzErrorString(errcode));
      }
      break;     
     }
    } // end to while(!FileFound)
   } // end to if(ext)

   if((errcode = unzOpenCurrentFile(tz)) != UNZ_OK)
   {
    throw MDFN_Error(0, _("Could not open file in ZIP archive: %s"), unzErrorString(errcode));
   }

   {
    unz_file_info ufo;
    unzGetCurrentFileInfo((unzFile)tz, &ufo, 0, 0, 0, 0, 0, 0);

    if(ufo.uncompressed_size > MaxROMImageSize)
     throw MDFN_Error(0, _("ROM image is too large; maximum size allowed is %llu bytes."), (unsigned long long)MaxROMImageSize);

    str.reset(new MemoryStream(ufo.uncompressed_size, true));

    unzReadCurrentFile((unzFile)tz, str->map(), str->size());
   }

   // Don't use MDFN_GetFilePathComponents() here.
   {
    char* ld = strrchr(tempu, '.');

    f_ext = std::string(ld ? ld : "");
    f_fbase = std::string(tempu, ld ? (ld - tempu) : strlen(tempu));
   }
  }
  else // If it's not a zip file, handle it as...another type of file!
  {
   std::unique_ptr<Stream> tfp(new FileStream(path, FileStream::MODE_READ));

   // We'll clean up f_ext to remove the leading period, and convert to lowercase, after
   // the plain vs gzip file handling code below(since gzip handling path will want to strip off an extra extension).
   MDFN_GetFilePathComponents(path, NULL, &f_fbase, &f_ext);

   uint8 gzmagic[3] = { 0 };

   if(tfp->read(gzmagic, 3, false) != 3 || gzmagic[0] != 0x1F || gzmagic[1] != 0x8b || gzmagic[2] != 0x08)
   {
    tfp->seek(0, SEEK_SET);

    if(tfp->size() > MaxROMImageSize)
     throw MDFN_Error(0, _("ROM image is too large; maximum size allowed is %llu bytes."), (unsigned long long)MaxROMImageSize);

    str = std::move(tfp);
   }
   else                  /* Probably gzip */
   {
    delete tfp.release();

    str.reset(new MemoryStream(new GZFileStream(path, GZFileStream::MODE::READ), MaxROMImageSize));

    MDFN_GetFilePathComponents(f_fbase, NULL, &f_fbase, &f_ext);
   } // End gzip handling
  } // End normal and gzip file handling else to zip

  // Remove leading period in file extension.
  if(f_ext.size() > 0 && f_ext[0] == '.')
   f_ext = f_ext.substr(1);

  // Convert file extension A-Z chars to lowercase, a-z
  for(auto& c : f_ext)
   if(c >= 'A' && c <= 'Z')
    c = 'a' + (c - 'A');

  //printf("|%s| --- |%s|\n", f_fbase.c_str(), f_ext.c_str());
 }
 catch(...)
 {
  if(tz != NULL)
  {
   unzCloseCurrentFile(tz);
   unzClose(tz);
  }

  Close();
  throw;
 }

 if(tz != NULL)
 {
  unzCloseCurrentFile(tz);
  unzClose(tz);
 }
}
#endif

void MDFNFILE::Close(void) throw()
{
 f_ext.clear();
 f_fbase.clear();

 if(str.get())
 {
  delete str.release();
 }
}

static INLINE void MDFN_DumpToFileReal(const std::string& path, const std::vector<PtrLengthPair> &pearpairs)
{
 FileStream fp(path, FileStream::MODE_WRITE_INPLACE);

 for(unsigned int i = 0; i < pearpairs.size(); i++)
 {
  fp.write(pearpairs[i].GetData(), pearpairs[i].GetLength());
 }

 fp.truncate(fp.tell());
 fp.close();
}

bool MDFN_DumpToFile(const std::string& path, const std::vector<PtrLengthPair> &pearpairs, bool throw_on_error)
{
 try
 {
  MDFN_DumpToFileReal(path, pearpairs);
 }
 catch(std::exception &e)
 {
  if(throw_on_error)
   throw;
  else
  {
   MDFN_PrintError("%s", e.what());
   return(false);
  }
 }
 return(true);
}

bool MDFN_DumpToFile(const std::string& path, const void *data, uint64 length, bool throw_on_error)
{
 std::vector<PtrLengthPair> tmp_pairs;

 tmp_pairs.push_back(PtrLengthPair(data, length));

 return MDFN_DumpToFile(path, tmp_pairs, throw_on_error);
}


std::unique_ptr<Stream> MDFN_AmbigGZOpenHelper(const std::string& path, std::vector<size_t> good_sizes)
{
 std::unique_ptr<Stream> fp(new FileStream(path, FileStream::MODE_READ));

 if(fp->size() >= 18)
 {
  uint8 head[10];

  fp->read(head, sizeof(head));

  if(head[0] == 0x1F && head[1] == 0x8B && head[2] == 0x08)
  {
   uint8 footer[8];
   uint32 fs;

   fp->seek(-8, SEEK_END);
   fp->read(footer, sizeof(footer));
   fs = MDFN_de32lsb(&footer[4]);

   for(auto const s : good_sizes)
   {
    if(s == fs)
    {
     fp.reset(nullptr);
     fp.reset(new GZFileStream(path, GZFileStream::MODE::READ));
     return fp;
    }
   }
  }
  fp->rewind();
 }

 return fp;
}

void MDFN_BackupSavFile(const uint8 max_backup_count, const char* sav_ext)
{
 FileStream cts(MDFN_MakeFName(MDFNMKF_SAVBACK, -1, sav_ext), FileStream::MODE_READ_WRITE, true);
 std::unique_ptr<MemoryStream> tmp;
 uint8 counter = max_backup_count - 1;

 cts.read(&counter, 1, false);
 //
 //
 try
 {
  tmp.reset(new MemoryStream(new FileStream(MDFN_MakeFName(MDFNMKF_SAV, 0, sav_ext), FileStream::MODE_READ)));
 }
 catch(MDFN_Error& e)
 {
  if(e.GetErrno() == ENOENT)
   return;

  throw;
 }
 //
 //
 //
 {
  try
  {
   MemoryStream oldbks(new GZFileStream(MDFN_MakeFName(MDFNMKF_SAVBACK, counter, sav_ext), GZFileStream::MODE::READ));

   if(oldbks.size() == tmp->size() && !memcmp(oldbks.map(), tmp->map(), oldbks.size()))
   {
    //puts("Skipped backup.");
    return;
   }
  }
  catch(MDFN_Error& e)
  {
   if(e.GetErrno() != ENOENT)
    throw;
  }
  //
  counter = (counter + 1) % max_backup_count;
  //
  GZFileStream bks(MDFN_MakeFName(MDFNMKF_SAVBACK, counter, sav_ext), GZFileStream::MODE::WRITE, 9);

  bks.write(tmp->map(), tmp->size());

  bks.close();
 }

 //
 //
 cts.rewind();
 cts.write(&counter, 1);
 cts.close();
}


//
//
//
void MDFN_mkdir_T(const char* path)
{
 //#ifdef WIN32
 //#else

 #ifdef HAVE_MKDIR
  #if MKDIR_TAKES_ONE_ARG
   ::mkdir(path);
  #else
   ::mkdir(path, S_IRWXU);
  #endif
 #elif HAVE__MKDIR
  ::_mkdir(path.c_str());
 #else
  #error "mkdir() missing?!"
 #endif
}

int MDFN_stat(const char* path, struct stat* buf)
{
 //#ifdef WIN32
 //#else
 return stat(path, buf);
}

int MDFN_unlink(const char* path)
{
 //#ifdef WIN32
 //#else
 return unlink(path);
}

int MDFN_rename(const char* oldpath, const char* newpath)
{
 return rename(oldpath, newpath);
}
