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

#include <imagine/config/defs.hh>
#include <imagine/util/string.h>
#include <imagine/util/utility.h>
#include <imagine/util/utf.hh>
#include <cstdint>
#include <cstring>

namespace IG
{

std::errc convertCharCode(const char** sourceStart, uint32_t &c)
{
	if(Config::UNICODE_CHARS)
	{
		switch(UTF::ConvertUTF8toUTF32((const uint8_t**)sourceStart, UTF::strictConversion, c))
		{
			case UTF::conversionOK: return {};
			case UTF::reachedNullChar: return std::errc::result_out_of_range;
			default: return std::errc::invalid_argument;
		}
	}
	else
	{
		c = **sourceStart;
		if(c == '\0')
			return std::errc::result_out_of_range;
		*sourceStart += 1;
		return {};
	}
}

std::u16string makeUTF16String(std::string_view strView)
{
	if(!strView.size())
		return {};
	std::u16string u16String{};
	unsigned utf16Len = 0;
	const char *s = strView.data();
	uint32_t c = 0;
	while(!(bool)convertCharCode(&s, c))
	{
		if(c > UINT16_MAX)
			continue;
		utf16Len++;
	}
	u16String.reserve(utf16Len);
	s = strView.data();
	while(!(bool)convertCharCode(&s, c))
	{
		if(c > UINT16_MAX)
			continue;
		u16String.push_back(c);
	}
	return u16String;
}

static constexpr unsigned char hexdigToInt(char hexdig)
{
	switch (hexdig) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return (unsigned char)(9 + hexdig - '9');

	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		return (unsigned char)(15 + hexdig - 'f');

	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return (unsigned char)(15 + hexdig - 'F');

	default:
		return 0;
	}
}

// URI decoder based on uriparser library

char *decodeUri(IG::CStringView input, char *write)
{
	const char *read = input;
	bool prevWasCr = false;

	for (;;) {
		switch (read[0]) {
		case '\0':
			if (read > write) {
				write[0] = '\0';
			}
			return write;

		case '%':
			switch (read[1]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				switch (read[2]) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					{
						/* Percent group found */
						const unsigned char left = hexdigToInt(read[1]);
						const unsigned char right = hexdigToInt(read[2]);
						const int code = 16 * left + right;
						switch (code) {
						case 10:
							write[0] = (char)10;
							write++;
							prevWasCr = false;
							break;

						case 13:
							write[0] = (char)13;
							write++;
							prevWasCr = true;
							break;

						default:
							write[0] = (char)(code);
							write++;

							prevWasCr = false;

						}
						read += 3;
					}
					break;

				default:
					/* Copy two chars unmodified and */
					/* look at this char again */
					if (read > write) {
						write[0] = read[0];
						write[1] = read[1];
					}
					read += 2;
					write += 2;

					prevWasCr = false;
				}
				break;

			default:
				/* Copy one char unmodified and */
				/* look at this char again */
				if (read > write) {
					write[0] = read[0];
				}
				read++;
				write++;

				prevWasCr = false;
			}
			break;

		default:
			/* Copy one char unmodified */
			if (read > write) {
				write[0] = read[0];
			}
			read++;
			write++;

			prevWasCr = false;
		}
	}
}

}
