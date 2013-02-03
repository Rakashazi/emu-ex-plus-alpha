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

CallResult Io::readLine(void* buffer, uint maxBytes)
{
	uint maxPrintableCharBytes = maxBytes - 1;
	uchar *charBuffer = (uchar*)buffer;
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
			seekF(1); // skip \n
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
