/*
 * libm_math.c - Math functions for platforms that are missing libm
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#if !defined(HAVE_LIBM) && !defined(BEOS_COMPILE) && !defined(_UWIN) && !defined(RHAPSODY_COMPILE)
#include <errno.h>

#ifdef HAVE_MATH_H
#include <math.h>
#else
#include "libm_math.h"
#endif

int errno;

extern double frexp(double, int *);
extern double modf(double, double *);

static double log2 = 0.693147180559945309e0;
static double sqrto2 = 0.707106781186547524e0;
static double logp0 = -.240139179559210510e2;
static double logp1 = 0.309572928215376501e2;
static double logp2 = -.963769093368686593e1;
static double logp3 = 0.421087371217979714e0;
static double logq0 = -.120069589779605255e2;
static double logq1 = 0.194809660700889731e2;
static double logq2 = -.891110902798312337e1;

static double expp0 = .2080384346694663001443843411e7;
static double expp1 = .3028697169744036299076048876e5;
static double expp2 = .6061485330061080841615584556e2;
static double expq0 = .6002720360238832528230907598e7;
static double expq1 = .3277251518082914423057964422e6;
static double expq2 = .1749287689093076403844945335e4;
static double log2e = 1.4426950408889634073599247;
static double sqrt2 = 1.4142135623730950488016887;
static double maxf = 10000;

static double twoopi = 0.63661977236758134308;
static double sinp0 = .1357884097877375669092680e8;
static double sinp1 = -.4942908100902844161158627e7;
static double sinp2 = .4401030535375266501944918e6;
static double sinp3 = -.1384727249982452873054457e5;
static double sinp4 = .1459688406665768722226959e3;
static double sinq0 = .8644558652922534429915149e7;
static double sinq1 = .4081792252343299749395779e6;
static double sinq2 = .9463096101538208180571257e4;
static double sinq3 = .1326534908786136358911494e3;

double floor(double d)
{
    double fract;

    if (d < 0.0) {
        d = -d;
        fract = modf(d, &d);
        if (fract != 0.0) {
            d += 1;
        }
        d = -d;
    } else {
        modf(d, &d);
    }
    return (d);
}

double ceil(double d)
{
    return (-floor(-d));
}

static double sinus(double arg, int quad)
{
    double e, f;
    double ysq;
    double x, y;
    int k;
    double temp1, temp2;

    x = arg;
    if (x < 0) {
        x = -x;
        quad = quad + 2;
    }
    x = x * twoopi;
    if (x > 32764) {
        y = modf(x, &e);
        e = e + quad;
        modf(0.25 * e, &f);
        quad = e - 4 * f;
    } else {
        k = x;
        y = x - k;
        quad = (quad + k) & 03;
    }
    if (quad & 1) {
        y = 1 - y;
    }
    if (quad > 1) {
        y = -y;
    }
    ysq = y * y;
    temp1 = ((((sinp4 * ysq + sinp3) * ysq + sinp2) * ysq + sinp1) * ysq + sinp0) * y;
    temp2 = ((((ysq + sinq3) * ysq + sinq2) * ysq + sinq1) * ysq + sinq0);
    return (temp1 / temp2);
}

double cos(double arg)
{
    if (arg < 0) {
        arg = -arg;
    }
    return (sinus(arg, 1));
}

double sin(double arg)
{
    return (sinus(arg, 0));
}

double exp(double arg)
{
    double fract;
    double temp1, temp2, xsq;
    int ent;

    if (arg == 0.) {
        return (1.);
    }
    if (arg < -maxf) {
        return (0.);
    }
    if (arg > maxf) {
        errno = ERANGE;
        return (HUGE);
    }
    arg *= log2e;
    ent = floor(arg);
    fract = (arg - ent) - 0.5;
    xsq = fract * fract;
    temp1 = ((expp2 * xsq + expp1) * xsq + expp0) * fract;
    temp2 = ((1.0 * xsq + expq2) * xsq + expq1) * xsq + expq0;
    return (ldexp(sqrt2 * (temp2 + temp1) / (temp2 - temp1), ent));
}

double log(double arg)
{
    double x, z, zsq, temp;
    int exp;

    if (arg <= 0.) {
        errno = EDOM;
        return (-HUGE);
    }
    x = frexp(arg, &exp);
    while (x < 0.5) {
        x = x * 2;
        exp = exp - 1;
    }
    if (x < sqrto2) {
        x = 2 * x;
        exp = exp - 1;
    }

    z = (x - 1) / (x + 1);
    zsq = z * z;

    temp = ((logp3 * zsq + logp2) * zsq + logp1) * zsq + logp0;
    temp = temp / (((1.0 * zsq + logq2) * zsq + logq1) * zsq + logq0);
    temp = temp * z + exp * log2;
    return (temp);
}

double pow(double arg1, double arg2)
{
    double temp;
    long l;

    if (arg1 <= 0.) {
        if (arg1 == 0.) {
            if (arg2 <= 0.) {
                goto domain;
            }
            return (0.);
        }
        l = arg2;
        if (l != arg2) {
            goto domain;
        }
        temp = exp(arg2 * log(-arg1));
        if (l & 1) {
            temp = -temp;
        }
        return(temp);
    }
    return (exp(arg2 * log(arg1)));

domain:
    errno = EDOM;
    return (0.);
}

double sqrt(double arg)
{
    double x, temp;
    int exp;
    int i;

    if (arg <= 0.) {
        if (arg < 0.) {
            errno = EDOM;
        }
        return (0.);
    }

    x = frexp(arg, &exp);

    while (x < 0.5) {
        x *= 2;
        exp--;
    }

    if (exp & 1) {
        x *= 2;
        exp--;
    }

    temp = 0.5 * (1.0 + x);

    while (exp > 60) {
        temp *= (1L << 30);
        exp -= 60;
    }

    while (exp < -60) {
        temp /= (1L << 30);
        exp += 60;
    }

    if (exp >= 0) {
        temp *= 1L << (exp / 2);
    } else {
        temp /= 1L << (-exp / 2);
    }

    for (i = 0; i <= 4; i++) {
        temp = 0.5 * (temp + arg / temp);
    }

    return (temp);
}
#endif
