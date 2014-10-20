#define LOGTAG "fileio"
#include "shared.h"
#include <unzip.h>
#include <imagine/io/FileIO.hh>

uint isROMExtension(const char *name);

/*
    Load a normal file, or ZIP/GZ archive.
    Returns NULL if an error occured.
*/
uint8 *load_archive(char *filename, int *file_size)
{
    int size = 0;
    uint8 *buf = NULL;

    if(check_zip(filename))
    {
        unzFile fd = NULL;
        unz_file_info info;
        int ret = 0;

        /* Attempt to open the archive */
        fd = unzOpen(filename);
        if(!fd) return (NULL);

        /* Go to first file in archive */
        ret = unzGoToFirstFile(fd);
        if(ret != UNZ_OK)
        {
            unzClose(fd);
            return (NULL);
        }

        // Find a valid file
        bool foundRom = 0;
        do
        {
        	ret = unzGetCurrentFileInfo(fd, &info, filename, 128, NULL, 0, NULL, 0);
			if(ret != UNZ_OK)
			{
				unzClose(fd);
				return (NULL);
			}

			if(isROMExtension(filename))
			{
				foundRom = 1;
				break;
			}
        }
        while(unzGoToNextFile(fd) == UNZ_OK);

        if(!foundRom)
        {
        	unzClose(fd);
        	return (NULL);
        }

        /* Open the file for reading */
        ret = unzOpenCurrentFile(fd);
        if(ret != UNZ_OK)
        {
            unzClose(fd);
            return (NULL);
        }

        /* Allocate file data buffer */
        size = info.uncompressed_size;
        buf = (uint8*)malloc(size);
        if(!buf)
        {
            unzClose(fd);
            return (NULL);
        }

        /* Read (decompress) the file */
        ret = unzReadCurrentFile(fd, buf, info.uncompressed_size);
        if(ret != (int)info.uncompressed_size)
        {
            free(buf);
            unzCloseCurrentFile(fd);
            unzClose(fd);
            return (NULL);
        }

        /* Close the current file */
        ret = unzCloseCurrentFile(fd);
        if(ret != UNZ_OK)
        {
            free(buf);
            unzClose(fd);
            return (NULL);
        }

        /* Close the archive */
        ret = unzClose(fd);
        if(ret != UNZ_OK)
        {
            free(buf);
            return (NULL);
        }

        /* Update file size and return pointer to file data */
        *file_size = size;
        return (buf);
    }
    else
    {
        FileIO file;

        /* Open file */
        file.open(filename);
        if(!file) return (0);

        /* Get file size */
        size = file.size();

        /* Allocate file data buffer */
        buf = (uint8*)malloc(size);
        if(!buf)
        {
            return (0);
        }

        /* Read file data */
        file.read(buf, size);

        /* Update file size and return pointer to file data */
        *file_size = size;
        return (buf);
    }
}


/*
    Verifies if a file is a ZIP archive or not.
    Returns: 1= ZIP archive, 0= not a ZIP archive
*/
int check_zip(char *filename)
{
    uint8 buf[2];
    FILE *fd = NULL;
    fd = fopen(filename, "rb");
    if(!fd) return (0);
    fread(buf, 2, 1, fd);
    fclose(fd);
    if(memcmp(buf, "PK", 2) == 0) return (1);
    return (0);
}


/*
    Returns the size of a GZ compressed file.
*/
int gzsize(gzFile gd)
{
    #define CHUNKSIZE   (0x10000)
    int size = 0, length = 0;
    unsigned char buffer[CHUNKSIZE];
    gzrewind(gd);
    do {
        size = gzread(gd, buffer, CHUNKSIZE);
        if(size <= 0) break;
        length += size;
    } while (!gzeof(gd));
    gzrewind(gd);
    return (length);
    #undef CHUNKSIZE
}
