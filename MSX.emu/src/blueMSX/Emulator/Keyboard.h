/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Keyboard.h,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-03-30 18:38:40 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2004 Daniel Vik
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************
*/
#error "Deprecated"

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "MsxTypes.h"

typedef enum { KEYMAP_MSX, KEYMAP_SVI, KEYMAP_COLECO, KEYMAP_SG1000 } KeyboardKeymap;

// Emulator Keys (Based on MSX Turbo-R keyboard layout)

#define EK_NONE      0

// ROW 0
#define EK_F1        1
#define EK_F2        2
#define EK_F3        3
#define EK_F4        4
#define EK_F5        5
#define EK_STOP      6
#define EK_CLS       7
#define EK_SELECT    8
#define EK_INS       9
#define EK_DEL      10

// ROW 1
#define EK_ESC      11
#define EK_1        12
#define EK_2        13
#define EK_3        14
#define EK_4        15
#define EK_5        16
#define EK_6        17
#define EK_7        18
#define EK_8        19
#define EK_9        20
#define EK_0        21
#define EK_NEG      22
#define EK_CIRCFLX  23
#define EK_BKSLASH  24
#define EK_BKSPACE  25

// ROW 2
#define EK_TAB      26
#define EK_Q        27
#define EK_W        28
#define EK_E        29
#define EK_R        30
#define EK_T        31
#define EK_Y        32
#define EK_U        33
#define EK_I        34
#define EK_O        35
#define EK_P        36
#define EK_AT       37
#define EK_LBRACK   38
#define EK_RETURN   39

// ROW 3
#define EK_CTRL     40
#define EK_A        41
#define EK_S        42
#define EK_D        43
#define EK_F        44
#define EK_G        45
#define EK_H        46
#define EK_J        47
#define EK_K        48
#define EK_L        49
#define EK_SEMICOL  50
#define EK_COLON    51
#define EK_RBRACK   52

// ROW 4
#define EK_LSHIFT   53
#define EK_Z        54
#define EK_X        55
#define EK_C        56
#define EK_V        57
#define EK_B        58
#define EK_N        59
#define EK_M        60
#define EK_COMMA    61
#define EK_PERIOD   62
#define EK_DIV      63
#define EK_UNDSCRE  64
#define EK_RSHIFT   65

// ROW 5
#define EK_CAPS     66
#define EK_GRAPH    67
#define EK_TORIKE   68
#define EK_SPACE    69
#define EK_JIKKOU   70
#define EK_CODE     71
#define EK_PAUSE    72

// ARROWS
#define EK_LEFT     73
#define EK_UP       74
#define EK_DOWN     75
#define EK_RIGHT    76

// NUMERIC KEYBOARD
#define EK_NUM7     77
#define EK_NUM8     78
#define EK_NUM9     79
#define EK_NUMDIV   80
#define EK_NUM4     81
#define EK_NUM5     82
#define EK_NUM6     83
#define EK_NUMMUL   84
#define EK_NUM1     85
#define EK_NUM2     86
#define EK_NUM3     87
#define EK_NUMSUB   88
#define EK_NUM0     89
#define EK_NUMPER   90
#define EK_NUMCOM   91
#define EK_NUMADD   92

// SVI SPECIFIC KEYS
#define EK_PRINT    93

#define EK_KEYCOUNT 94   // Remove

#define EK_JOY1_UP      100
#define EK_JOY1_DOWN    101
#define EK_JOY1_LEFT    102
#define EK_JOY1_RIGHT   103
#define EK_JOY1_BUTTON1 104
#define EK_JOY1_BUTTON2 105
#define EK_JOY1_BUTTON3 106
#define EK_JOY1_BUTTON4 107
#define EK_JOY1_BUTTON5 108
#define EK_JOY1_BUTTON6 109

#define EK_JOY2_UP      110
#define EK_JOY2_DOWN    111
#define EK_JOY2_LEFT    112
#define EK_JOY2_RIGHT   113
#define EK_JOY2_BUTTON1 114
#define EK_JOY2_BUTTON2 115
#define EK_JOY2_BUTTON3 116
#define EK_JOY2_BUTTON4 117
#define EK_JOY2_BUTTON5 118
#define EK_JOY2_BUTTON6 119


void keyboardReset();

void keyboardLoadState();
void keyboardSaveState();

void keyboardKeyDown(int keyCode);
void keyboardKeyUp(int keyCode);

int keyboardGetKeyState(int keyCode);

void keyboardSetKeymap(KeyboardKeymap keymap);
UInt8* keyboardGetState();

int keyboardStringToKeyCode(const char* keyName);
const char* keyboardKeyCodeToString(int keyCode);

#endif

