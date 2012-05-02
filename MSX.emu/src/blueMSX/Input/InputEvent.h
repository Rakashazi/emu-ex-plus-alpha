/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Input/InputEvent.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2008/03/30 18:38:40 $
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
#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "MsxTypes.h"

void inputEventReset();

int inputEventStringToCode(const char* eventName);
const char* inputEventCodeToString(int eventCode);

#define EC_NONE      0

// ROW 0
#define EC_F1        1
#define EC_F2        2
#define EC_F3        3
#define EC_F4        4
#define EC_F5        5
#define EC_STOP      6
#define EC_CLS       7
#define EC_SELECT    8
#define EC_INS       9
#define EC_DEL      10

// ROW 1
#define EC_ESC      11
#define EC_1        12
#define EC_2        13
#define EC_3        14
#define EC_4        15
#define EC_5        16
#define EC_6        17
#define EC_7        18
#define EC_8        19
#define EC_9        20
#define EC_0        21
#define EC_NEG      22
#define EC_CIRCFLX  23
#define EC_BKSLASH  24
#define EC_BKSPACE  25

// ROW 2
#define EC_TAB      26
#define EC_Q        27
#define EC_W        28
#define EC_E        29
#define EC_R        30
#define EC_T        31
#define EC_Y        32
#define EC_U        33
#define EC_I        34
#define EC_O        35
#define EC_P        36
#define EC_AT       37
#define EC_LBRACK   38
#define EC_RETURN   39

// ROW 3
#define EC_CTRL     40
#define EC_A        41
#define EC_S        42
#define EC_D        43
#define EC_F        44
#define EC_G        45
#define EC_H        46
#define EC_J        47
#define EC_K        48
#define EC_L        49
#define EC_SEMICOL  50
#define EC_COLON    51
#define EC_RBRACK   52

// ROW 4
#define EC_LSHIFT   53
#define EC_Z        54
#define EC_X        55
#define EC_C        56
#define EC_V        57
#define EC_B        58
#define EC_N        59
#define EC_M        60
#define EC_COMMA    61
#define EC_PERIOD   62
#define EC_DIV      63
#define EC_UNDSCRE  64
#define EC_RSHIFT   65

// ROW 5
#define EC_CAPS     66
#define EC_GRAPH    67
#define EC_TORIKE   68
#define EC_SPACE    69
#define EC_JIKKOU   70
#define EC_CODE     71
#define EC_PAUSE    72

// ARROWS
#define EC_LEFT     73
#define EC_UP       74
#define EC_DOWN     75
#define EC_RIGHT    76

// NUMERIC KEYBOARD
#define EC_NUM7     77
#define EC_NUM8     78
#define EC_NUM9     79
#define EC_NUMDIV   80
#define EC_NUM4     81
#define EC_NUM5     82
#define EC_NUM6     83
#define EC_NUMMUL   84
#define EC_NUM1     85
#define EC_NUM2     86
#define EC_NUM3     87
#define EC_NUMSUB   88
#define EC_NUM0     89
#define EC_NUMPER   90
#define EC_NUMCOM   91
#define EC_NUMADD   92

// SVI SPECIFIC KEYS
#define EC_PRINT    93

#define EC_JOY1_UP      100
#define EC_JOY1_DOWN    101
#define EC_JOY1_LEFT    102
#define EC_JOY1_RIGHT   103
#define EC_JOY1_BUTTON1 104
#define EC_JOY1_BUTTON2 105
#define EC_JOY1_BUTTON3 106
#define EC_JOY1_BUTTON4 107
#define EC_JOY1_BUTTON5 108
#define EC_JOY1_BUTTON6 109
#define EC_JOY1_WHEELA  136
#define EC_JOY1_WHEELB  137

#define EC_JOY2_UP      110
#define EC_JOY2_DOWN    111
#define EC_JOY2_LEFT    112
#define EC_JOY2_RIGHT   113
#define EC_JOY2_BUTTON1 114
#define EC_JOY2_BUTTON2 115
#define EC_JOY2_BUTTON3 116
#define EC_JOY2_BUTTON4 117
#define EC_JOY2_BUTTON5 118
#define EC_JOY2_BUTTON6 119
#define EC_JOY2_WHEELA  138
#define EC_JOY2_WHEELB  139

#define EC_COLECO1_0    120
#define EC_COLECO1_1    121
#define EC_COLECO1_2    122
#define EC_COLECO1_3    123
#define EC_COLECO1_4    124
#define EC_COLECO1_5    125
#define EC_COLECO1_6    126
#define EC_COLECO1_7    127
#define EC_COLECO1_8    128
#define EC_COLECO1_9    129
#define EC_COLECO1_STAR 130
#define EC_COLECO1_HASH 131

#define EC_COLECO2_0    140
#define EC_COLECO2_1    141
#define EC_COLECO2_2    142
#define EC_COLECO2_3    143
#define EC_COLECO2_4    144
#define EC_COLECO2_5    145
#define EC_COLECO2_6    146
#define EC_COLECO2_7    147
#define EC_COLECO2_8    148
#define EC_COLECO2_9    149
#define EC_COLECO2_STAR 150
#define EC_COLECO2_HASH 151

#define EC_HOT_QUIT                     160
#define EC_HOT_TOGGLE_FDC_TIMING        161
#define EC_HOT_TOGGLE_SPRITE_ENABLE     162
#define EC_HOT_TOGGLE_MSX_AUDIO_SWITCH  163
#define EC_HOT_TOGGLE_FRONT_SWITCH      164
#define EC_HOT_TOGGLE_PAUSE_SWITCH      165
#define EC_HOT_TOGGLE_WAVE_CAPTURE      166
#define EC_HOT_SCREEN_CAPTURE           167
#define EC_HOT_QUICK_LOAD_STATE         168
#define EC_HOT_QUICK_SAVE_STATE         169
#define EC_HOT_CARD_REMOVE_1            170
#define EC_HOT_CARD_REMOVE_2            171
#define EC_HOT_TOGGLE_CARD_AUTO_RESET   172
#define EC_HOT_DISK_QUICK_CHANGE        173
#define EC_HOT_DISK_REMOVE_A            174
#define EC_HOT_DISK_REMOVE_B            175
#define EC_HOT_TOGGLE_DISK_AUTO_RESET   176
#define EC_HOT_CAS_REWIND               177
#define EC_HOT_CAS_REMOVE               178
#define EC_HOT_CAS_TOGGLE_READ_ONLY     179
#define EC_HOT_TOGGLE_CAS_AUTO_REWIND   180
#define EC_HOT_CAS_SAVE                 181
#define EC_HOT_EMU_TOGGLE_PAUSE         182
#define EC_HOT_EMU_STOP                 183
#define EC_HOT_EMU_SPEED_NORMAL         184
#define EC_HOT_EMU_SPEED_INCREASE       185
#define EC_HOT_EMU_SPEED_DECREASE       186
#define EC_HOT_MAX_SPEED_TOGGLE         187
#define EC_HOT_EMU_RESET_SOFT           188
#define EC_HOT_EMU_RESET_HARD           189
#define EC_HOT_EMU_RESET_CLEAN          190
#define EC_HOT_VOLUME_INCREASE          191
#define EC_HOT_VOLUME_DECREASE          192
#define EC_HOT_MUTE_TOGGLE_MASTER       193
#define EC_HOT_VOLUME_TOGGLE_STEREO     194
#define EC_HOT_WINDOW_SIZE_NORMAL       195
#define EC_HOT_WINDOW_SIZE_FULLSCREEN   196
#define EC_HOT_FULLSCREEN_TOGGLE        197

#define EC_KEYCOUNT                     198

// Inlines
extern int eventMap[256];

#define inputEventIsJoystick1(eventCode) \
        ((eventCode >= EC_JOY1_UP   && eventCode <= EC_JOY1_BUTTON6) || \
         (eventCode >= EC_COLECO1_0 && eventCode <= EC_COLECO1_HASH))

#define inputEventIsJoystick2(eventCode) \
        ((eventCode >= EC_JOY2_UP   && eventCode <= EC_JOY2_BUTTON6) || \
         (eventCode >= EC_COLECO2_0 && eventCode <= EC_COLECO2_HASH))

#define inputEventIsKeyboard(eventCode) \
        (!inputEventIsJoystick1(eventCode) && !inputEventIsJoystick2(eventCode))

#define inputEventSet(eventCode) eventMap[eventCode] = 1
#define inputEventUnset(eventCode) eventMap[eventCode] = 0
#define inputEventGetState(eventCode) eventMap[eventCode]

#endif
