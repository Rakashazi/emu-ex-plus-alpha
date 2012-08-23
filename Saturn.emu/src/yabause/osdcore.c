/*  Copyright 2012 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "osdcore.h"
#include "vdp1.h"

#include <stdlib.h>
#include <stdarg.h>

#ifndef YAB_PORT_OSD
/*
Heya fellow port developper :)
If you're reading this, that may be because you want to
use your own OSD core in your port and you're about to
do it here...
Don't.
Please define the CPP constant YAB_PORT_OSD and define
your own list of OSD cores in your port.
This definition was added here to avoid breaking "everything"
when we the new OSD system was added.
*/
OSD_struct *OSDCoreList[] = {
&OSDDummy,
#ifdef HAVE_LIBGLUT
&OSDGlut,
#endif
NULL
};
#else
extern OSD_struct * OSDCoreList[];
#endif

static OSD_struct * OSD = NULL;
static OSDMessage_struct osdmessages[OSDMSG_COUNT];

int OSDInit(int coreid)
{
   int i;

   for (i = 0; OSDCoreList[i] != NULL; i++)
   {
      if (OSDCoreList[i]->id == coreid)
      {
         OSD = OSDCoreList[i];
         break;
      }
   }

   if (OSD == NULL)
      return -1;

   if (OSD->Init() != 0)
      return -1;

   memset(osdmessages, 0, sizeof(osdmessages));
   osdmessages[OSDMSG_FPS].hidden = 1;

   return 0;
}

void OSDDeInit() {
   if (OSD)
      OSD->DeInit();
   OSD = NULL;
}

int OSDChangeCore(int coreid)
{
   OSDDeInit();
   OSDInit(coreid);
}

void OSDPushMessage(int msgtype, int ttl, const char * format, ...)
{
   va_list arglist;
   char message[1024];

   va_start(arglist, format);
   vsprintf(message, format, arglist);
   va_end(arglist);

   osdmessages[msgtype].type = msgtype;
   osdmessages[msgtype].message = strdup(message);
   osdmessages[msgtype].timetolive = ttl;
   osdmessages[msgtype].timeleft = ttl;
}

void OSDDisplayMessages(void)
{
   int i = 0;

   if (OSD == NULL) return;

   for(i = 0;i < OSDMSG_COUNT;i++)
      if ((osdmessages[i].hidden == 0) && (osdmessages[i].timeleft > 0))
      {
         OSD->DisplayMessage(osdmessages + i);
         osdmessages[i].timeleft--;
         if (osdmessages[i].timeleft == 0) free(osdmessages[i].message);
      }
}

void OSDToggle(int what)
{
   if ((what < 0) || (what >= OSDMSG_COUNT)) return;

   osdmessages[what].hidden = 1 - osdmessages[what].hidden;
}

int OSDIsVisible(int what)
{
   if ((what < 0) || (what >= OSDMSG_COUNT)) return -1;

   return 1 - osdmessages[what].hidden;
}

void OSDSetVisible(int what, int visible)
{
   if ((what < 0) || (what >= OSDMSG_COUNT)) return;

   visible = visible == 0 ? 0 : 1;
   osdmessages[what].hidden = 1 - visible;
}

void ToggleFPS()
{
   OSDToggle(OSDMSG_FPS);
}

int GetOSDToggle(void)
{
   return OSDIsVisible(OSDMSG_FPS);
}

void SetOSDToggle(int toggle)
{
   OSDSetVisible(OSDMSG_FPS, toggle);
}

void DisplayMessage(const char* str)
{
   OSDPushMessage(OSDMSG_STATUS, 120, str);
}

static int OSDDummyInit(void);
static void OSDDummyDeInit(void);
static void OSDDummyReset(void);
static void OSDDummyDisplayMessage(OSDMessage_struct * message);

OSD_struct OSDDummy = {
    OSDCORE_DUMMY,
    "Dummy OSD Interface",
    OSDDummyInit,
    OSDDummyDeInit,
    OSDDummyReset,
    OSDDummyDisplayMessage
};

int OSDDummyInit(void)
{
   return 0;
}

void OSDDummyDeInit(void)
{
}

void OSDDummyReset(void)
{
}

void OSDDummyDisplayMessage(OSDMessage_struct * message)
{
}

#ifdef HAVE_LIBGLUT
#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

static int OSDGlutInit(void);
static void OSDGlutDeInit(void);
static void OSDGlutReset(void);
static void OSDGlutDisplayMessage(OSDMessage_struct * message);

OSD_struct OSDGlut = {
    OSDCORE_GLUT,
    "Glut OSD Interface",
    OSDGlutInit,
    OSDGlutDeInit,
    OSDGlutReset,
    OSDGlutDisplayMessage
};

int OSDGlutInit(void)
{
#ifndef WIN32
   int fake_argc = 1;
   char * fake_argv[] = { "yabause" };
   static int glutinited = 0;

   if (!glutinited)
   {
      glutInit(&fake_argc, fake_argv);
      glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
      glutinited = 1;
   }
#endif
   return 0;
}

void OSDGlutDeInit(void)
{
}

void OSDGlutReset(void)
{
}

void OSDGlutDisplayMessage(OSDMessage_struct * message)
{
   int LeftX=9;
   int Width=500;
   int TxtY=11;
   int Height=13;
   int i, msglength;
   int vidwidth, vidheight;

   VIDCore->GetGlSize(&vidwidth, &vidheight);
   Width = vidwidth - 2 * LeftX;

   switch(message->type) {
      case OSDMSG_STATUS:
         TxtY = vidheight - (Height + TxtY);
         break;
   }

   msglength = strlen(message->message);

   glBegin(GL_POLYGON);
      glColor3f(0, 0, 0);
      glVertex2i(LeftX, TxtY);
      glVertex2i(LeftX + Width, TxtY);
      glVertex2i(LeftX + Width, TxtY + Height);
      glVertex2i(LeftX, TxtY + Height);
   glEnd();

   glColor3f(1.0f, 1.0f, 1.0f);
   glRasterPos2i(10, TxtY + 11);
   for (i = 0; i < msglength; i++) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, message->message[i]);
   }
   glColor3f(1, 1, 1);
}
#endif
