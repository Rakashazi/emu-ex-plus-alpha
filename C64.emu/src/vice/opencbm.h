/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005           Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2008-2009 Spiro Trikaliotis
 *  Copyright 2006,2011           Wolfgang Moser (http://d81.de)
 *  Copyright 2009,2012           Arnd Menge
 *  Copyright 2011                Thomas Winkler
 */

/*! **************************************************************
** \file include/opencbm.h \n
** \author Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de> \n
** \authors With modifications to fit on Windows from
**    Spiro Trikaliotis \n
** \authors With additions from Wolfgang Moser \n
** \authors With CBM 1530/1531 tape drive additions from Arnd Menge \n
** \n
** \brief DLL interface for accessing the driver
**
****************************************************************/

#ifndef OPENCBM_H
#define OPENCBM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifdef WIN32
  /* we have windows */

#include <windows.h>

#ifdef DEFINE_ULONG_PTR
#define ULONG_PTR ULONG
#endif /* #ifdef DEFINE_ULONG_PTR */


# if defined DLL
#  define EXTERN __declspec(dllexport) /*!< we are exporting the functions */
# else
#  define EXTERN __declspec(dllimport) /*!< we are importing the functions */
# endif


#define CBMAPIDECL __cdecl /*!< On Windows, we need c-type function declarations */
# define CBM_FILE HANDLE /*!< The "file descriptor" for an opened driver */
# define CBM_FILE_INVALID INVALID_HANDLE_VALUE /*!< An invalid "file descriptor" (CBM_FILE) */

#elif defined(__MSDOS__)

  /* we have MS-DOS */

#include <stdlib.h>

# define EXTERN extern /*!< EXTERN is not defined on MS-DOS */
# define CBMAPIDECL /*!< CBMAPIDECL is a dummy on MS-DOS */
# define WINAPI /*!< WINAPI is a dummy on MS-DOS */
# define CBM_FILE int /*!< The "file descriptor" for an opened driver */
# define CBM_FILE_INVALID ((CBM_FILE)-1)

extern int vdd_init(void);
extern void vdd_uninit(void);
extern int vdd_install_iohook(CBM_FILE f, int IoBaseAddress, int CableType);
extern int vdd_uninstall_iohook(CBM_FILE f);
extern void vdd_usleep(CBM_FILE f, unsigned int howlong);
#else

  /* we have linux or Mac */

/*
 * remove this include once the CBM_FILE intptr_t declaration
 * below is changed back to int after the plugin/driver handle
 * mapping was implemented
 */
#include <stdint.h>

# define EXTERN extern /*!< EXTERN is not defined on Linux */
# define CBMAPIDECL /*!< CBMAPIDECL is a dummy on Linux */
# define WINAPI /*!< WINAPI is a dummy on Linux */
# define CBM_FILE intptr_t /*!< The "file descriptor" for an opened driver */
# define CBM_FILE_INVALID ((CBM_FILE)-1) /*!< An invalid "file descriptor" (CBM_FILE) */

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x)
#endif

#endif

/* specifiers for the IEC bus lines */
#define IEC_DATA   0x01 /*!< Specify the DATA line */
#define IEC_CLOCK  0x02 /*!< Specify the CLOCK line */
#define IEC_ATN    0x04 /*!< Specify the ATN line */
#define IEC_RESET  0x08 /*!< Specify the RESET line */
#define IEC_SRQ    0x10 /*!< Specify the SRQ line */

/* specifiers for the IEEE-488 bus lines  */
#define IEE_NDAC    0x01 /*!< Specify the NDAC line */
#define IEE_NRFD    0x02 /*!< Specify the NRFD line */
#define IEE_ATN     0x04 /*!< Specify the ATN line */
#define IEE_IFC     0x08 /*!< Specify the IFC line */
#define IEE_DAV     0x10 /*!< Specify the DAV line */
#define IEE_EOI     0x20 /*!< Specify the EOI line */
#define IEE_REN     0x40 /*!< Specify the REN line */
#define IEE_SRQ     0x80 /*!< Specify the SRQ line */

/*! Specifies the type of a device for cbm_identify() */
enum cbm_device_type_e
{
    cbm_dt_unknown = -1, /*!< The device could not be identified */
    cbm_dt_cbm1541,      /*!< The device is a VIC 1541 */
    cbm_dt_cbm1570,      /*!< The device is a VIC 1570 */
    cbm_dt_cbm1571,      /*!< The device is a VIC 1571 */
    cbm_dt_cbm1581,      /*!< The device is a VIC 1581 */
    cbm_dt_cbm2040,      /*!< The device is a CBM-2040 DOS1 or 2   */
    cbm_dt_cbm2031,      /*!< The device is a CBM-2031 DOS2.6      */
    cbm_dt_cbm3040,      /*!< The device is a CBM-3040 DOS1 or 2   */
    cbm_dt_cbm4040,      /*!< The device is a CBM-4040 DOS2        */
    cbm_dt_cbm4031,      /*!< The device is a CBM-4031 DOS2.6      */
    cbm_dt_cbm8050,      /*!< The device is a CBM-8050             */
    cbm_dt_cbm8250,      /*!< The device is a CBM-8250 or SFD-1001 */
    cbm_dt_sfd1001       /*!< The device is a SFD-1001             */
};

/*! Specifies the type of a device for cbm_identify() */
enum cbm_cable_type_e
{
    cbm_ct_unknown = -1, /*!< The device could not be identified */
    cbm_ct_none,         /*!< The device does not have a parallel cable */
    cbm_ct_xp1541        /*!< The device does have a parallel cable */
};

/*! \todo FIXME: port isn't used yet */
EXTERN int CBMAPIDECL cbm_driver_open(CBM_FILE *f, int port);
EXTERN int CBMAPIDECL cbm_driver_open_ex(CBM_FILE *f, char * adapter);
EXTERN void CBMAPIDECL cbm_driver_close(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_lock(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_unlock(CBM_FILE f);

/*! \todo FIXME: port isn't used yet */
EXTERN const char * CBMAPIDECL cbm_get_driver_name(int port);
EXTERN const char * CBMAPIDECL cbm_get_driver_name_ex(char * adapter);

EXTERN int CBMAPIDECL cbm_listen(CBM_FILE f, unsigned char dev, unsigned char secadr);
EXTERN int CBMAPIDECL cbm_talk(CBM_FILE f, unsigned char dev, unsigned char secadr);

EXTERN int CBMAPIDECL cbm_open(CBM_FILE f, unsigned char dev, unsigned char secadr, const void *fname, size_t len);
EXTERN int CBMAPIDECL cbm_close(CBM_FILE f, unsigned char dev, unsigned char secadr);

EXTERN int CBMAPIDECL cbm_raw_read(CBM_FILE f, void *buf, size_t size);
EXTERN int CBMAPIDECL cbm_raw_write(CBM_FILE f, const void *buf, size_t size);

EXTERN int CBMAPIDECL cbm_unlisten(CBM_FILE f);
EXTERN int CBMAPIDECL cbm_untalk(CBM_FILE f);

EXTERN int CBMAPIDECL cbm_get_eoi(CBM_FILE f);
EXTERN int CBMAPIDECL cbm_clear_eoi(CBM_FILE f);

EXTERN int CBMAPIDECL cbm_reset(CBM_FILE f);

EXTERN unsigned char CBMAPIDECL cbm_pp_read(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_pp_write(CBM_FILE f, unsigned char c);

EXTERN int CBMAPIDECL cbm_iec_poll(CBM_FILE f);
EXTERN int CBMAPIDECL cbm_iec_get(CBM_FILE f, int line);
EXTERN void CBMAPIDECL cbm_iec_set(CBM_FILE f, int line);
EXTERN void CBMAPIDECL cbm_iec_release(CBM_FILE f, int line);
EXTERN void CBMAPIDECL cbm_iec_setrelease(CBM_FILE f, int set, int release);
EXTERN int CBMAPIDECL cbm_iec_wait(CBM_FILE f, int line, int state);

EXTERN int CBMAPIDECL cbm_upload(CBM_FILE f, unsigned char dev, int adr, const void *prog, size_t size);
EXTERN int CBMAPIDECL cbm_download(CBM_FILE f, unsigned char dev, int adr, void *dbuf, size_t size);

EXTERN int CBMAPIDECL cbm_device_status(CBM_FILE f, unsigned char dev, void *buf, size_t bufsize);
EXTERN int CBMAPIDECL cbm_exec_command(CBM_FILE f, unsigned char dev, const void *cmd, size_t len);

EXTERN int CBMAPIDECL cbm_identify(CBM_FILE f, unsigned char drv,
                                   enum cbm_device_type_e *t,
                                   const char **type_str);

EXTERN unsigned int CBMAPIDECL cbm_determine_pport_address(enum cbm_device_type_e CbmDeviceType);

EXTERN int CBMAPIDECL cbm_identify_xp1541(CBM_FILE HandleDevice,
                                          unsigned char DeviceAddress,
                                          enum cbm_device_type_e *CbmDeviceType,
                                          enum cbm_cable_type_e *CableType);


EXTERN char CBMAPIDECL cbm_petscii2ascii_c(char character);
EXTERN char CBMAPIDECL cbm_ascii2petscii_c(char character);
EXTERN char * CBMAPIDECL cbm_petscii2ascii(char *str);
EXTERN char * CBMAPIDECL cbm_ascii2petscii(char *str);

EXTERN int CBMAPIDECL gcr_5_to_4_decode(const unsigned char *source, unsigned char *dest,
                                        size_t sourceLength,         size_t destLength);
EXTERN int CBMAPIDECL gcr_4_to_5_encode(const unsigned char *source, unsigned char *dest,
                                        size_t sourceLength,         size_t destLength);


#if DBG
EXTERN int CBMAPIDECL cbm_get_debugging_buffer(CBM_FILE HandleDevice, char *buffer, size_t len);
#endif

EXTERN int CBMAPIDECL cbm_iec_dbg_read (CBM_FILE HandleDevice);
EXTERN int CBMAPIDECL cbm_iec_dbg_write(CBM_FILE HandleDevice, unsigned char Value);

/* functions specifically for parallel burst */

EXTERN unsigned char CBMAPIDECL cbm_parallel_burst_read(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_parallel_burst_write(CBM_FILE f, unsigned char c);
EXTERN int CBMAPIDECL cbm_parallel_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length);
EXTERN int CBMAPIDECL cbm_parallel_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length);
EXTERN int CBMAPIDECL  cbm_parallel_burst_read_track(CBM_FILE f, unsigned char *buffer, unsigned int length);
EXTERN int CBMAPIDECL  cbm_parallel_burst_read_track_var(CBM_FILE f, unsigned char *buffer, unsigned int length);
EXTERN int CBMAPIDECL cbm_parallel_burst_write_track(CBM_FILE f, unsigned char *buffer, unsigned int length);

/* parallel burst functions end */

/* functions specifically for srq nibbler */

EXTERN unsigned char CBMAPIDECL cbm_srq_burst_read(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_srq_burst_write(CBM_FILE f, unsigned char c);
EXTERN int CBMAPIDECL cbm_srq_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length);
EXTERN int CBMAPIDECL cbm_srq_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length);
EXTERN int CBMAPIDECL  cbm_srq_burst_read_track(CBM_FILE f, unsigned char *buffer, unsigned int length);
EXTERN int CBMAPIDECL cbm_srq_burst_write_track(CBM_FILE f, unsigned char *buffer, unsigned int length);

/* srq nibbler functions end */

/* functions specifically for CBM 153x tape drive */

EXTERN int CBMAPIDECL cbm_tap_prepare_capture(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_prepare_write(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_get_sense(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_wait_for_stop_sense(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_wait_for_play_sense(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_start_capture(CBM_FILE f, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead);
EXTERN int CBMAPIDECL cbm_tap_start_write(CBM_FILE f, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten);
EXTERN int CBMAPIDECL cbm_tap_motor_on(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_motor_off(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_get_ver(CBM_FILE f, int *Status);
EXTERN int CBMAPIDECL cbm_tap_download_config(CBM_FILE f, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead);
EXTERN int CBMAPIDECL cbm_tap_upload_config(CBM_FILE f, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten);
EXTERN int CBMAPIDECL cbm_tap_break(CBM_FILE f);

/* tape capture functions end */

/* get function address of the plugin */
EXTERN void * CBMAPIDECL cbm_get_plugin_function_address(const char * Functionname);

#ifdef __cplusplus
}
#endif

#endif /* OPENCBM_H */
