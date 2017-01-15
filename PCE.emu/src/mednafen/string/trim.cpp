/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* trim.cpp:
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

#include <mednafen/mednafen.h>
#include "trim.h"

static INLINE bool IsWS(const char c)
{
 return c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == 0x0b;
}

// Remove whitespace from beginning of s
void MDFN_ltrim(char* s)
{
 const char* si = s;
 char* di = s;
 bool InWhitespace = true;

 while(*si)
 {
  if(!InWhitespace || !IsWS(*si))
  {
   InWhitespace = false;
   *di = *si;
   di++;
  }
  si++;
 }

 *di = 0;
}

// Remove whitespace from beginning of s
void MDFN_ltrim(std::string& s)
{
 const size_t len = s.length();
 size_t di = 0, si = 0;
 bool InWhitespace = true;

 while(si < len)
 {
  if(!InWhitespace || !IsWS(s[si]))
  {
   InWhitespace = false;
   s[di] = s[si];
   di++;
  }
  si++;
 }

 s.resize(di);
}

// Remove whitespace from end of s
void MDFN_rtrim(char* s)
{
 const size_t len = strlen(s);

 if(!len)
  return;
 //
 size_t x = len;

 do
 {
  x--;

  if(!IsWS(s[x]))
   break;
 
  s[x] = 0;
 } while(x);
}

// Remove whitespace from end of s
void MDFN_rtrim(std::string& s)
{
 const size_t len = s.length();

 if(!len)
  return;
 //
 size_t x = len;
 size_t new_len = len;

 do
 {
  x--;

  if(!IsWS(s[x]))
   break;
 
  new_len--;
 } while(x);

 s.resize(new_len);
}

void MDFN_trim(char* s)
{
 MDFN_rtrim(s);
 MDFN_ltrim(s);
}

void MDFN_trim(std::string& s)
{
 MDFN_rtrim(s);
 MDFN_ltrim(s);
}


void MDFN_zapctrlchars(char* s)
{
 if(!s)
  return;

 while(*s)
 {
  if((unsigned char)*s < 0x20)
   *s = ' ';

  s++;
 }
}

void MDFN_zapctrlchars(std::string& s)
{
 for(auto& c : s)
  if((unsigned char)c < 0x20)
   c = ' ';
}

