/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include "Io.hh"

int Io::fgetc()
{
	char byte;
	if(read(&byte, 1) == OK)
	{
		return byte;
	}
	else
		return EOF;
}

CallResult Io::read(void* buffer, size_t numBytes)
{
	if(readUpTo(buffer, numBytes) != (ssize_t)numBytes)
		return IO_ERROR;
	else
		return OK;
}

size_t Io::fread(void* ptr, size_t size, size_t nmemb)
{
	return readUpTo(ptr, (size * nmemb)) / size;
}

long Io::ftell()
{
	ulong pos = 0;
	return (tell(pos) == OK) ? (long)pos : -1;
}

int Io::fseek(long offset, int whence)
{
	// TODO: are we returning the correct error codes to simulate fseek?
	switch(whence)
	{
		case SEEK_SET: return (seekA(offset) == OK) ? 0 : -1;
		case SEEK_CUR: return (seekR(offset) == OK) ? 0 : -1;
		case SEEK_END: return (seekAE(offset) == OK) ? 0 : -1;
	}
	return -1;
}

CallResult Io::readLine(void* buffer, uint maxBytes)
{
	uint maxPrintableCharBytes = maxBytes - 1;
	auto charBuffer = (char*)buffer;
	for(uint i = 0; i < maxPrintableCharBytes; i++)
	{
		if(read(charBuffer, 1) != OK)
		{
			break;
		}

		if(*charBuffer == '\r') // Windows new line
		{
			*charBuffer = 0;
			charBuffer++;
			seekR(1); // skip \n
			break;
		}
		else if(*charBuffer == '\n')
		{
			*charBuffer = 0;
			charBuffer++;
			break;
		}
		else charBuffer++;
	}

	//did we read any chars?
	if(buffer == charBuffer)
		return(IO_ERROR);

	//null terminate string
	*(charBuffer) = 0;

	return (OK);
}

CallResult Io::writeToIO(Io &io)
{
	auto bytesToWrite = size();
	char buff[4096];
	while(bytesToWrite)
	{
		size_t bytes = std::min((ulong)sizeof(buff), bytesToWrite);
		CallResult ret = read(buff, bytes);
		if(ret != OK)
		{
			logErr("error reading from IO source with %d bytes to write", (int)bytesToWrite);
			return ret;
		}
		if(io.fwrite(buff, bytes, 1) != 1)
		{
			logErr("error writing to IO destination with %d bytes to write", (int)bytesToWrite);
			return IO_ERROR;
		}
		bytesToWrite -= bytes;
	}
	return OK;
}
