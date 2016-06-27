/*
 * diskconstants.h - Disk constants.
 *
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
 * Disk Drive Specs
 * For customized disks, the values must fit beteen the NUM_ and MAX_
 * limits. Do not change the NUM_ values, as they define the standard
 * disk geometry.
 */

#define NUM_TRACKS_1541        35
#define NUM_BLOCKS_1541        683      /* 664 free */
#define EXT_TRACKS_1541        40
#define EXT_BLOCKS_1541        768
#define MAX_TRACKS_1541        42
#define MAX_BLOCKS_1541        802
#define DIR_TRACK_1541         18
#define DIR_SECTOR_1541        1
#define BAM_TRACK_1541         18
#define BAM_SECTOR_1541        0
#define BAM_NAME_1541          144
#define BAM_ID_1541            162
#define BAM_EXT_BIT_MAP_1541   192

#define NUM_TRACKS_2040        35
#define NUM_BLOCKS_2040        690      /* 670 free */
#define MAX_TRACKS_2040        35
#define MAX_BLOCKS_2040        690
#define DIR_TRACK_2040         18
#define DIR_SECTOR_2040        1
#define BAM_TRACK_2040         18
#define BAM_SECTOR_2040        0
#define BAM_NAME_2040          144
#define BAM_ID_2040            162
#define BAM_EXT_BIT_MAP_2040   192

#define NUM_TRACKS_1571        70
#define NUM_BLOCKS_1571        1366     /* 1328 free */
#define MAX_TRACKS_1571        70
#define MAX_BLOCKS_1571        1366
#define DIR_TRACK_1571         18
#define DIR_SECTOR_1571        1
#define BAM_TRACK_1571         18
#define BAM_SECTOR_1571        0
#define BAM_NAME_1571          144
#define BAM_ID_1571            162
#define BAM_EXT_BIT_MAP_1571   221

#define NUM_TRACKS_1581        80
#define NUM_SECTORS_1581       40       /* Logical sectors */
#define NUM_BLOCKS_1581        3200     /* 3160 free */
#define MAX_TRACKS_1581        83
#define MAX_BLOCKS_1581        3320
#define DIR_TRACK_1581         40
#define DIR_SECTOR_1581        3
#define BAM_TRACK_1581         40
#define BAM_SECTOR_1581        0
#define BAM_NAME_1581          4
#define BAM_ID_1581            22

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
