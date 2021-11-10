#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>
#include <ctype.h>
#include "IniFileParser.h"


// PacketFileSystem.h Need to be included after all other includes
#include "PacketFileSystem.h"

FILE *openMachineIni(const char *name, const char *mode);

static char* iniBuffer;
static char* iniPtr;
static char* iniEnd;
static char* wrtBuffer;
static int   wrtBufferSize;
static int   wrtOffset;
static int   modified;
static char iniFilename[512];

static int readFile(const char* filename)
{
    int length;
    FILE* f;

    iniPtr = NULL;
    iniEnd = NULL;
    iniBuffer = NULL;

    f = openMachineIni(filename, "r");
    if (f == NULL) {
        return 0;
    }

    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (length > 0) {
        iniBuffer = malloc(length);
        length = fread(iniBuffer, 1, length, f);
        if (length > 0) {
            iniPtr = iniBuffer;
            iniEnd = iniBuffer + length;
        }
        else {
            free(iniBuffer);
            iniBuffer = NULL;
        }
    }
    fclose(f);

    return 1;
}

int readLine(char *line) 
{   
    int i = 0; 

    while (iniPtr != iniEnd) {
        char c = *iniPtr++;
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            *line = 0;
            return i;
        }
        *line++ = c; 
        i++;
    }

    return -1;
}

void createWriteBuffer()
{
    wrtBufferSize = 8192;
    wrtBuffer = malloc(wrtBufferSize);
    wrtOffset = 0;
}

void rewindBuffer()
{
    iniPtr = iniBuffer;
}

void destroyWriteBuffer()
{
    if (iniBuffer) {
        free(iniBuffer);
    }
    iniBuffer = wrtBuffer;
    iniPtr = iniBuffer;
    iniEnd = iniBuffer + wrtOffset;
}

void writeLine(const char* line)
{
    int length = strlen(line);
    if (length + wrtOffset > wrtBufferSize) {
        wrtBufferSize += 8192;
        wrtBuffer = realloc(wrtBuffer, wrtBufferSize);
    }

    memcpy(wrtBuffer + wrtOffset, line, length);
    wrtOffset += length;

    modified = 1;
}

void writeFile(const char* filename)
{
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        return;
    }
    fwrite(iniBuffer, 1, iniEnd - iniBuffer, f);
    fclose(f);
}

int iniFileOpen(const char *filename)
{
    modified = 0;
    strcpy(iniFilename, filename);
    return readFile(iniFilename);
}

int iniFileClose()
{
    if (iniBuffer == NULL) {
        return 0;
    }

    if (modified) {
        writeFile(iniFilename);
    }

    free(iniBuffer);
    iniBuffer = NULL;

    return 1;
}

int iniFileGetInt(char* section, 
                  char* entry, 
                  int   def) 
{   

    char buff[MAX_LINE_LENGTH]; 
    char *ep; 
    char t_section[MAX_LINE_LENGTH]; 
    char value[16]; 
    int len = strlen(entry); 
    int i; 
    
    rewindBuffer();

    sprintf(t_section, "[%s]", section);

    do {   
        if (readLine(buff) < 0) {
            return def; 
        } 
    } while(strcmp(buff, t_section)); 

    do {   
        if (readLine(buff) < 0 || buff[0] == '[') {
            return def; 
        } 
    } while(strncmp(buff, entry, len)); 

    ep = strrchr(buff, '=');
    if (ep == NULL) {
        return def;
    }
    ep++;
    if (!strlen(ep)) {
        return def; 
    }

    for (i = 0; isdigit(ep[i]); i++) {
        value[i] = ep[i]; 
    }

    value[i] = '\0'; 
    
    return atoi(value); 


} 


int iniFileGetString(char* section, 
                     char* entry, 
                     char* defVal, 
                     char* buffer, 
                     int   bufferLen) 
{   
    char def[MAX_LINE_LENGTH];
    char buff[MAX_LINE_LENGTH]; 
    char *ep; 
    char t_section[MAX_LINE_LENGTH]; 
    int len = strlen(entry); 

    rewindBuffer();

    strcpy(def, defVal);

    sprintf(t_section, "[%s]", section);
     
    do {   
        if (readLine(buff) < 0) {   
            strncpy(buffer, def, bufferLen); 
            buffer[bufferLen - 1] = '\0';
            return strlen(buffer); 
        } 
    } while (strcmp(buff, t_section)); 

    do {   
        if (readLine(buff) < 0 || buff[0] == '[') {   
            strncpy(buffer, def, bufferLen);   
            buffer[bufferLen - 1] = '\0';  
            return strlen(buffer); 
        } 
    } while (strncmp(buff, entry, len)); 

    ep = strrchr(buff, '=');
    ep++; 

    strncpy(buffer, ep, bufferLen); 
    buffer[bufferLen - 1] = '\0'; 

    return strlen(buffer); 
} 

int iniFileGetSection(char* section, 
                      char* buffer, 
                      int   bufferLen)
{
    char buff[MAX_LINE_LENGTH]; 
    char t_section[MAX_LINE_LENGTH]; 
    int offset = 0;
    int len;

    rewindBuffer();

    sprintf(t_section, "[%s]", section);

    do {   
        if (readLine(buff) < 0) {   
            buffer[offset++] = '\0';
            buffer[offset++] = '\0';
            return strlen(buffer); 
        } 
    } while (strcmp(buff, t_section)); 
    
    while ((len = readLine(buff)) >= 0 && buff[0] != '[') {
        if (offset + len + 2 < bufferLen) {
            strcpy(buffer + offset, buff);
            offset += len + 1;
        }
    }

    buffer[offset++] = '\0';
    buffer[offset++] = '\0';

    return 1;
}

int iniFileWriteString(char* section, 
                       char* entry, 
                       char* buffer) 
{
    char buff[MAX_LINE_LENGTH]; 
    char t_section[MAX_LINE_LENGTH]; 
    char t_entry[MAX_LINE_LENGTH]; 
    int len;

    rewindBuffer();

    createWriteBuffer();

    sprintf(t_section, "[%s]", section);
    sprintf(t_entry, "%s=", entry);
    len = strlen(t_entry);

    do {  
        if (readLine(buff) < 0) {  
            writeLine(t_section);
            writeLine("\n");
            writeLine(t_entry);
            writeLine(buffer);
            writeLine("\n");
            destroyWriteBuffer();
            return 1; 
        } 
        writeLine(buff);
        writeLine("\n");
    } while (strcmp(buff, t_section)); 

    for (;;) {   
        if (readLine(buff) < 0) { 
            writeLine(t_entry);
            writeLine(buffer);
            writeLine("\n");
            destroyWriteBuffer();
            return 1; 
        } 

        if (!strncmp(buff, t_entry, len) || buff[0] == '[') {
            break; 
        }
        writeLine(buff);
        writeLine("\n");
    } 

    writeLine(t_entry);
    writeLine(buffer);
    writeLine("\n");

    if (strncmp(buff, t_entry, len)) {
        writeLine(buff);
        writeLine("\n");
    }
    while (readLine(buff) >= 0) {
        writeLine(buff);
        writeLine("\n");
    }

    destroyWriteBuffer();

    return 1; 
} 


int iniFileWriteSection(char* section, 
                        char* buffer)
{
    char buff[MAX_LINE_LENGTH]; 
    char t_section[MAX_LINE_LENGTH]; 
    int len; 

    rewindBuffer();

    createWriteBuffer();

    sprintf(t_section, "[%s]", section);

    while (readLine(buff) >= 0 && strcmp(buff, t_section) != 0) {
        writeLine(buff);
        writeLine("\n");
    }
    
    writeLine(t_section);
    writeLine("\n");
    while (*buffer != '\0') {
        writeLine(buffer);
        writeLine("\n");
        buffer += strlen(buffer) + 1;
    }
    
    while ((len = readLine(buff)) >= 0 && buff[0] != '\0' && buff[0] != '[');

    while (len >= 0) {
        writeLine(buff);
        writeLine("\n");
        len = readLine(buff);
    }

    destroyWriteBuffer();

    return 1;
}
