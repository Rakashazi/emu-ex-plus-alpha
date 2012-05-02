/**************************************************************************­**** 
 PORTABLE ROUTINES FOR WRITING PRIVATE PROFILE STRINGS --  by Joseph J. Graf 
 Header file containing prototypes and compile-time configuration. 
***************************************************************************­***/ 

#ifndef INIFILE_PARSER_H
#define INIFILE_PARSER_H

#define MAX_LINE_LENGTH    512 


int iniFileOpen(const char* filename);
int iniFileClose();

int iniFileGetInt(char* section, 
                  char* entry, 
                  int   def);
int iniFileGetString(char* section, 
                     char* entry, 
                     char* defVal, 
                     char* buffer, 
                     int   bufferLen);
int iniFileGetSection(char* section, 
                     char* buffer, 
                     int   bufferLen);
int iniFileWriteString(char* section, 
                       char* entry, 
                       char* buffer);
int iniFileWriteSection(char* section, 
                        char* buffer);


#endif
