/* Platform CPU discovery
 *
 * CPU        | compiletime-support | runtime-support
 * -------------------------------------------------------
 * alpha      | yes, +sub           | not yet
 * amd64      | yes                 | not yet
 * arc        | yes, +endian        | not yet
 * arm        | yes, +endian +sub   | not yet
 * avr32      | yes, -sub           | not yet
 * blackfin   | yes, +sub           | not yet
 * cris       | yes, ~sub           | not yet
 * crx        | no                  | not yet
 * fr30       | no                  | not yet
 * frv        | no                  | not yet
 * h8300      | no                  | not yet
 * hppa       | yes, -sub           | not yet
 * ia64       | yes, -sub           | not yet
 * lm32       | no                  | not yet
 * m32c       | no                  | not yet
 * m32r       | no                  | not yet
 * m68k       | yes, incomplete sub | not yet
 * m68hc1x    | no                  | not yet
 * mcore      | no                  | not yet
 * mep        | no                  | not yet
 * microblaze | no                  | not yet
 * mips       | yes, +endian -sub   | not yet
 * mips64     | yes, +endian -sub   | not yet
 * mmix       | no                  | not yet
 * mn10300    | no                  | not yet
 * ns32k      | yes                 | not yet
 * pdp-11     | no                  | not yet
 * picochip   | no                  | not yet
 * powerpc    | yes, -sub           | not yet
 * powerpc64  | yes, -sub           | not yet
 * rx         | no                  | not yet
 * s390       | yes                 | not yet
 * s390x      | yes                 | not yet
 * score      | no                  | not yet
 * sh         | yes, -sub -endian   | not yet
 * sparc      | yes, -sub           | not yet
 * sparc64    | yes, -sub           | not yet
 * tile       | no                  | not yet
 * vax        | yes                 | not yet
 * x86        | yes, +sub           | in progress
 * xtensa     | no                  | not yet
 */

#ifndef VICE_PLATFORM_CPU_TYPE_H
#define VICE_PLATFORM_CPU_TYPE_H

#include "types.h"

#include <string.h>

/* Generic alpha cpu discovery */
#if !defined(FIND_ALPHA_CPU) && (defined(__alpha__) || defined(__alpha_ev6__) || defined(__alpha_ev5__) || defined(__alpha_ev4__))
#define FIND_ALPHA_CPU
#endif

#ifdef FIND_ALPHA_CPU

#ifdef __alpha_ev6__
#define PLATFORM_CPU "Alpha EV6"
#endif

#if !defined(PLATFORM_CPU) && defined(__alpha_ev5__)
#define PLATFORM_CPU "Alpha EV5"
#endif

#if !defined(PLATFORM_CPU) && defined(__alpha_ev4__)
#define PLATFORM_CPU "Alpha EV4"
#endif

#ifndef PLATFORM_CPU
#define PLATFORM_CPU "Alpha"
#endif

#endif


/* Generic amd64/x86_64 cpu discovery */
#if !defined(PLATFORM_CPU) && (defined(__amd64__) || defined(__x86_64__))
#define PLATFORM_CPU "AMD64/x86_64"
#endif


/* Generic arc cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__arc__)
#  ifdef WORDS_BIGENDIAN
#    define PLATFORM_CPU "ARC (big endian)"
#  else
#    define PLATFORM_CPU "ARC (little endian)"
#  endif
#endif


/* Generic arm cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__arm__)

#  ifdef WORDS_BIGENDIAN
#    define PLATFORM_ENDIAN " (big endian)"
#  else
#    define PLATFORM_ENDIAN " (little endian)"
#  endif

/* find specific cpu name definitions first */
#  if defined(arm2)
#    define PCPU "ARM2"
#  elif defined(arm250)
#    define PCPU "ARM250"
#  elif defined(arm3)
#    define PCPU "ARM3"
#  elif defined(arm6)
#    define PCPU "ARM6"
#  elif defined(arm60)
#    define PCPU "ARM60"
#  elif defined(arm600)
#    define PCPU "ARM600"
#  elif defined(arm610)
#    define PCPU "ARM610"
#  elif defined(arm620)
#    define PCPU "ARM620"
#  elif defined(arm7)
#    define PCPU "ARM7"
#  elif defined(arm7d)
#    define PCPU "ARM7D"
#  elif defined(arm7di)
#    define PCPU "ARM7DI"
#  elif defined(arm70)
#    define PCPU "ARM70"
#  elif defined(arm700)
#    define PCPU "ARM700"
#  elif defined(arm700i)
#    define PCPU "ARM700I"
#  elif defined(arm710)
#    define PCPU "ARM710"
#  elif defined(arm720)
#    define PCPU "ARM720"
#  elif defined(arm710c)
#    define PCPU "ARM710C"
#  elif defined(arm7100)
#    define PCPU "ARM7100"
#  elif defined(arm7500)
#    define PCPU "ARM7500"
#  elif defined(arm7500fe)
#    define PCPU "ARM7500FE"
#  elif defined(arm7m)
#    define PCPU "ARM7M"
#  elif defined(arm7dm)
#    define PCPU "ARM7DM"
#  elif defined(arm7dmi)
#    define PCPU "ARM7DMI"
#  elif defined(arm8)
#    define PCPU "ARM8"
#  elif defined(arm810)
#    define PCPU "ARM810"
#  elif defined(strongarm)
#    define PCPU "StrongARM"
#  elif defined(strongarm110)
#    define PCPU "StrongARM110"
#  elif defined(strongarm1100)
#    define PCPU "StrongARM1100"
#  elif defined(strongarm1110)
#    define PCPU "StrongARM1110"
#  elif defined(arm7tdmi)
#    define PCPU "ARM7TDMI"
#  elif defined(arm7tdmis)
#    define PCPU "ARM7TDMI-S"
#  elif defined(arm710t)
#    define PCPU "ARM710T"
#  elif defined(arm720t)
#    define PCPU "ARM720T"
#  elif defined(arm740t)
#    define PCPU "ARM740T"
#  elif defined(arm9)
#    define PCPU "ARM9"
#  elif defined(arm9tdmi)
#    define PCPU "ARM9TDMI"
#  elif defined(arm920)
#    define PCPU "ARM920"
#  elif defined(arm920t)
#    define PCPU "ARM920T"
#  elif defined(arm922t)
#    define PCPU "ARM922T"
#  elif defined(arm940t)
#    define PCPU "ARM940T"
#  elif defined(ep9312)
#    define PCPU "EP9312"
#  elif defined(arm10tdmi)
#    define PCPU "ARM10TDMI"
#  elif defined(arm1020t)
#    define PCPU "ARM1020T"
#  elif defined(arm9e)
#    define PCPU "ARM9E"
#  elif defined(arm946es)
#    define PCPU "ARM946E-S"
#  elif defined(arm966es)
#    define PCPU "ARM966E-S"
#  elif defined(arm968es)
#    define PCPU "ARM968E-S"
#  elif defined(arm10e)
#    define PCPU "ARM10E"
#  elif defined(arm1020e)
#    define PCPU "ARM1020E"
#  elif defined(arm1022e)
#    define PCPU "ARM1022E"
#  elif defined(xscale)
#    define PCPU "XSCALE"
#  elif defined(iwmmxt)
#    define PCPU "IWMMXT"
#  elif defined(iwmmxt2)
#    define PCPU "IWMMXT2"
#  elif defined(arm926ejs)
#    define PCPU "ARM926EJ-S"
#  elif defined(arm1026ejs)
#    define PCPU "ARM1026EJ-S"
#  elif defined(arm1136js)
#    define PCPU "ARM1136J-S"
#  elif defined(arm1136jfs)
#    define PCPU "ARM1136JF-S"
#  elif defined(arm1176jzs)
#    define PCPU "ARM1176JZ-S"
#  elif defined(arm1176jzfs)
#    define PCPU "ARM1176JZF-S"
#  elif defined(mpcorenovfp)
#    define PCPU "MPCORENOVFP"
#  elif defined(mpcore)
#    define PCPU "MPCORE"
#  elif defined(arm1156t2s)
#    define PCPU "ARM1156T2-S"
#  elif defined(arm1156t2fs)
#    define PCPU "ARM1156T2F-S"
#  elif defined(cortexa5)
#    define PCPU "CORTEX-A5"
#  elif defined(cortexa8)
#    define PCPU "CORTEX-A8"
#  elif defined(cortexa15)
#    define PCPU "CORTEX-A15"
#  elif defined(cortexr4)
#    define PCPU "CORTEX-R4"
#  elif defined(cortexr4f)
#    define PCPU "CORTEX-R4F"
#  elif defined(cortexm4)
#    define PCPU "CORTEX-M4"
#  elif defined(cortexm3)
#    define PCPU "CORTEX-M3"
#  elif defined(cortexm1)
#    define PCPU "CORTEX-M1"
#  elif defined(cortexm0)
#    define PCPU "CORTEX-M0"
#  else

/* find out by generic cpu defines what we are compiling for */

#    if defined(__MAVERICK__)
#      define PCPU "Maverick"
#    elif defined(__XSCALE__)
#      define PCPU "XSCALE"
#    elif defined(__IWMMXT__)
#      define PCPU "IWMMXT"
#    elif defined(__ARM_NEON__)
#      define PCPU "NEON"
#    elif defined(__thumb__)
#      define PCPU "Thumb"
#    elif defined(__thumb2__)
#      define PCPU "Thumb2"
#    else

/* Unknown cpu, so handle as plain ARM */
#      define PCPU "ARM"
#    endif
#  endif
#  define PLATFORM_CPU PCPU PLATFORM_ENDIAN
#endif


/* Generic avr32 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__avr32__)
#define PLATFORM_CPU "AVR32"
#endif


/* Generic bfin cpu discovery */
#if !defined(PLATFORM_CPU) && defined(BFIN)
#if defined(__ADSPBF512__)
#define PLATFORM_CPU "BFIN512"
#elif defined(__ADSPBF514__)
#define PLATFORM_CPU "BFIN514"
#elif defined(__ADSPBF516__)
#define PLATFORM_CPU "BFIN516"
#elif defined(__ADSPBF518__)
#define PLATFORM_CPU "BFIN518"
#elif defined(__ADSPBF522__)
#define PLATFORM_CPU "BFIN522"
#elif defined(__ADSPBF523__)
#define PLATFORM_CPU "BFIN523"
#elif defined(__ADSPBF524__)
#define PLATFORM_CPU "BFIN524"
#elif defined(__ADSPBF525__)
#define PLATFORM_CPU "BFIN525"
#elif defined(__ADSPBF526__)
#define PLATFORM_CPU "BFIN526"
#elif defined(__ADSPBF527__)
#define PLATFORM_CPU "BFIN527"
#elif defined(__ADSPBF531__)
#define PLATFORM_CPU "BFIN531"
#elif defined(__ADSPBF532__)
#define PLATFORM_CPU "BFIN532"
#elif defined(__ADSPBF533__)
#define PLATFORM_CPU "BFIN533"
#elif defined(__ADSPBF534__)
#define PLATFORM_CPU "BFIN534"
#elif defined(__ADSPBF536__)
#define PLATFORM_CPU "BFIN536"
#elif defined(__ADSPBF537__)
#define PLATFORM_CPU "BFIN537"
#elif defined(__ADSPBF538__)
#define PLATFORM_CPU "BFIN538"
#elif defined(__ADSPBF539__)
#define PLATFORM_CPU "BFIN539"
#elif defined(__ADSPBF542M__)
#define PLATFORM_CPU "BFIN542M"
#elif defined(__ADSPBF542__)
#define PLATFORM_CPU "BFIN542"
#elif defined(__ADSPBF544M__)
#define PLATFORM_CPU "BFIN544M"
#elif defined(__ADSPBF544__)
#define PLATFORM_CPU "BFIN544"
#elif defined(__ADSPBF547M__)
#define PLATFORM_CPU "BFIN547M"
#elif defined(__ADSPBF547__)
#define PLATFORM_CPU "BFIN547"
#elif defined(__ADSPBF548M__)
#define PLATFORM_CPU "BFIN548M"
#elif defined(__ADSPBF548__)
#define PLATFORM_CPU "BFIN548"
#elif defined(__ADSPBF549M__)
#define PLATFORM_CPU "BFIN549M"
#elif defined(__ADSPBF549__)
#define PLATFORM_CPU "BFIN549"
#elif defined(__ADSPBF561__)
#define PLATFORM_CPU "BFIN561"
#else
#define PLATFORM_CPU "BFIN"
#endif
#endif


/* Generic cris cpu discovery */
#if !defined(PLATFORM_CPU) && defined(CRIS)

#endif

/* Generic hppa cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__hppa__)
#defined PLATFORM_CPU "HPPA"
#endif

/* Generic ia64 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__ia64__)
#define PLATFORM_CPU "IA64"
#endif


/* Generic m68k cpu discovery */
#if (defined(__mc68060__) || defined(__mc68040__) || defined(__mc68030__) || defined(__mc68020__) || defined(__mc68010__) || defined(__mc68000__)) && !defined(__m68k__)
#define __m68k__
#endif

#if !defined(PLATFORM_CPU) && !defined(FIND_M68K_CPU) && defined(__m68k__)
#define FIND_M68K_CPU
#endif

#if !defined(PLATFORM_CPU) && defined(FIND_M68K_CPU)

#ifdef __mc68060__
#define PLATFORM_CPU "68060"
#endif

#if !defined(PLATFORM_CPU) && defined(__mc68040__)
#define PLATFORM_CPU "68040"
#endif

#if !defined(PLATFORM_CPU) && defined(__mc68030__)
#define PLATFORM_CPU "68030"
#endif

#if !defined(PLATFORM_CPU) && defined(__mc68020__)
#define PLATFORM_CPU "68020"
#endif

#if !defined(PLATFORM_CPU) && defined(__mc68010__)
#define PLATFORM_CPU "68010"
#endif

#if !defined(PLATFORM_CPU) && defined(__mc68000__)
#define PLATFORM_CPU "68000"
#endif

#ifndef PLATFORM_CPU
#define PLATFORM_CPU "M68K"
#endif

#endif


/* Generic mips cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__mips__) && !defined(__mips64__)
#  ifdef WORDS_BIGENDIAN
#    define PLATFORM_CPU "MIPS (big endian)"
#  else
#    define PLATFORM_CPU "MIPS (little endian)"
#  endif
#endif


/* Generic mips64 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__mips64__)
#  ifdef WORDS_BIGENDIAN
#    define PLATFORM_CPU "MIPS64 (big endian)"
#  else
#    define PLATFORM_CPU "MIPS64 (little endian)"
#  endif
#endif


/* Generic ns32k cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__ns32000__)
#define PLATFORM_CPU_"NS32K"
#endif


/* Generic powerpc cpu discovery */
#if !defined(PLATFORM_CPU) && (defined(__powerpc__) || defined(__ppc__)) && !defined(__powerpc64__)
#define PLATFORM_CPU "PPC"
#endif


/* Generic powerpc64 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__powerpc64__)
#define PLATFORM_CPU "PPC64"
#endif


/* Generic s390 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__s390__) && !defined(__s390x__)
#define PLATFORM_CPU "S390"
#endif

#if !defined(PLATFORM_CPU) && defined(__s390x__)
#define PLATFORM_CPU "S390x"
#endif


/* Generic sh cpu descovery */
#if !defined(PLATFORM_CPU) && defined(__sh3__)
#  ifdef WORDS_BIGENDIAN
#    define PLATFORM_CPU "SH3 (big endian)"
#  else
#    define PLATFORM_CPU "SH3 (little endian)"
#  endif
#endif

#if !defined(PLATFORM_CPU) && defined(__SH4__)
#define PLATFORM_CPU "SH4"
#endif


/* Generic sparc64 cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__sparc64__)
#define PLATFORM_CPU "SPARC64"
#endif


/* Generic sparc cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__sparc__)
#define PLATFORM_CPU "SPARC"
#endif


/* Generic vax cpu discovery */
#if !defined(PLATFORM_CPU) && defined(__vax__)
#define PLATFORM_CPU "VAX"
#endif

/* Generic x86 cpu discovery */
#if !defined(PLATFORM_CPU) && !defined(FIND_X86_CPU) && (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)) && !defined(__amd64__) && !defined(__x86_64__)
#define FIND_X86_CPU
#endif

#if !defined(PLATFORM_CPU) && defined(FIND_X86_CPU)

#ifdef __i686__
#define PLATFORM_CPU "Pentium Pro"
#endif

#if !defined(PLATFORM_CPU) && defined(__i586__)
#define PLATFORM_CPU "Pentium"
#endif

#if !defined(PLATFORM_CPU) && defined(__i486__)
#define PLATFORM_CPU "80486"
#endif

#if !defined(PLATFORM_CPU) && defined(__i386__)
#define PLATFORM_CPU "80386"
#endif

#ifndef PLATFORM_CPU
#define PLATFORM_CPU "Unknown intel x86 compatible"
#endif

#endif

#endif // VICE_PLATFORM_CPU_TYPE_H
