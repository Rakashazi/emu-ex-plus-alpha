/***************************************************************************************
 *  Genesis Plus
 *  Sega Mouse support
 *
 *  Copyright Eke-Eke (2007-2011)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************************/

#include "shared.h"

static struct
{
  uint8 State;
  uint8 Counter;
  uint8 Wait;
  uint8 Port;
} mouse;

void mouse_reset(int port)
{
  input.analog[port][0] = 0;
  input.analog[port][1] = 0;
  mouse.State = 0x60;
  mouse.Counter = 0;
  mouse.Wait = 0;
  mouse.Port = port;
}

unsigned char mouse_read()
{
  unsigned int temp = 0x00;
  int x = input.analog[mouse.Port][0];
  int y = input.analog[mouse.Port][1];

  switch (mouse.Counter)
  {
    case 0: /* initial */
      temp = 0x00;
      break;

    case 1: /* xxxx1011 */
      temp = 0x0B;
      break;

    case 2: /* xxxx1111 */
      temp = 0x0F;
      break;

    case 3: /* xxxx1111 */
      temp = 0x0F;
      break;

    case 4: /* Axis sign & overflow (not emulated) bits */
      temp |= (x < 0);
      temp |= (y < 0) << 1;
      /*
      temp |= (abs(x) > 255) << 2;
      temp |= (abs(y) > 255) << 3;
      */
      break;

    case 5: /* START, A, B, C buttons state (active high) */
      temp = (input.pad[mouse.Port] >> 4) & 0x0F;
      break;

    case 6: /* X Axis MSB */
      temp = (x >> 4) & 0x0F;
      break;
      
    case 7: /* X Axis LSB */
      temp = (x & 0x0F);
      break;

    case 8: /* Y Axis MSB */
      temp = (y >> 4) & 0x0F;
      break;
      
    case 9: /* Y Axis LSB */
      temp = (y & 0x0F);
      break;
  }

  /* TL = busy status */
  if (mouse.Wait)
  {
    /* wait before ACK, fix some buggy mouse routine (Cannon Fodder, Shangai 2, Wack World,...) */
    mouse.Wait = 0;

    /* TL = !TR */
    temp |= (~mouse.State & 0x20) >> 1;
  }
  else
  {
    /* TL = TR (data is ready) */
    temp |= (mouse.State & 0x20) >> 1;
  }

  return temp;
}

void mouse_write(unsigned char data, unsigned char mask)
{
  /* update bits set as output only */
  data = (mouse.State & ~mask) | (data & mask);

  /* TH transition */
  if ((mouse.State ^ data) & 0x40)
  {
    /* start (TH=0) or stop (TH=1) acquisition */
    mouse.Counter = 1 - ((data & 0x40) >> 6);
  }

  /* TR transition */
  if ((mouse.State ^ data) & 0x20)
  {
    /* acquisition in progress */
    if ((mouse.Counter > 0) && (mouse.Counter < 10))
    {
      /* increment phase */
      mouse.Counter++;
    }

    /* TL handshake latency */
    mouse.Wait = 1;
  }

  /* update internal state */
  mouse.State = data;
}
