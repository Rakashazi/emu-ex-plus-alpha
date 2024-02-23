/** \file   uiactions.h
 * \brief   UI actions interface - header
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

#ifndef VICE_UIACTIONS_H
#define VICE_UIACTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* this header is required if the macro IS_ACTION_NAME_CHAR() is used: */
#include <ctype.h>

/* for the VICE_MACHINE_* masks */
#include "machine.h"


/** \brief  Mapping of action IDs to names and descriptions
 */
typedef struct ui_action_info_s {
    int         id;     /**< action ID */
    const char *name;   /**< action name */
    const char *desc;   /**< action description */
} ui_action_info_t;


/** \brief  Mapping of an action ID to a handler
 */
typedef struct ui_action_map_s {
    int    action;          /**< action ID */

    /*
     * Action handler data
     */

    void (*handler)(struct ui_action_map_s*); /**< function handling the action */
    void  *data;            /**< optional user data */


    /* modes */
    bool   blocks;          /**< action blocks (the same action cannot be
                                 triggered again until it finishes) */
    bool   dialog;          /**< action pops up a dialog (only one dialog action
                                 is allowed at a time), this implies using the
                                 UI thread */
    bool   uithread;        /**< must run on the UI thread */

    /* state */
    bool   is_busy;         /**< action is busy */

    /*
     * Hotkey data
     */
    uint32_t  vice_keysym;  /**< VICE keysym, see hotkeys.vhkkeysyms.h */
    uint32_t  vice_modmask; /**< VICE modmask, see hotkeys.vhkkeysyms.h */
    uint32_t  arch_keysym;  /**< arch keysym */
    uint32_t  arch_modmask; /**< arch modmask */
    void     *menu_item[2]; /**< menu item references */
    void     *user_data;    /**< additional data (optional) */
} ui_action_map_t;

/** \brief  Terminator of action maps
 */
#define UI_ACTION_MAP_TERMINATOR { .action = ACTION_NONE, .handler = NULL }


/** \brief  Check for valid action name character
 *
 * Check if \a ch is a valid character in an action name.
 *
 * Supported characters are:
 * * a-z
 * * A-Z
 * * 0-9
 * * '_', '-' and ':'
 */
#define IS_ACTION_NAME_CHAR(ch) \
    (isalpha((unsigned char)(ch)) || isdigit((unsigned char)(ch)) || \
     ch == '_' || ch == '-' || ch == ':')


/** \brief  IDs for the UI actions
 *
 * These IDs are used to refer to specific UI actions/dialogs.
 */
enum {
    ACTION_INVALID = -1,
    ACTION_NONE = 0,
    ACTION_ADVANCE_FRAME,
    ACTION_CART_ATTACH,         /* Gtk3 has one dialog for CRT and RAW */
    ACTION_CART_ATTACH_RAW,     /* SDL has separate dialogs for CRT and RAW */
    ACTION_CART_ATTACH_RAW_1000,        /* CBM-II */
    ACTION_CART_ATTACH_RAW_2000,        /* CBM-II, VIC-20 */
    ACTION_CART_ATTACH_RAW_4000,        /* CBM-II, VIC_20 */
    ACTION_CART_ATTACH_RAW_6000,        /* CBM-II, VIC-20 */
    ACTION_CART_ATTACH_RAW_A000,        /* VIC-20 */
    ACTION_CART_ATTACH_RAW_B000,        /* VIC-20 */
    ACTION_CART_ATTACH_RAW_BEHRBONZ,    /* VIC-20 */
    ACTION_CART_ATTACH_RAW_FINAL,       /* VIC-20 */
    ACTION_CART_ATTACH_RAW_MEGACART,    /* VIC-20 */
    ACTION_CART_ATTACH_RAW_ULTIMEM,     /* VIC-20 */
    ACTION_CART_ATTACH_RAW_VICFP,       /* VIC-20 */
    ACTION_CART_ATTACH_RAW_JACINT1MB,   /* Plus/4 */
    ACTION_CART_ATTACH_RAW_MAGIC,       /* Plus/4 */
    ACTION_CART_ATTACH_RAW_MULTI,       /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C1_FULL,     /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C1_LOW,      /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C1_HIGH,     /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C2_FULL,     /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C2_LOW,      /* Plus/4 */
    ACTION_CART_ATTACH_RAW_C2_HIGH,     /* Plus/4 */
    ACTION_CART_DETACH,
    ACTION_CART_DETACH_1000,            /* CBM-II */
    ACTION_CART_DETACH_2000,            /* CBM-II */
    ACTION_CART_DETACH_4000,            /* CBM-II */
    ACTION_CART_DETACH_6000,            /* CBM-II */
    ACTION_CART_FREEZE,
    ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
    ACTION_DEBUG_BLITTER_LOG_TOGGLE,
    ACTION_DEBUG_CORE_DUMP_TOGGLE,
    ACTION_DEBUG_DMA_LOG_TOGGLE,
    ACTION_DEBUG_FLASH_LOG_TOGGLE,
    ACTION_DEBUG_TRACE_CPU_TOGGLE,
    ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,
    ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,
    ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,
    ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,
    ACTION_DEBUG_TRACE_IEC_TOGGLE,
    ACTION_DEBUG_TRACE_IEEE488_TOGGLE,
    ACTION_DEBUG_TRACE_MODE,
    ACTION_DRIVE_ATTACH_10_0,
    ACTION_DRIVE_ATTACH_10_1,
    ACTION_DRIVE_ATTACH_11_0,
    ACTION_DRIVE_ATTACH_11_1,
    ACTION_DRIVE_ATTACH_8_0,
    ACTION_DRIVE_ATTACH_8_1,
    ACTION_DRIVE_ATTACH_9_0,
    ACTION_DRIVE_ATTACH_9_1,
    ACTION_DRIVE_CREATE,
    ACTION_DRIVE_DETACH_10_0,
    ACTION_DRIVE_DETACH_10_1,
    ACTION_DRIVE_DETACH_11_0,
    ACTION_DRIVE_DETACH_11_1,
    ACTION_DRIVE_DETACH_8_0,
    ACTION_DRIVE_DETACH_8_1,
    ACTION_DRIVE_DETACH_9_0,
    ACTION_DRIVE_DETACH_9_1,
    ACTION_DRIVE_DETACH_ALL,
    ACTION_EDIT_COPY,
    ACTION_EDIT_PASTE,
    ACTION_FLIPLIST_ADD_8_0,
    ACTION_FLIPLIST_CLEAR_8_0,
    ACTION_FLIPLIST_LOAD_8_0,
    ACTION_FLIPLIST_NEXT_8_0,
    ACTION_FLIPLIST_PREVIOUS_8_0,
    ACTION_FLIPLIST_REMOVE_8_0,
    ACTION_FLIPLIST_SAVE_8_0,
    ACTION_FLIPLIST_ADD_8_1,
    ACTION_FLIPLIST_CLEAR_8_1,
    ACTION_FLIPLIST_LOAD_8_1,
    ACTION_FLIPLIST_NEXT_8_1,
    ACTION_FLIPLIST_PREVIOUS_8_1,
    ACTION_FLIPLIST_REMOVE_8_1,
    ACTION_FLIPLIST_SAVE_8_1,
    ACTION_FLIPLIST_ADD_9_0,
    ACTION_FLIPLIST_CLEAR_9_0,
    ACTION_FLIPLIST_LOAD_9_0,
    ACTION_FLIPLIST_NEXT_9_0,
    ACTION_FLIPLIST_PREVIOUS_9_0,
    ACTION_FLIPLIST_REMOVE_9_0,
    ACTION_FLIPLIST_SAVE_9_0,
    ACTION_FLIPLIST_ADD_9_1,
    ACTION_FLIPLIST_CLEAR_9_1,
    ACTION_FLIPLIST_LOAD_9_1,
    ACTION_FLIPLIST_NEXT_9_1,
    ACTION_FLIPLIST_PREVIOUS_9_1,
    ACTION_FLIPLIST_REMOVE_9_1,
    ACTION_FLIPLIST_SAVE_9_1,

    ACTION_FLIPLIST_ADD_10_0,
    ACTION_FLIPLIST_CLEAR_10_0,
    ACTION_FLIPLIST_LOAD_10_0,
    ACTION_FLIPLIST_NEXT_10_0,
    ACTION_FLIPLIST_PREVIOUS_10_0,
    ACTION_FLIPLIST_REMOVE_10_0,
    ACTION_FLIPLIST_SAVE_10_0,
    ACTION_FLIPLIST_ADD_10_1,
    ACTION_FLIPLIST_CLEAR_10_1,
    ACTION_FLIPLIST_LOAD_10_1,
    ACTION_FLIPLIST_NEXT_10_1,
    ACTION_FLIPLIST_PREVIOUS_10_1,
    ACTION_FLIPLIST_REMOVE_10_1,
    ACTION_FLIPLIST_SAVE_10_1,
    ACTION_FLIPLIST_ADD_11_0,
    ACTION_FLIPLIST_CLEAR_11_0,
    ACTION_FLIPLIST_LOAD_11_0,
    ACTION_FLIPLIST_NEXT_11_0,
    ACTION_FLIPLIST_PREVIOUS_11_0,
    ACTION_FLIPLIST_REMOVE_11_0,
    ACTION_FLIPLIST_SAVE_11_0,
    ACTION_FLIPLIST_ADD_11_1,
    ACTION_FLIPLIST_CLEAR_11_1,
    ACTION_FLIPLIST_LOAD_11_1,
    ACTION_FLIPLIST_NEXT_11_1,
    ACTION_FLIPLIST_PREVIOUS_11_1,
    ACTION_FLIPLIST_REMOVE_11_1,
    ACTION_FLIPLIST_SAVE_11_1,
    ACTION_FULLSCREEN_DECORATIONS_TOGGLE,
    ACTION_FULLSCREEN_TOGGLE,
    ACTION_HELP_ABOUT,
    ACTION_HELP_COMMAND_LINE,
    ACTION_HELP_COMPILE_TIME,
    ACTION_HELP_HOTKEYS,
    ACTION_HELP_MANUAL,
    ACTION_HISTORY_MILESTONE_RESET,
    ACTION_HISTORY_MILESTONE_SET,
    ACTION_HISTORY_PLAYBACK_START,
    ACTION_HISTORY_PLAYBACK_STOP,
    ACTION_HISTORY_RECORD_START,
    ACTION_HISTORY_RECORD_STOP,
    ACTION_HOTKEYS_CLEAR,
    ACTION_HOTKEYS_DEFAULT,
    ACTION_HOTKEYS_LOAD,
    ACTION_HOTKEYS_LOAD_FROM,
    ACTION_HOTKEYS_SAVE,
    ACTION_HOTKEYS_SAVE_TO,
    ACTION_KEYSET_JOYSTICK_TOGGLE,
    ACTION_MEDIA_RECORD,
    ACTION_MEDIA_RECORD_AUDIO,
    ACTION_MEDIA_RECORD_SCREENSHOT,
    ACTION_MEDIA_RECORD_VIDEO,
    ACTION_MEDIA_STOP,
    ACTION_MONITOR_OPEN,
    ACTION_MOUSE_GRAB_TOGGLE,
    ACTION_PAUSE_TOGGLE,
    ACTION_QUIT,
    ACTION_MACHINE_RESET_CPU,
    ACTION_MACHINE_POWER_CYCLE,
    /* TODO: rework into DRIVE_[9-11]RESET_BUTTON/POWER_CYCLE_ etc. */
    ACTION_RESET_DRIVE_8,
    ACTION_RESET_DRIVE_8_CONFIG,
    ACTION_RESET_DRIVE_8_INSTALL,
    ACTION_RESET_DRIVE_9,
    ACTION_RESET_DRIVE_9_CONFIG,
    ACTION_RESET_DRIVE_9_INSTALL,
    ACTION_RESET_DRIVE_10,
    ACTION_RESET_DRIVE_10_CONFIG,
    ACTION_RESET_DRIVE_10_INSTALL,
    ACTION_RESET_DRIVE_11,
    ACTION_RESET_DRIVE_11_CONFIG,
    ACTION_RESET_DRIVE_11_INSTALL,
    ACTION_RESTORE_DISPLAY,
    ACTION_SCREENSHOT_QUICKSAVE,
    ACTION_SETTINGS_DEFAULT,
    ACTION_SETTINGS_DIALOG,
    ACTION_SETTINGS_LOAD_EXTRA,
    ACTION_SETTINGS_LOAD_FROM,
    ACTION_SETTINGS_LOAD,
    ACTION_SETTINGS_SAVE,
    ACTION_SETTINGS_SAVE_TO,
    ACTION_SHOW_STATUSBAR_TOGGLE,
    ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
    ACTION_SMART_ATTACH,
    ACTION_SNAPSHOT_LOAD,
    ACTION_SNAPSHOT_QUICKLOAD,
    ACTION_SNAPSHOT_QUICKSAVE,
    ACTION_SNAPSHOT_SAVE,
    ACTION_SPEED_CPU_10,
    ACTION_SPEED_CPU_25,
    ACTION_SPEED_CPU_50,
    ACTION_SPEED_CPU_100,
    ACTION_SPEED_CPU_200,
    ACTION_SPEED_CPU_CUSTOM,
    ACTION_SPEED_FPS_50,
    ACTION_SPEED_FPS_60,
    ACTION_SPEED_FPS_CUSTOM,
    ACTION_SPEED_FPS_REAL,
    ACTION_SWAP_CONTROLPORT_TOGGLE,
    ACTION_TAPE_ATTACH_1,
    ACTION_TAPE_ATTACH_2,
    ACTION_TAPE_CREATE_1,
    ACTION_TAPE_CREATE_2,
    ACTION_TAPE_DETACH_1,
    ACTION_TAPE_DETACH_2,
    ACTION_TAPE_FFWD_1,
    ACTION_TAPE_FFWD_2,
    ACTION_TAPE_PLAY_1,
    ACTION_TAPE_PLAY_2,
    ACTION_TAPE_RECORD_1,
    ACTION_TAPE_RECORD_2,
    ACTION_TAPE_RESET_1,
    ACTION_TAPE_RESET_2,
    ACTION_TAPE_RESET_COUNTER_1,
    ACTION_TAPE_RESET_COUNTER_2,
    ACTION_TAPE_REWIND_1,
    ACTION_TAPE_REWIND_2,
    ACTION_TAPE_STOP_1,
    ACTION_TAPE_STOP_2,
    ACTION_WARP_MODE_TOGGLE,

    /* VSID actions */
    ACTION_PSID_LOAD,
    ACTION_PSID_OVERRIDE_TOGGLE,
    ACTION_PSID_SUBTUNE_1,
    ACTION_PSID_SUBTUNE_2,
    ACTION_PSID_SUBTUNE_3,
    ACTION_PSID_SUBTUNE_4,
    ACTION_PSID_SUBTUNE_5,
    ACTION_PSID_SUBTUNE_6,
    ACTION_PSID_SUBTUNE_7,
    ACTION_PSID_SUBTUNE_8,
    ACTION_PSID_SUBTUNE_9,
    ACTION_PSID_SUBTUNE_10,
    ACTION_PSID_SUBTUNE_11,
    ACTION_PSID_SUBTUNE_12,
    ACTION_PSID_SUBTUNE_13,
    ACTION_PSID_SUBTUNE_14,
    ACTION_PSID_SUBTUNE_15,
    ACTION_PSID_SUBTUNE_16,
    ACTION_PSID_SUBTUNE_17,
    ACTION_PSID_SUBTUNE_18,
    ACTION_PSID_SUBTUNE_19,
    ACTION_PSID_SUBTUNE_20,
    ACTION_PSID_SUBTUNE_21,
    ACTION_PSID_SUBTUNE_22,
    ACTION_PSID_SUBTUNE_23,
    ACTION_PSID_SUBTUNE_24,
    ACTION_PSID_SUBTUNE_25,
    ACTION_PSID_SUBTUNE_26,
    ACTION_PSID_SUBTUNE_27,
    ACTION_PSID_SUBTUNE_28,
    ACTION_PSID_SUBTUNE_29,
    ACTION_PSID_SUBTUNE_30,

    ACTION_PSID_SUBTUNE_DEFAULT,
    ACTION_PSID_SUBTUNE_NEXT,
    ACTION_PSID_SUBTUNE_PREVIOUS,

    ACTION_PSID_PLAY,
    ACTION_PSID_PAUSE,
    ACTION_PSID_STOP,
    ACTION_PSID_FFWD,
    ACTION_PSID_LOOP_TOGGLE,

    /* playlist actions */
    ACTION_PSID_PLAYLIST_FIRST,
    ACTION_PSID_PLAYLIST_PREVIOUS,
    ACTION_PSID_PLAYLIST_NEXT,
    ACTION_PSID_PLAYLIST_LAST,
    ACTION_PSID_PLAYLIST_ADD,
    ACTION_PSID_PLAYLIST_LOAD,
    ACTION_PSID_PLAYLIST_SAVE,
    ACTION_PSID_PLAYLIST_CLEAR,

    /* TODO: VSID playlist controls? */

    /* PET */
    ACTION_DIAGNOSTIC_PIN_TOGGLE,

    /* Printers and plotters */
    ACTION_PRINTER_FORMFEED_4,
    ACTION_PRINTER_FORMFEED_5,
    ACTION_PRINTER_FORMFEED_6,
    ACTION_PRINTER_FORMFEED_USERPORT,

    /* SDL UI only: show virtual keyboard on the emulated display */
    ACTION_VIRTUAL_KEYBOARD,

    /* Border modes */
    ACTION_BORDER_MODE_NORMAL,
    ACTION_BORDER_MODE_FULL,
    ACTION_BORDER_MODE_DEBUG,
    ACTION_BORDER_MODE_NONE,

    /* SCPU64 switches */
    ACTION_SCPU_JIFFY_SWITCH_TOGGLE,
    ACTION_SCPU_SPEED_SWITCH_TOGGLE,

    ACTION_ID_COUNT     /**< number of action IDs */
};

/* Action info getters */
int                     ui_action_get_id       (const char *name);
const char *            ui_action_get_name     (int action);
const char *            ui_action_get_desc     (int action);
ui_action_info_t *      ui_action_get_info_list(void);
bool                    ui_action_is_valid     (int action);

/* Get action IDs for drive actions */
int                     ui_action_id_fliplist_add       (int unit, int drive);
int                     ui_action_id_fliplist_remove    (int unit, int drive);
int                     ui_action_id_fliplist_next      (int unit, int drive);
int                     ui_action_id_fliplist_previous  (int unit, int drive);
int                     ui_action_id_fliplist_clear     (int unit, int drive);
int                     ui_action_id_fliplist_load      (int unit, int drive);
int                     ui_action_id_fliplist_save      (int unit, int drive);
int                     ui_action_id_drive_attach       (int unit, int drive);
int                     ui_action_id_drive_detach       (int unit, int drive);
int                     ui_action_id_drive_reset        (int unit);
int                     ui_action_id_drive_reset_config (int unit);
int                     ui_action_id_drive_reset_install(int unit);

/* Main API */
void                    ui_actions_init          (void);
void                    ui_actions_set_dispatch  (void (*dispatch)(ui_action_map_t *));
void                    ui_actions_shutdown      (void);
void                    ui_actions_register      (const ui_action_map_t *mappings);

void                    ui_action_trigger        (int action);
void                    ui_action_finish         (int action);
/* TODO: implement the following: */
bool                    ui_action_def            (int action, const char *hotkey);
bool                    ui_action_redef          (int action, const char *hotkey);
bool                    ui_action_undef          (int action);

/*
 * For the added hotkeys data:
 */

ui_action_map_t *ui_action_map_get                   (int action);
ui_action_map_t *ui_action_map_get_by_hotkey         (uint32_t vice_keysym, uint32_t vice_modmask);
ui_action_map_t *ui_action_map_get_by_arch_hotkey    (uint32_t arch_keysym, uint32_t arch_modmask);

void             ui_action_map_clear_hotkey          (ui_action_map_t *map);
void             ui_action_map_clear_hotkey_by_action(int action);
void             ui_action_map_clear_hotkey_by_hotkey(uint32_t vice_keysym, uint32_t vice_modmask);

ui_action_map_t *ui_action_map_set_hotkey            (int       action,
                                                      uint32_t  vice_keysym,
                                                      uint32_t  vice_modmask,
                                                      uint32_t  arch_keysym,
                                                      uint32_t  arch_modmask);
void             ui_action_map_set_hotkey_by_map     (ui_action_map_t *map,
                                                      uint32_t         vice_keysym,
                                                      uint32_t         vice_modmask,
                                                      uint32_t         arch_keysym,
                                                      uint32_t         arch_modmask);

char            *ui_action_map_get_hotkey_label      (ui_action_map_t *map);
char            *ui_action_get_hotkey_label          (int action);

#endif
