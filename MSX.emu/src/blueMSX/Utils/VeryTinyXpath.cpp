/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Utils/VeryTinyXpath.cpp,v $
**
** $Revision: 1.8 $
**
** $Date: 2009-07-18 17:16:36 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "VeryTinyXpath.h"
#include "tinyxml.h"
#ifdef WII
#include <string>
#else
#include <string.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#ifdef __APPLE__
#include <vector.h>
#else
#include <vector>
#endif

struct VtXpath {
    bool openForWrite;
    char filename[512];
    TiXmlDocument* doc;
};

typedef std::vector<std::string> PathVector;

static void createPathVector(PathVector& pathVector, va_list marker, int count)
{
    while (count--) {
        char* nodePtr = va_arg(marker, char*);
        std::string node(nodePtr);
        pathVector.push_back(node);
   }
}

extern "C" VtXpath* vtXpathOpenForRead(const char* filename)
{
    VtXpath* xpath = new VtXpath;
    strcpy(xpath->filename, filename);
    xpath->openForWrite = false;
    xpath->doc = new TiXmlDocument;

    xpath->doc->LoadFile(filename);
    
    if (xpath->doc->Error()) {
        vtXpathClose(xpath);
        return NULL;
    }

    return xpath;
}

extern "C" VtXpath* vtXpathOpenForWrite(const char* filename)
{
    VtXpath* xpath = new VtXpath;
    xpath->openForWrite = true;
    strcpy(xpath->filename, filename);
    xpath->doc = new TiXmlDocument;

    return xpath;
}

extern "C" void vtXpathClose(VtXpath* xpath)
{
    if (xpath == NULL) {
        return;
    }

    if (xpath->openForWrite) {
        xpath->doc->SaveFile(xpath->filename);
    }
    delete xpath->doc;
    delete xpath;
}

static const char* vtXpathGetStringFromPath(VtXpath* xpath, PathVector& pathVector)
{
    if (xpath == NULL) {
        return NULL;
    }

    TiXmlElement* el = xpath->doc->FirstChildElement(pathVector[0].c_str());
    if (el == NULL) {
        return NULL;
    }

    for (unsigned int i = 1; el != NULL && i < pathVector.size(); i++) {
        el = el->FirstChildElement(pathVector[i].c_str());
    }
    
    if (el == NULL) {
        return NULL;
    }

    TiXmlNode* node = el->FirstChild();
    if (node == NULL) {
        return NULL;
    }

    return node->Value();
}

static void vtXpathSetStringFromPath(VtXpath* xpath, PathVector& pathVector, const char* value)
{
    if (xpath == NULL) {
        return;
    }

    TiXmlElement* el = xpath->doc->FirstChildElement(pathVector[0].c_str());
    if (el == NULL) {
        std::string rootDoc;
        rootDoc += "<?xml version='1.0' encoding='UTF-8'?>";
        rootDoc += "<";
        rootDoc += pathVector[0];
        rootDoc += ">\n</";
        rootDoc += pathVector[0];
        rootDoc += ">\n";
        xpath->doc->Parse(rootDoc.c_str());
    }
    el = xpath->doc->FirstChildElement(pathVector[0].c_str());
    if (el == NULL) {
        return;
    }

    for (unsigned int i = 1; el != NULL && i < pathVector.size(); i++) {
        TiXmlElement* child = el->FirstChildElement(pathVector[i].c_str());
        if (child == NULL) {
            child = (TiXmlElement*)el->InsertEndChild(TiXmlElement(pathVector[i].c_str()));
        }
        el = child;
    }
    
    if (el != NULL) {
        el->InsertEndChild(TiXmlText(value));
    }
}

extern "C" int vtXpathGetInt(VtXpath* xpath, int numLevels, const char* first, ...)
{
    PathVector pathVector;
    va_list marker;
    va_start(marker, numLevels);
    createPathVector(pathVector, marker, numLevels);
    va_end(marker);

    const char* strValue = vtXpathGetStringFromPath(xpath, pathVector);
    if (strValue == NULL) {
        return VTXPATH_INT_NOT_FOUND;
    }

    int value = atoi(strValue);
    if (value == 0 && strValue[0] != '0') {
        return VTXPATH_INT_NOT_FOUND;
    }
    return value;
}

extern "C" const char* vtXpathGetString(VtXpath* xpath, int numLevels, const char* first, ...)
{
    PathVector pathVector;
    va_list marker;
    va_start(marker, numLevels);
    createPathVector(pathVector, marker, numLevels);
    va_end(marker);

    return vtXpathGetStringFromPath(xpath, pathVector);
}

extern "C" void vtXpathSetInt(VtXpath* xpath, int value, int numLevels, const char* first, ...)
{
    PathVector pathVector;
    va_list marker;
    va_start(marker, numLevels);
    createPathVector(pathVector, marker, numLevels);
    va_end(marker);

    char buf[16];
    sprintf(buf, "%d", value);

    vtXpathSetStringFromPath(xpath, pathVector, buf);
}

extern "C" void vtXpathSetString(VtXpath* xpath, const char* value, int numLevels, const char* first, ...)
{
    PathVector pathVector;
    va_list marker;
    va_start(marker, numLevels);
    createPathVector(pathVector, marker, numLevels);
    va_end(marker);

    vtXpathSetStringFromPath(xpath, pathVector, value);
}
