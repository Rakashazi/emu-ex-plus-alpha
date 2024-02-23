/** \file   archdep_set_openmp_wait_policy.c
 * \brief   Set OMP_WAIT_POLICY to PASSIVE
 * \author  groepaz <groepaz@gmx.net>
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
 *
 */

#include "vice.h"
#include "archdep_defs.h"
#include "archdep_set_openmp_wait_policy.h"

#include <stdlib.h>

#include "archdep_set_openmp_wait_policy.h"

/*
 * OpenMP defaults to spinning threads for a couple hundred ms
 * after they are used, which means they max out forever in our
 * use case. Setting OMP_WAIT_POLICY to PASSIVE fixes that.
 */

void archdep_set_openmp_wait_policy(void)
{
#if defined(WINDOWS_COMPILE)
    _putenv("OMP_WAIT_POLICY=PASSIVE");
#else
    setenv("OMP_WAIT_POLICY", "PASSIVE", 1);
#endif
}
