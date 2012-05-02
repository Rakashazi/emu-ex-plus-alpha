/*
 * $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/MegaSCSIsub.c,v $
 *
 * $Revision: 1.9 $
 *
 * $Date: 2007-03-28 17:35:35 $
 *
 * Copyright (C) 2007 white cat
 *
 * Drive setting parameters for MEGA-SCSI and WAVE-SCSI.
 */

#include "MegaSCSIsub.h"

#if 0
// The SCSI mode making parameter moves to MB89352.c.

#include "ScsiDefs.h"
#include "ScsiDevice.h"
#include "Disk.h"
#include <stdlib.h>


SCSICREATE MegaSCSIparm[8] = {
{
    /* #0 */
    NULL, SDT_DirectAccess,
    MODE_SCSI2 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #1 */
    NULL, SDT_DirectAccess,
    MODE_SCSI2 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #2 */
    NULL, SDT_DirectAccess,
    MODE_SCSI2 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #3 */
    NULL, SDT_OpticalMemory,
    MODE_SCSI1 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #4 */
    NULL, SDT_DirectAccess,
    MODE_SCSI2 | MODE_MEGASCSI | MODE_CHECK2 | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #5 */
    NULL, SDT_DirectAccess,
    MODE_SCSI2 | MODE_MEGASCSI | MODE_CHECK2 | MODE_FDS120 | MODE_REMOVABLE },
{
    /* #6 */
    NULL, SDT_CDROM,
    MODE_SCSI2 | MODE_UNITATTENTION | MODE_REMOVABLE },
{
    /* #7 */
    NULL, SDT_DirectAccess,
    MODE_SCSI3 | MODE_MEGASCSI | MODE_CHECK2 | MODE_FDS120 | MODE_REMOVABLE }
};

/*
     Sample
     0123456789ABCDEF (needs 16bytes)
    "PRODUCT NAME    ", SDT_DirectAccess,
    MODE_SCSI2 | MODE_UNITATTENTION | MODE_MEGASCSI | MODE_REMOVABLE }
*/

const SCSICREATE* getMegaSCSIparm(int hdId)
{
    // CD_UPDATE: Dynnamically create parameter table and select correct
    //            parameters based on inserted disk
    int i;
    for (i = 0; i < 8; ++i) {
        if (diskIsCdrom(diskGetHdDriveId(hdId, i))) {
            MegaSCSIparm[i].deviceType = SDT_CDROM;
            MegaSCSIparm[i].scsiMode   = MODE_SCSI2 | MODE_UNITATTENTION | MODE_REMOVABLE;
        }
        else {
            MegaSCSIparm[i].deviceType = SDT_DirectAccess;
            MegaSCSIparm[i].scsiMode   = MODE_SCSI2 | MODE_MEGASCSI | MODE_CHECK2 | MODE_FDS120 | MODE_REMOVABLE;
            MegaSCSIparm[i].scsiMode   = MODE_SCSI2 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE;
        }
    }
    return MegaSCSIparm;
}
#endif

int EseRamSize(int size)
{
    int i = 0;
    size /= 0x20000;
    do {
        if ((size >>= 1) == 0) {
            break;
        }
        ++i;
    } while (i < 3);
    return i;
}
