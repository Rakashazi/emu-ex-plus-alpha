/***************************************************************************
 Emulation by the MESS team (www.mess.org)
 scsidev.h - Header which defines the interface between SCSI device handlers
             and SCSI interfaces.

***************************************************************************/

#ifndef _SCSIDEV_H_
#define _SCSIDEV_H_

#define SCSI_MAX_DEVICES	(16)

typedef int (* pSCSIDispatch)(int operation, void *file, Int64 intparm, UInt8 *ptrparm);

typedef struct scsiconfigitem
{
	int scsiID;
	int diskID;
	pSCSIDispatch handler;
} SCSIConfigItem;

typedef struct scsiconfigtable
{
	int devs_present;
	SCSIConfigItem devices[SCSI_MAX_DEVICES];
} SCSIConfigTable;

// SCSI IDs
enum
{
	SCSI_ID_0 = 0,
	SCSI_ID_1,
	SCSI_ID_2,
	SCSI_ID_3,
	SCSI_ID_4,
	SCSI_ID_5,
	SCSI_ID_6,
	SCSI_ID_7
};

// commands accepted by a SCSI device's dispatch handler
enum
{
	SCSIOP_EXEC_COMMAND = 0,	// execute a command packet
	SCSIOP_READ_DATA,		// data transfer from the device
	SCSIOP_WRITE_DATA,		// data transfer to the device
	SCSIOP_ALLOC_INSTANCE,		// allocate an instance of the device
	SCSIOP_DELETE_INSTANCE,		// delete an instance of the device
	SCSIOP_GET_DEVICE,		// get the device's internal device (CDROM or HDD pointer)
	SCSIOP_SET_DEVICE,		// set the device's internal device (CDROM or HDD pointer)
};

// macros to make config structs cleaner
#define SCSI_DEVICE_CDROM scsicd_dispatch
#define SCSI_DEVICE_HARDDISK scsihd_dispatch

// CD-ROM handler
int scsicd_dispatch(int operation, void *file, Int64 intparm, UInt8 *ptrparm);

// hard disk handler
int scsihd_dispatch(int operation, void *file, Int64 intparm, UInt8 *ptrparm);

#endif

