/** \file   src/diskconstants.h
 *
 * \brief   Disk drive specifications
 *
 * For customized disks, the values must fit beteen the NUM_ and MAX_
 * limits. Do not change the NUM_ values, as they define the standard
 * disk geometry.
 */

/*
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
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
 *
 */

#ifndef VICE_DISKCONSTANTS_H
#define VICE_DISKCONSTANTS_H


/*
 * CBM 1541 constants
 */

#define NUM_TRACKS_1541        35   /**< number of tracks on a standard 1541
                                         floppy */
#define NUM_BLOCKS_1541        683  /**< number of blocks on a standard 1541
                                         floppy (664 free) */
#define EXT_TRACKS_1541        40   /**< number of tracks in an extended 1541
                                         floppy, used by SpeedDOS,
                                             ProLogic DOS, etc */
#define EXT_BLOCKS_1541        768  /**< number of blocks on an extended 1541
                                         floppy */
#define MAX_TRACKS_1541        42   /**< maximum number of accessible tracks for
                                         a 1541 drive: not all 1541 drives can
                                         do this */
#define MAX_BLOCKS_1541        802  /**< maximum number of blocks for a 1541
                                         floppy, using 42 tracks */
#define DIR_TRACK_1541         18   /**< 1541 directory track number */
#define DIR_SECTOR_1541        1    /**< 1541 directory sector number */
#define BAM_TRACK_1541         18   /**< 1541 BAM track number */
#define BAM_SECTOR_1541        0    /**< 1541 BAM sector number */
#define BAM_NAME_1541          144  /**< offset in BAM of the disk name in
                                         PETSCII, 16 bytes, padded with 0xa0 */
#define BAM_ID_1541            162  /**< offset in BAM of the disk ID in
                                         PETSCII, 2 bytes for standard DOS */
#define BAM_EXT_BIT_MAP_1541   192  /**< BAM bitmap entries for tracks 36-40 for
                                         extended floppies, only valid for
                                         SpeedDOS */


/* CBM 2040 constants */

#define NUM_TRACKS_2040        35   /**< number of tracks on a 2040 floppy */
#define NUM_BLOCKS_2040        690  /**< number of blocks on a 2040 floppy
                                         (670 free */
#define MAX_TRACKS_2040        35   /**< maximum number of tracks on a 2040
                                         floppy */
#define MAX_BLOCKS_2040        690  /**< maximum number of blocks on a 2040
                                         floppy */
#define DIR_TRACK_2040         18   /**< directory track number */
#define DIR_SECTOR_2040        1    /**< directory sector number */
#define BAM_TRACK_2040         18   /**< BAM track number */
#define BAM_SECTOR_2040        0    /**< BAM sector number */
#define BAM_NAME_2040          144  /**< offset in BAM of the disk name in
                                         PETSCII, 16 bytes, padded with 0xa0 */
#define BAM_ID_2040            162  /**< offset in BAM of the disk ID in
                                         PETSCII, 2 bytes */
#define BAM_EXT_BIT_MAP_2040   192  /**< BAM bitmap entries for tracks 36-40 for
                                         extended floppies (did these exist?) */


/* CBM 1571 constants, these constants are for a double-sided 1571
 *
 * FIXME: constants for the second BAM at (53,0) are missing
 */

#define NUM_TRACKS_1571        70   /**< number of tracks on a 1571 floppy */
#define NUM_BLOCKS_1571        1366 /**< number of blocks on a 1571 floppy
                                         (1328 blocks free) */
#define MAX_TRACKS_1571        70   /**< maximum number of tracks on a 1571
                                         floppy */
#define MAX_BLOCKS_1571        1366 /**< maximum number of blocks on a 1571
                                         floppy */
#define DIR_TRACK_1571         18   /**< directory track number */
#define DIR_SECTOR_1571        1    /**< directory sector number */
#define BAM_TRACK_1571         18   /**< BAM track number (side 1 BAM) */
#define BAM_SECTOR_1571        0    /**< BAM sector number (side 1 BAM) */
#define BAM_NAME_1571          144  /**< offset in BAM of the disk name in
                                         PETSCII, 16 bytes, padded with 0xa0 */
#define BAM_ID_1571            162  /**< offset in BAM of the disk ID in
                                         PETSCII, 2 bytes */
#define BAM_EXT_BIT_MAP_1571   221  /**< BAM bitmap entries for tracks 36-40 for
                                         extended floppies (did these exist?) */


/* CBM 1581 constants */

#define NUM_TRACKS_1581        80   /**< number of tracks on a 1581 diskette */
#define NUM_SECTORS_1581       40   /**< number of sectors per track */
#define NUM_BLOCKS_1581        3200 /**< number of blocks on a 1581 diskette
                                         (3160 blocks free) */
#define MAX_TRACKS_1581        83   /**< maximum number of tracks on a 1581
                                         diskette */
#define MAX_BLOCKS_1581        3320 /**< maximum number of blocks on a 1581
                                         diskette when using 83 tracks */
#define DIR_TRACK_1581         40   /**< directory track number */
#define DIR_SECTOR_1581        3    /**< directory sector number */
#define BAM_TRACK_1581         40   /**< BAM track number */
#define BAM_SECTOR_1581        0    /**< BAM first sector number (the BAM of a
                                        1581 diskette is three sectors) */
#define BAM_NAME_1581          4    /**< offset in BAM of disk name in PETSCII,
                                         16 bytes, padded with 0xa0 */
#define BAM_ID_1581            22   /**< offset in BAM of disk ID in PETSCII,
                                         2 bytes */

/* FIXME: I'm not really familiar with the following drives, so documentation
 *        of these constants will have to wait.
 */

/* CBM 8050 constants */

#define NUM_TRACKS_8050        77
#define NUM_BLOCKS_8050        2083     /* 2052 free */
#define MAX_TRACKS_8050        77
#define MAX_BLOCKS_8050        2083
#define BAM_TRACK_8050         39
#define BAM_SECTOR_8050        0
#define BAM_NAME_8050          6        /* pos. of disk name in 1st BAM blk */
#define BAM_ID_8050            24       /* pos. of disk id in 1st BAM blk */
#define DIR_TRACK_8050         39
#define DIR_SECTOR_8050        1

#define NUM_TRACKS_8250        154
#define NUM_BLOCKS_8250        4166     /* 4133 free */
#define MAX_TRACKS_8250        154
#define MAX_BLOCKS_8250        4166
#define BAM_TRACK_8250         39
#define BAM_SECTOR_8250        0
#define BAM_NAME_8250          6        /* pos. of disk name in 1st BAM blk */
#define BAM_ID_8250            24       /* pos. of disk id in 1st BAM blk */
#define DIR_TRACK_8250         39
#define DIR_SECTOR_8250        1

#define NUM_TRACKS_1000        13
#define NUM_BLOCKS_1000        3240
#define NUM_SYS_SECTORS_1000   168      /* on system partition track */
#define MAX_TRACKS_1000        13
#define MAX_BLOCKS_1000        3240

#define NUM_TRACKS_2000        26
#define NUM_BLOCKS_2000        6480
#define NUM_SYS_SECTORS_2000   80       /* on system partition track */
#define MAX_TRACKS_2000        26
#define MAX_BLOCKS_2000        6480

#define NUM_TRACKS_4000        51
#define NUM_BLOCKS_4000        12960
#define NUM_SYS_SECTORS_4000   160      /* on system partition track */
#define MAX_TRACKS_4000        51
#define MAX_BLOCKS_4000        12960

#define DIR_TRACK_4000         1
#define DIR_SECTOR_4000        34
#define BAM_TRACK_4000         1
#define BAM_SECTOR_4000        1
#define BAM_NAME_4000          4
#define BAM_ID_4000            22

#define MAX_TRACKS_ANY         MAX_TRACKS_8250
#define MAX_BLOCKS_ANY         MAX_BLOCKS_8250

#endif
