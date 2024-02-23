/** \file   vhkkeysyms.h
 * \brief   UI-agnostic key symbols and names - header
 *
 * List of keysym identifiers used by the hotkeys in VICE.
 * Each UI toolkit has its own key symbols and names, so we need a way to refer
 * to keys that isn't toolkit-specific.
 *
 * The keysyms provided here are taken from `/usr/include/X11/keysymdef.h`.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 */

#ifndef VICE_HOTKEYS_VHKKEYSYMS_H
#define VICE_HOTKEYS_VHKKEYSYMS_H

#include <stdint.h>
#include "hotkeystypes.h"

#define VHK_KEY_BackSpace               0xff08  /* Back space, back char */
#define VHK_KEY_Tab                     0xff09
#define VHK_KEY_Linefeed                0xff0a  /* Linefeed, LF */
#define VHK_KEY_Clear                   0xff0b
#define VHK_KEY_Return                  0xff0d  /* Return, enter */
#define VHK_KEY_Pause                   0xff13  /* Pause, hold */
#define VHK_KEY_Scroll_Lock             0xff14
#define VHK_KEY_Sys_Req                 0xff15
#define VHK_KEY_Escape                  0xff1b
#define VHK_KEY_Delete                  0xffff  /* Delete, rubout */

#define VHK_KEY_Home                    0xff50
#define VHK_KEY_Left                    0xff51  /* Move left, left arrow */
#define VHK_KEY_Up                      0xff52  /* Move up, up arrow */
#define VHK_KEY_Right                   0xff53  /* Move right, right arrow */
#define VHK_KEY_Down                    0xff54  /* Move down, down arrow */
#define VHK_KEY_Prior                   0xff55  /* Prior, previous */
#define VHK_KEY_Page_Up                 0xff55
#define VHK_KEY_Next                    0xff56  /* Next */
#define VHK_KEY_Page_Down               0xff56
#define VHK_KEY_End                     0xff57  /* EOL */
#define VHK_KEY_Begin                   0xff58  /* BOL */

/* Misc functions */

#define VHK_KEY_Select                  0xff60  /* Select, mark */
#define VHK_KEY_Print                   0xff61
#define VHK_KEY_Execute                 0xff62  /* Execute, run, do */
#define VHK_KEY_Insert                  0xff63  /* Insert, insert here */
#define VHK_KEY_Undo                    0xff65
#define VHK_KEY_Redo                    0xff66  /* Redo, again */
#define VHK_KEY_Menu                    0xff67
#define VHK_KEY_Find                    0xff68  /* Find, search */
#define VHK_KEY_Cancel                  0xff69  /* Cancel, stop, abort, exit */
#define VHK_KEY_Help                    0xff6a  /* Help */
#define VHK_KEY_Break                   0xff6b
#define VHK_KEY_Mode_switch             0xff7e  /* Character set switch */
#define VHK_KEY_script_switch           0xff7e  /* Alias for mode_switch */

/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */

#define VHK_KEY_KP_Space                0xff80  /* Space */
#define VHK_KEY_KP_Tab                  0xff89
#define VHK_KEY_KP_Enter                0xff8d  /* Enter */
#define VHK_KEY_KP_F1                   0xff91  /* PF1, KP_A, ... */
#define VHK_KEY_KP_F2                   0xff92
#define VHK_KEY_KP_F3                   0xff93
#define VHK_KEY_KP_F4                   0xff94
#define VHK_KEY_KP_Home                 0xff95
#define VHK_KEY_KP_Left                 0xff96
#define VHK_KEY_KP_Up                   0xff97
#define VHK_KEY_KP_Right                0xff98
#define VHK_KEY_KP_Down                 0xff99
#define VHK_KEY_KP_Prior                0xff9a
#define VHK_KEY_KP_Page_Up              0xff9a
#define VHK_KEY_KP_Next                 0xff9b
#define VHK_KEY_KP_Page_Down            0xff9b
#define VHK_KEY_KP_End                  0xff9c
#define VHK_KEY_KP_Begin                0xff9d
#define VHK_KEY_KP_Insert               0xff9e
#define VHK_KEY_KP_Delete               0xff9f
#define VHK_KEY_KP_Equal                0xffbd  /* Equals */
#define VHK_KEY_KP_Multiply             0xffaa
#define VHK_KEY_KP_Add                  0xffab
#define VHK_KEY_KP_Separator            0xffac  /* Separator, often comma */
#define VHK_KEY_KP_Subtract             0xffad
#define VHK_KEY_KP_Decimal              0xffae
#define VHK_KEY_KP_Divide               0xffaf

#define VHK_KEY_KP_0                    0xffb0
#define VHK_KEY_KP_1                    0xffb1
#define VHK_KEY_KP_2                    0xffb2
#define VHK_KEY_KP_3                    0xffb3
#define VHK_KEY_KP_4                    0xffb4
#define VHK_KEY_KP_5                    0xffb5
#define VHK_KEY_KP_6                    0xffb6
#define VHK_KEY_KP_7                    0xffb7
#define VHK_KEY_KP_8                    0xffb8
#define VHK_KEY_KP_9                    0xffb9

/* Function keys */

#define VHK_KEY_F1                      0xffbe
#define VHK_KEY_F2                      0xffbf
#define VHK_KEY_F3                      0xffc0
#define VHK_KEY_F4                      0xffc1
#define VHK_KEY_F5                      0xffc2
#define VHK_KEY_F6                      0xffc3
#define VHK_KEY_F7                      0xffc4
#define VHK_KEY_F8                      0xffc5
#define VHK_KEY_F9                      0xffc6
#define VHK_KEY_F10                     0xffc7
#define VHK_KEY_F11                     0xffc8
#define VHK_KEY_F12                     0xffc9
#define VHK_KEY_F13                     0xffca
#define VHK_KEY_F14                     0xffcb
#define VHK_KEY_F15                     0xffcc
#define VHK_KEY_F16                     0xffcd
#define VHK_KEY_F17                     0xffce
#define VHK_KEY_F18                     0xffcf
#define VHK_KEY_F19                     0xffd0
#define VHK_KEY_F20                     0xffd1
#define VHK_KEY_F21                     0xffd2
#define VHK_KEY_F22                     0xffd3
#define VHK_KEY_F23                     0xffd4
#define VHK_KEY_F24                     0xffd5
#define VHK_KEY_F25                     0xffd6
#define VHK_KEY_F26                     0xffd7
#define VHK_KEY_F27                     0xffd8
#define VHK_KEY_F28                     0xffd9
#define VHK_KEY_F29                     0xffda
#define VHK_KEY_F30                     0xffdb
#define VHK_KEY_F31                     0xffdc
#define VHK_KEY_F32                     0xffdd
#define VHK_KEY_F33                     0xffde
#define VHK_KEY_F34                     0xffdf
#define VHK_KEY_F35                     0xffe0

#define VHK_KEY_space                   0x0020  /* U+0020 SPACE */
#define VHK_KEY_exclam                  0x0021  /* U+0021 EXCLAMATION MARK */
#define VHK_KEY_quotedbl                0x0022  /* U+0022 QUOTATION MARK */
#define VHK_KEY_numbersign              0x0023  /* U+0023 NUMBER SIGN */
#define VHK_KEY_dollar                  0x0024  /* U+0024 DOLLAR SIGN */
#define VHK_KEY_percent                 0x0025  /* U+0025 PERCENT SIGN */
#define VHK_KEY_ampersand               0x0026  /* U+0026 AMPERSAND */
#define VHK_KEY_apostrophe              0x0027  /* U+0027 APOSTROPHE */
#define VHK_KEY_quoteright              0x0027  /* deprecated */
#define VHK_KEY_parenleft               0x0028  /* U+0028 LEFT PARENTHESIS */
#define VHK_KEY_parenright              0x0029  /* U+0029 RIGHT PARENTHESIS */
#define VHK_KEY_asterisk                0x002a  /* U+002A ASTERISK */
#define VHK_KEY_plus                    0x002b  /* U+002B PLUS SIGN */
#define VHK_KEY_comma                   0x002c  /* U+002C COMMA */
#define VHK_KEY_minus                   0x002d  /* U+002D HYPHEN-MINUS */
#define VHK_KEY_period                  0x002e  /* U+002E FULL STOP */
#define VHK_KEY_slash                   0x002f  /* U+002F SOLIDUS */
#define VHK_KEY_0                       0x0030  /* U+0030 DIGIT ZERO */
#define VHK_KEY_1                       0x0031  /* U+0031 DIGIT ONE */
#define VHK_KEY_2                       0x0032  /* U+0032 DIGIT TWO */
#define VHK_KEY_3                       0x0033  /* U+0033 DIGIT THREE */
#define VHK_KEY_4                       0x0034  /* U+0034 DIGIT FOUR */
#define VHK_KEY_5                       0x0035  /* U+0035 DIGIT FIVE */
#define VHK_KEY_6                       0x0036  /* U+0036 DIGIT SIX */
#define VHK_KEY_7                       0x0037  /* U+0037 DIGIT SEVEN */
#define VHK_KEY_8                       0x0038  /* U+0038 DIGIT EIGHT */
#define VHK_KEY_9                       0x0039  /* U+0039 DIGIT NINE */
#define VHK_KEY_colon                   0x003a  /* U+003A COLON */
#define VHK_KEY_semicolon               0x003b  /* U+003B SEMICOLON */
#define VHK_KEY_less                    0x003c  /* U+003C LESS-THAN SIGN */
#define VHK_KEY_equal                   0x003d  /* U+003D EQUALS SIGN */
#define VHK_KEY_greater                 0x003e  /* U+003E GREATER-THAN SIGN */
#define VHK_KEY_question                0x003f  /* U+003F QUESTION MARK */
#define VHK_KEY_at                      0x0040  /* U+0040 COMMERCIAL AT */
/* Upper case letters, conveniently mapping to ASCII */
#define VHK_KEY_A                       0x0041
#define VHK_KEY_B                       0x0042
#define VHK_KEY_C                       0x0043
#define VHK_KEY_D                       0x0044
#define VHK_KEY_E                       0x0045
#define VHK_KEY_F                       0x0046
#define VHK_KEY_G                       0x0047
#define VHK_KEY_H                       0x0048
#define VHK_KEY_I                       0x0049
#define VHK_KEY_J                       0x004a
#define VHK_KEY_K                       0x004b
#define VHK_KEY_L                       0x004c
#define VHK_KEY_M                       0x004d
#define VHK_KEY_N                       0x004e
#define VHK_KEY_O                       0x004f
#define VHK_KEY_P                       0x0050
#define VHK_KEY_Q                       0x0051
#define VHK_KEY_R                       0x0052
#define VHK_KEY_S                       0x0053
#define VHK_KEY_T                       0x0054
#define VHK_KEY_U                       0x0055
#define VHK_KEY_V                       0x0056
#define VHK_KEY_W                       0x0057
#define VHK_KEY_X                       0x0058
#define VHK_KEY_Y                       0x0059
#define VHK_KEY_Z                       0x005a

#define VHK_KEY_bracketleft             0x005b  /* U+005B LEFT SQUARE BRACKET */
#define VHK_KEY_backslash               0x005c  /* U+005C REVERSE SOLIDUS */
#define VHK_KEY_bracketright            0x005d  /* U+005D RIGHT SQUARE BRACKET */
#define VHK_KEY_asciicircum             0x005e  /* U+005E CIRCUMFLEX ACCENT */
#define VHK_KEY_underscore              0x005f  /* U+005F LOW LINE */
#define VHK_KEY_grave                   0x0060  /* U+0060 GRAVE ACCENT */
#define VHK_KEY_quoteleft               0x0060  /* deprecated */
/* Lower case letters, conveniently mapping to ASCII */
#define VHK_KEY_a                       0x0061
#define VHK_KEY_b                       0x0062
#define VHK_KEY_c                       0x0063
#define VHK_KEY_d                       0x0064
#define VHK_KEY_e                       0x0065
#define VHK_KEY_f                       0x0066
#define VHK_KEY_g                       0x0067
#define VHK_KEY_h                       0x0068
#define VHK_KEY_i                       0x0069
#define VHK_KEY_j                       0x006a
#define VHK_KEY_k                       0x006b
#define VHK_KEY_l                       0x006c
#define VHK_KEY_m                       0x006d
#define VHK_KEY_n                       0x006e
#define VHK_KEY_o                       0x006f
#define VHK_KEY_p                       0x0070
#define VHK_KEY_q                       0x0071
#define VHK_KEY_r                       0x0072
#define VHK_KEY_s                       0x0073
#define VHK_KEY_t                       0x0074
#define VHK_KEY_u                       0x0075
#define VHK_KEY_v                       0x0076
#define VHK_KEY_w                       0x0077
#define VHK_KEY_x                       0x0078
#define VHK_KEY_y                       0x0079
#define VHK_KEY_z                       0x007a

#define VHK_KEY_braceleft               0x007b  /* U+007B LEFT CURLY BRACKET */
#define VHK_KEY_bar                     0x007c  /* U+007C VERTICAL LINE */
#define VHK_KEY_braceright              0x007d  /* U+007D RIGHT CURLY BRACKET */
#define VHK_KEY_asciitilde              0x007e  /* U+007E TILDE */

/* Special, "international", keys */
#define VHK_KEY_nobreakspace           0x00a0  /* U+00A0 NO-BREAK SPACE */
#define VHK_KEY_exclamdown             0x00a1  /* U+00A1 INVERTED EXCLAMATION MARK */
#define VHK_KEY_cent                   0x00a2  /* U+00A2 CENT SIGN */
#define VHK_KEY_sterling               0x00a3  /* U+00A3 POUND SIGN */
#define VHK_KEY_currency               0x00a4  /* U+00A4 CURRENCY SIGN */
#define VHK_KEY_yen                    0x00a5  /* U+00A5 YEN SIGN */
#define VHK_KEY_brokenbar              0x00a6  /* U+00A6 BROKEN BAR */
#define VHK_KEY_section                0x00a7  /* U+00A7 SECTION SIGN */
#define VHK_KEY_diaeresis              0x00a8  /* U+00A8 DIAERESIS */
#define VHK_KEY_copyright              0x00a9  /* U+00A9 COPYRIGHT SIGN */
#define VHK_KEY_ordfeminine            0x00aa  /* U+00AA FEMININE ORDINAL INDICATOR */
#define VHK_KEY_guillemotleft          0x00ab  /* U+00AB LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
#define VHK_KEY_notsign                0x00ac  /* U+00AC NOT SIGN */
#define VHK_KEY_hyphen                 0x00ad  /* U+00AD SOFT HYPHEN */
#define VHK_KEY_registered             0x00ae  /* U+00AE REGISTERED SIGN */
#define VHK_KEY_macron                 0x00af  /* U+00AF MACRON */
#define VHK_KEY_degree                 0x00b0  /* U+00B0 DEGREE SIGN */
#define VHK_KEY_plusminus              0x00b1  /* U+00B1 PLUS-MINUS SIGN */
#define VHK_KEY_twosuperior            0x00b2  /* U+00B2 SUPERSCRIPT TWO */
#define VHK_KEY_threesuperior          0x00b3  /* U+00B3 SUPERSCRIPT THREE */
#define VHK_KEY_acute                  0x00b4  /* U+00B4 ACUTE ACCENT */
#define VHK_KEY_mu                     0x00b5  /* U+00B5 MICRO SIGN */
#define VHK_KEY_paragraph              0x00b6  /* U+00B6 PILCROW SIGN */
#define VHK_KEY_periodcentered         0x00b7  /* U+00B7 MIDDLE DOT */
#define VHK_KEY_cedilla                0x00b8  /* U+00B8 CEDILLA */
#define VHK_KEY_onesuperior            0x00b9  /* U+00B9 SUPERSCRIPT ONE */
#define VHK_KEY_masculine              0x00ba  /* U+00BA MASCULINE ORDINAL INDICATOR */
#define VHK_KEY_guillemotright         0x00bb  /* U+00BB RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
#define VHK_KEY_onequarter             0x00bc  /* U+00BC VULGAR FRACTION ONE QUARTER */
#define VHK_KEY_onehalf                0x00bd  /* U+00BD VULGAR FRACTION ONE HALF */
#define VHK_KEY_threequarters          0x00be  /* U+00BE VULGAR FRACTION THREE QUARTERS */
#define VHK_KEY_questiondown           0x00bf  /* U+00BF INVERTED QUESTION MARK */
#define VHK_KEY_Agrave                 0x00c0  /* U+00C0 LATIN CAPITAL LETTER A WITH GRAVE */
#define VHK_KEY_Aacute                 0x00c1  /* U+00C1 LATIN CAPITAL LETTER A WITH ACUTE */
#define VHK_KEY_Acircumflex            0x00c2  /* U+00C2 LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
#define VHK_KEY_Atilde                 0x00c3  /* U+00C3 LATIN CAPITAL LETTER A WITH TILDE */
#define VHK_KEY_Adiaeresis             0x00c4  /* U+00C4 LATIN CAPITAL LETTER A WITH DIAERESIS */
#define VHK_KEY_Aring                  0x00c5  /* U+00C5 LATIN CAPITAL LETTER A WITH RING ABOVE */
#define VHK_KEY_AE                     0x00c6  /* U+00C6 LATIN CAPITAL LETTER AE */
#define VHK_KEY_Ccedilla               0x00c7  /* U+00C7 LATIN CAPITAL LETTER C WITH CEDILLA */
#define VHK_KEY_Egrave                 0x00c8  /* U+00C8 LATIN CAPITAL LETTER E WITH GRAVE */
#define VHK_KEY_Eacute                 0x00c9  /* U+00C9 LATIN CAPITAL LETTER E WITH ACUTE */
#define VHK_KEY_Ecircumflex            0x00ca  /* U+00CA LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
#define VHK_KEY_Ediaeresis             0x00cb  /* U+00CB LATIN CAPITAL LETTER E WITH DIAERESIS */
#define VHK_KEY_Igrave                 0x00cc  /* U+00CC LATIN CAPITAL LETTER I WITH GRAVE */
#define VHK_KEY_Iacute                 0x00cd  /* U+00CD LATIN CAPITAL LETTER I WITH ACUTE */
#define VHK_KEY_Icircumflex            0x00ce  /* U+00CE LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
#define VHK_KEY_Idiaeresis             0x00cf  /* U+00CF LATIN CAPITAL LETTER I WITH DIAERESIS */
#define VHK_KEY_ETH                    0x00d0  /* U+00D0 LATIN CAPITAL LETTER ETH */
#define VHK_KEY_Eth                    0x00d0  /* deprecated */
#define VHK_KEY_Ntilde                 0x00d1  /* U+00D1 LATIN CAPITAL LETTER N WITH TILDE */
#define VHK_KEY_Ograve                 0x00d2  /* U+00D2 LATIN CAPITAL LETTER O WITH GRAVE */
#define VHK_KEY_Oacute                 0x00d3  /* U+00D3 LATIN CAPITAL LETTER O WITH ACUTE */
#define VHK_KEY_Ocircumflex            0x00d4  /* U+00D4 LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
#define VHK_KEY_Otilde                 0x00d5  /* U+00D5 LATIN CAPITAL LETTER O WITH TILDE */
#define VHK_KEY_Odiaeresis             0x00d6  /* U+00D6 LATIN CAPITAL LETTER O WITH DIAERESIS */
#define VHK_KEY_multiply               0x00d7  /* U+00D7 MULTIPLICATION SIGN */
#define VHK_KEY_Oslash                 0x00d8  /* U+00D8 LATIN CAPITAL LETTER O WITH STROKE */
#define VHK_KEY_Ooblique               0x00d8  /* U+00D8 LATIN CAPITAL LETTER O WITH STROKE */
#define VHK_KEY_Ugrave                 0x00d9  /* U+00D9 LATIN CAPITAL LETTER U WITH GRAVE */
#define VHK_KEY_Uacute                 0x00da  /* U+00DA LATIN CAPITAL LETTER U WITH ACUTE */
#define VHK_KEY_Ucircumflex            0x00db  /* U+00DB LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
#define VHK_KEY_Udiaeresis             0x00dc  /* U+00DC LATIN CAPITAL LETTER U WITH DIAERESIS */
#define VHK_KEY_Yacute                 0x00dd  /* U+00DD LATIN CAPITAL LETTER Y WITH ACUTE */
#define VHK_KEY_THORN                  0x00de  /* U+00DE LATIN CAPITAL LETTER THORN */
#define VHK_KEY_Thorn                  0x00de  /* deprecated */
#define VHK_KEY_ssharp                 0x00df  /* U+00DF LATIN SMALL LETTER SHARP S */
#define VHK_KEY_agrave                 0x00e0  /* U+00E0 LATIN SMALL LETTER A WITH GRAVE */
#define VHK_KEY_aacute                 0x00e1  /* U+00E1 LATIN SMALL LETTER A WITH ACUTE */
#define VHK_KEY_acircumflex            0x00e2  /* U+00E2 LATIN SMALL LETTER A WITH CIRCUMFLEX */
#define VHK_KEY_atilde                 0x00e3  /* U+00E3 LATIN SMALL LETTER A WITH TILDE */
#define VHK_KEY_adiaeresis             0x00e4  /* U+00E4 LATIN SMALL LETTER A WITH DIAERESIS */
#define VHK_KEY_aring                  0x00e5  /* U+00E5 LATIN SMALL LETTER A WITH RING ABOVE */
#define VHK_KEY_ae                     0x00e6  /* U+00E6 LATIN SMALL LETTER AE */
#define VHK_KEY_ccedilla               0x00e7  /* U+00E7 LATIN SMALL LETTER C WITH CEDILLA */
#define VHK_KEY_egrave                 0x00e8  /* U+00E8 LATIN SMALL LETTER E WITH GRAVE */
#define VHK_KEY_eacute                 0x00e9  /* U+00E9 LATIN SMALL LETTER E WITH ACUTE */
#define VHK_KEY_ecircumflex            0x00ea  /* U+00EA LATIN SMALL LETTER E WITH CIRCUMFLEX */
#define VHK_KEY_ediaeresis             0x00eb  /* U+00EB LATIN SMALL LETTER E WITH DIAERESIS */
#define VHK_KEY_igrave                 0x00ec  /* U+00EC LATIN SMALL LETTER I WITH GRAVE */
#define VHK_KEY_iacute                 0x00ed  /* U+00ED LATIN SMALL LETTER I WITH ACUTE */
#define VHK_KEY_icircumflex            0x00ee  /* U+00EE LATIN SMALL LETTER I WITH CIRCUMFLEX */
#define VHK_KEY_idiaeresis             0x00ef  /* U+00EF LATIN SMALL LETTER I WITH DIAERESIS */
#define VHK_KEY_eth                    0x00f0  /* U+00F0 LATIN SMALL LETTER ETH */
#define VHK_KEY_ntilde                 0x00f1  /* U+00F1 LATIN SMALL LETTER N WITH TILDE */
#define VHK_KEY_ograve                 0x00f2  /* U+00F2 LATIN SMALL LETTER O WITH GRAVE */
#define VHK_KEY_oacute                 0x00f3  /* U+00F3 LATIN SMALL LETTER O WITH ACUTE */
#define VHK_KEY_ocircumflex            0x00f4  /* U+00F4 LATIN SMALL LETTER O WITH CIRCUMFLEX */
#define VHK_KEY_otilde                 0x00f5  /* U+00F5 LATIN SMALL LETTER O WITH TILDE */
#define VHK_KEY_odiaeresis             0x00f6  /* U+00F6 LATIN SMALL LETTER O WITH DIAERESIS */
#define VHK_KEY_division               0x00f7  /* U+00F7 DIVISION SIGN */
#define VHK_KEY_oslash                 0x00f8  /* U+00F8 LATIN SMALL LETTER O WITH STROKE */
#define VHK_KEY_ooblique               0x00f8  /* U+00F8 LATIN SMALL LETTER O WITH STROKE */
#define VHK_KEY_ugrave                 0x00f9  /* U+00F9 LATIN SMALL LETTER U WITH GRAVE */
#define VHK_KEY_uacute                 0x00fa  /* U+00FA LATIN SMALL LETTER U WITH ACUTE */
#define VHK_KEY_ucircumflex            0x00fb  /* U+00FB LATIN SMALL LETTER U WITH CIRCUMFLEX */
#define VHK_KEY_udiaeresis             0x00fc  /* U+00FC LATIN SMALL LETTER U WITH DIAERESIS */
#define VHK_KEY_yacute                 0x00fd  /* U+00FD LATIN SMALL LETTER Y WITH ACUTE */
#define VHK_KEY_thorn                  0x00fe  /* U+00FE LATIN SMALL LETTER THORN */
#define VHK_KEY_ydiaeresis             0x00ff  /* U+00FF LATIN SMALL LETTER Y WITH DIAERESIS */

/* Key modifier masks */
#define VHK_MOD_NONE    0x0000
#define VHK_MOD_ALT     0x0001
#define VHK_MOD_COMMAND 0x0002
#define VHK_MOD_CONTROL 0x0004
#define VHK_MOD_HYPER   0x0008
#define VHK_MOD_META    0x0010
#define VHK_MOD_OPTION  0x0020
#define VHK_MOD_SHIFT   0x0040
#define VHK_MOD_SUPER   0x0080

uint32_t    vhk_keysym_from_name  (const char *name);
const char *vhk_keysym_name       (uint32_t vice_keysym);
uint32_t    vhk_modifier_from_name(const char *name, const char **endptr);
const char *vhk_modifier_name     (uint32_t vice_modifier);
char       *vhk_modmask_name      (uint32_t vice_modmask);
char       *vhk_hotkey_label      (uint32_t vice_keysym, uint32_t vice_modmask);

#endif
