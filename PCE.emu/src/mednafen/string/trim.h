/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* trim.h:
**  Copyright (C) 2007-2016 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MDFN_STRING_TRIM_H
#define __MDFN_STRING_TRIM_H

// Removes whitespace from the beginning of the string.
void MDFN_ltrim(char* s);
void MDFN_ltrim(std::string& s);

// Removes whitespace from the end of the string.
void MDFN_rtrim(char* s);
void MDFN_rtrim(std::string& s);

// Removes whitespace from the beginning and end of the string.
void MDFN_trim(char* s);
void MDFN_trim(std::string& s);

// Replaces control characters with space(' ') character.
void MDFN_zapctrlchars(char* s);
void MDFN_zapctrlchars(std::string& s);

#endif
