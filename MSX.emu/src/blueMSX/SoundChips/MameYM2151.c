/*****************************************************************************
*
*	Yamaha YM2151 driver (version 2.150 final beta)
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "MameYM2151.h"
#include "SaveState.h"


/* struct describing a single operator */
typedef struct
{
	UInt32		phase;					/* accumulated operator phase */
	UInt32		freq;					/* operator frequency count */
	Int32		dt1;					/* current DT1 (detune 1 phase inc/decrement) value */
	UInt32		mul;					/* frequency count multiply */
	UInt32		dt1_i;					/* DT1 index * 32 */
	UInt32		dt2;					/* current DT2 (detune 2) value */

	Int32		mem_value;				/* delayed sample (MEM) value */

	/* channel specific data; note: each operator number 0 contains channel specific data */
	UInt32		fb_shift;				/* feedback shift value for operators 0 in each channel */
	Int32		fb_out_curr;			/* operator feedback value (used only by operators 0) */
	Int32		fb_out_prev;			/* previous feedback value (used only by operators 0) */
	UInt32		kc;						/* channel KC (copied to all operators) */
	UInt32		kc_i;					/* just for speedup */
	UInt32		pms;					/* channel PMS */
	UInt32		ams;					/* channel AMS */
	/* end of channel specific data */

	UInt32		AMmask;					/* LFO Amplitude Modulation enable mask */
	UInt32		state;					/* Envelope state: 4-attack(AR) 3-decay(D1R) 2-sustain(D2R) 1-release(RR) 0-off */
	UInt8		eg_sh_ar;				/*  (attack state) */
	UInt8		eg_sel_ar;				/*  (attack state) */
	UInt32		tl;						/* Total attenuation Level */
	Int32		volume;					/* current envelope attenuation level */
	UInt8		eg_sh_d1r;				/*  (decay state) */
	UInt8		eg_sel_d1r;				/*  (decay state) */
	UInt32		d1l;					/* envelope switches to sustain state after reaching this level */
	UInt8		eg_sh_d2r;				/*  (sustain state) */
	UInt8		eg_sel_d2r;				/*  (sustain state) */
	UInt8		eg_sh_rr;				/*  (release state) */
	UInt8		eg_sel_rr;				/*  (release state) */

	UInt32		key;					/* 0=last key was KEY OFF, 1=last key was KEY ON */

	UInt32		ks;						/* key scale    */
	UInt32		ar;						/* attack rate  */
	UInt32		d1r;					/* decay rate   */
	UInt32		d2r;					/* sustain rate */
	UInt32		rr;						/* release rate */

	signed int *connect;				/* operator output 'direction' */
	signed int *mem_connect;			/* where to put the delayed sample (MEM) */

	UInt32		reserved0;				/**/
	UInt32		reserved1;				/**/
} YM2151Operator;

struct MameYm2151
{
    void* ref;

	YM2151Operator	oper[32];			/* the 32 operators */

	UInt32		pan[16];				/* channels output masks (0xffffffff = enable) */

	UInt32		eg_cnt;					/* global envelope generator counter */
	UInt32		eg_timer;				/* global envelope generator counter works at frequency = chipclock/64/3 */
	UInt32		eg_timer_add;			/* step of eg_timer */
	UInt32		eg_timer_overflow;		/* envelope generator timer overlfows every 3 samples (on real chip) */

	UInt32		lfo_phase;				/* accumulated LFO phase (0 to 255) */
	UInt32		lfo_timer;				/* LFO timer						*/
	UInt32		lfo_timer_add;			/* step of lfo_timer				*/
	UInt32		lfo_overflow;			/* LFO generates new output when lfo_timer reaches this value */
	UInt32		lfo_counter;			/* LFO phase increment counter		*/
	UInt32		lfo_counter_add;		/* step of lfo_counter				*/
	UInt8		lfo_wsel;				/* LFO waveform (0-saw, 1-square, 2-triangle, 3-random noise) */
	UInt8		amd;					/* LFO Amplitude Modulation Depth	*/
	Int8		pmd;					/* LFO Phase Modulation Depth		*/
	UInt32		lfa;					/* LFO current AM output			*/
	Int32		lfp;					/* LFO current PM output			*/

	UInt8		test;					/* TEST register */
	UInt8		ct;						/* output control pins (bit1-CT2, bit0-CT1) */

	UInt32		noise;					/* noise enable/period register (bit 7 - noise enable, bits 4-0 - noise period */
	UInt32		noise_rng;				/* 17 bit noise shift register */
	UInt32		noise_p;				/* current noise 'phase'*/
	UInt32		noise_f;				/* current noise period */

	UInt32		csm_req;				/* CSM  KEY ON / KEY OFF sequence request */

	UInt32		irq_enable;				/* IRQ enable for timer B (bit 3) and timer A (bit 2); bit 7 - CSM mode (keyon to all slots, everytime timer A overflows) */
	UInt32		status;					/* chip status (BUSY, IRQ Flags) */
	UInt8		connect[8];				/* channels connections */

    UInt16      timer_A_val;

	/*	Frequency-deltas to get the closest frequency possible.
	*	There are 11 octaves because of DT2 (max 950 cents over base frequency)
	*	and LFO phase modulation (max 800 cents below AND over base frequency)
	*	Summary:   octave  explanation
	*              0       note code - LFO PM
	*              1       note code
	*              2       note code
	*              3       note code
	*              4       note code
	*              5       note code
	*              6       note code
	*              7       note code
	*              8       note code
	*              9       note code + DT2 + LFO PM
	*              10      note code + DT2 + LFO PM
	*/
	UInt32		freq[11*768];			/* 11 octaves, 768 'cents' per octave */    // No Save

	/*	Frequency deltas for DT1. These deltas alter operator frequency
	*	after it has been taken from frequency-deltas table.
	*/
	Int32		dt1_freq[8*32];			/* 8 DT1 levels, 32 KC values */              // No Save
	UInt32		noise_tab[32];			/* 17bit Noise Generator periods */           // No Save

	unsigned int clock;					/* chip clock in Hz (passed from 2151intf.c) */
	unsigned int sampfreq;				/* sampling frequency in Hz (passed from 2151intf.c) */

    signed int chanout[8];
    signed int m2,c1,c2; /* Phase Modulation input for operators 2,3,4 */
    signed int mem;		/* one sample delay memory */

};

extern void	ym2151TimerSet(void* ref, int timer, int count);
extern void	ym2151TimerStart(void* ref, int timer, int start);
extern void	ym2151Irq(void* ref, int irq);
extern void ym2151WritePortCallback(void* ref, UInt32 port, UInt8 value);

#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
#define LFO_SH			10  /* 22.10 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)

#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)

#define MAX_ATT_INDEX	(ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX	(0)			/* 0 */

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */


#define FINAL_SH	(0)
#define MAXOUT		(+32767)
#define MINOUT		(-32768)


/*	TL_TAB_LEN is calculated as:
*	13 - sinus amplitude bits     (Y axis)
*	2  - sinus sign bit           (Y axis)
*	TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
static signed int tl_tab[TL_TAB_LEN];

#define ENV_QUIET		(TL_TAB_LEN>>3)

/* sin waveform table in 'decibel' scale */
static unsigned int sin_tab[SIN_LEN];


/* translate from D1L to volume index (16 D1L levels) */
static UInt32 d1l_tab[16];


#define RATE_STEPS (8)
static UInt8 eg_inc[19*RATE_STEPS]={

/*cycle:0 1  2 3  4 5  6 7*/

/* 0 */ 0,1, 0,1, 0,1, 0,1, /* rates 00..11 0 (increment by 0 or 1) */
/* 1 */ 0,1, 0,1, 1,1, 0,1, /* rates 00..11 1 */
/* 2 */ 0,1, 1,1, 0,1, 1,1, /* rates 00..11 2 */
/* 3 */ 0,1, 1,1, 1,1, 1,1, /* rates 00..11 3 */

/* 4 */ 1,1, 1,1, 1,1, 1,1, /* rate 12 0 (increment by 1) */
/* 5 */ 1,1, 1,2, 1,1, 1,2, /* rate 12 1 */
/* 6 */ 1,2, 1,2, 1,2, 1,2, /* rate 12 2 */
/* 7 */ 1,2, 2,2, 1,2, 2,2, /* rate 12 3 */

/* 8 */ 2,2, 2,2, 2,2, 2,2, /* rate 13 0 (increment by 2) */
/* 9 */ 2,2, 2,4, 2,2, 2,4, /* rate 13 1 */
/*10 */ 2,4, 2,4, 2,4, 2,4, /* rate 13 2 */
/*11 */ 2,4, 4,4, 2,4, 4,4, /* rate 13 3 */

/*12 */ 4,4, 4,4, 4,4, 4,4, /* rate 14 0 (increment by 4) */
/*13 */ 4,4, 4,8, 4,4, 4,8, /* rate 14 1 */
/*14 */ 4,8, 4,8, 4,8, 4,8, /* rate 14 2 */
/*15 */ 4,8, 8,8, 4,8, 8,8, /* rate 14 3 */

/*16 */ 8,8, 8,8, 8,8, 8,8, /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
/*17 */ 16,16,16,16,16,16,16,16, /* rates 15 2, 15 3 for attack */
/*18 */ 0,0, 0,0, 0,0, 0,0, /* infinity rates for attack and decay(s) */
};


#define O(a) (a*RATE_STEPS)

/*note that there is no O(17) in this table - it's directly in the code */
static UInt8 eg_rate_select[32+64+32]={	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
/* 32 dummy (infinite time) rates */
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),

/* rates 00-11 */
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),

/* rate 12 */
O( 4),O( 5),O( 6),O( 7),

/* rate 13 */
O( 8),O( 9),O(10),O(11),

/* rate 14 */
O(12),O(13),O(14),O(15),

/* rate 15 */
O(16),O(16),O(16),O(16),

/* 32 dummy rates (same as 15 3) */
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16)

};
#undef O

/*rate  0,    1,    2,   3,   4,   5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15*/
/*shift 11,   10,   9,   8,   7,   6,  5,  4,  3,  2, 1,  0,  0,  0,  0,  0 */
/*mask  2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3, 1,  0,  0,  0,  0,  0 */

#define O(a) (a*1)
static UInt8 eg_rate_shift[32+64+32]={	/* Envelope Generator counter shifts (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),


/* rates 00-11 */
O(11),O(11),O(11),O(11),
O(10),O(10),O(10),O(10),
O( 9),O( 9),O( 9),O( 9),
O( 8),O( 8),O( 8),O( 8),
O( 7),O( 7),O( 7),O( 7),
O( 6),O( 6),O( 6),O( 6),
O( 5),O( 5),O( 5),O( 5),
O( 4),O( 4),O( 4),O( 4),
O( 3),O( 3),O( 3),O( 3),
O( 2),O( 2),O( 2),O( 2),
O( 1),O( 1),O( 1),O( 1),
O( 0),O( 0),O( 0),O( 0),

/* rate 12 */
O( 0),O( 0),O( 0),O( 0),

/* rate 13 */
O( 0),O( 0),O( 0),O( 0),

/* rate 14 */
O( 0),O( 0),O( 0),O( 0),

/* rate 15 */
O( 0),O( 0),O( 0),O( 0),

/* 32 dummy rates (same as 15 3) */
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0)

};
#undef O

/*	DT2 defines offset in cents from base note
*
*	This table defines offset in frequency-deltas table.
*	User's Manual page 22
*
*	Values below were calculated using formula: value =  orig.val / 1.5625
*
*	DT2=0 DT2=1 DT2=2 DT2=3
*	0     600   781   950
*/
static UInt32 dt2_tab[4] = { 0, 384, 500, 608 };

/*	DT1 defines offset in Hertz from base note
*	This table is converted while initialization...
*	Detune table shown in YM2151 User's Manual is wrong (verified on the real chip)
*/

static UInt8 dt1_tab[4*32] = { /* 4*32 DT1 values */
/* DT1=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

/* DT1=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,

/* DT1=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,

/* DT1=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

static UInt16 phaseinc_rom[768]={
1299,1300,1301,1302,1303,1304,1305,1306,1308,1309,1310,1311,1313,1314,1315,1316,
1318,1319,1320,1321,1322,1323,1324,1325,1327,1328,1329,1330,1332,1333,1334,1335,
1337,1338,1339,1340,1341,1342,1343,1344,1346,1347,1348,1349,1351,1352,1353,1354,
1356,1357,1358,1359,1361,1362,1363,1364,1366,1367,1368,1369,1371,1372,1373,1374,
1376,1377,1378,1379,1381,1382,1383,1384,1386,1387,1388,1389,1391,1392,1393,1394,
1396,1397,1398,1399,1401,1402,1403,1404,1406,1407,1408,1409,1411,1412,1413,1414,
1416,1417,1418,1419,1421,1422,1423,1424,1426,1427,1429,1430,1431,1432,1434,1435,
1437,1438,1439,1440,1442,1443,1444,1445,1447,1448,1449,1450,1452,1453,1454,1455,
1458,1459,1460,1461,1463,1464,1465,1466,1468,1469,1471,1472,1473,1474,1476,1477,
1479,1480,1481,1482,1484,1485,1486,1487,1489,1490,1492,1493,1494,1495,1497,1498,
1501,1502,1503,1504,1506,1507,1509,1510,1512,1513,1514,1515,1517,1518,1520,1521,
1523,1524,1525,1526,1528,1529,1531,1532,1534,1535,1536,1537,1539,1540,1542,1543,
1545,1546,1547,1548,1550,1551,1553,1554,1556,1557,1558,1559,1561,1562,1564,1565,
1567,1568,1569,1570,1572,1573,1575,1576,1578,1579,1580,1581,1583,1584,1586,1587,
1590,1591,1592,1593,1595,1596,1598,1599,1601,1602,1604,1605,1607,1608,1609,1610,
1613,1614,1615,1616,1618,1619,1621,1622,1624,1625,1627,1628,1630,1631,1632,1633,
1637,1638,1639,1640,1642,1643,1645,1646,1648,1649,1651,1652,1654,1655,1656,1657,
1660,1661,1663,1664,1666,1667,1669,1670,1672,1673,1675,1676,1678,1679,1681,1682,
1685,1686,1688,1689,1691,1692,1694,1695,1697,1698,1700,1701,1703,1704,1706,1707,
1709,1710,1712,1713,1715,1716,1718,1719,1721,1722,1724,1725,1727,1728,1730,1731,
1734,1735,1737,1738,1740,1741,1743,1744,1746,1748,1749,1751,1752,1754,1755,1757,
1759,1760,1762,1763,1765,1766,1768,1769,1771,1773,1774,1776,1777,1779,1780,1782,
1785,1786,1788,1789,1791,1793,1794,1796,1798,1799,1801,1802,1804,1806,1807,1809,
1811,1812,1814,1815,1817,1819,1820,1822,1824,1825,1827,1828,1830,1832,1833,1835,
1837,1838,1840,1841,1843,1845,1846,1848,1850,1851,1853,1854,1856,1858,1859,1861,
1864,1865,1867,1868,1870,1872,1873,1875,1877,1879,1880,1882,1884,1885,1887,1888,
1891,1892,1894,1895,1897,1899,1900,1902,1904,1906,1907,1909,1911,1912,1914,1915,
1918,1919,1921,1923,1925,1926,1928,1930,1932,1933,1935,1937,1939,1940,1942,1944,
1946,1947,1949,1951,1953,1954,1956,1958,1960,1961,1963,1965,1967,1968,1970,1972,
1975,1976,1978,1980,1982,1983,1985,1987,1989,1990,1992,1994,1996,1997,1999,2001,
2003,2004,2006,2008,2010,2011,2013,2015,2017,2019,2021,2022,2024,2026,2028,2029,
2032,2033,2035,2037,2039,2041,2043,2044,2047,2048,2050,2052,2054,2056,2058,2059,
2062,2063,2065,2067,2069,2071,2073,2074,2077,2078,2080,2082,2084,2086,2088,2089,
2092,2093,2095,2097,2099,2101,2103,2104,2107,2108,2110,2112,2114,2116,2118,2119,
2122,2123,2125,2127,2129,2131,2133,2134,2137,2139,2141,2142,2145,2146,2148,2150,
2153,2154,2156,2158,2160,2162,2164,2165,2168,2170,2172,2173,2176,2177,2179,2181,
2185,2186,2188,2190,2192,2194,2196,2197,2200,2202,2204,2205,2208,2209,2211,2213,
2216,2218,2220,2222,2223,2226,2227,2230,2232,2234,2236,2238,2239,2242,2243,2246,
2249,2251,2253,2255,2256,2259,2260,2263,2265,2267,2269,2271,2272,2275,2276,2279,
2281,2283,2285,2287,2288,2291,2292,2295,2297,2299,2301,2303,2304,2307,2308,2311,
2315,2317,2319,2321,2322,2325,2326,2329,2331,2333,2335,2337,2338,2341,2342,2345,
2348,2350,2352,2354,2355,2358,2359,2362,2364,2366,2368,2370,2371,2374,2375,2378,
2382,2384,2386,2388,2389,2392,2393,2396,2398,2400,2402,2404,2407,2410,2411,2414,
2417,2419,2421,2423,2424,2427,2428,2431,2433,2435,2437,2439,2442,2445,2446,2449,
2452,2454,2456,2458,2459,2462,2463,2466,2468,2470,2472,2474,2477,2480,2481,2484,
2488,2490,2492,2494,2495,2498,2499,2502,2504,2506,2508,2510,2513,2516,2517,2520,
2524,2526,2528,2530,2531,2534,2535,2538,2540,2542,2544,2546,2549,2552,2553,2556,
2561,2563,2565,2567,2568,2571,2572,2575,2577,2579,2581,2583,2586,2589,2590,2593
};


/*
	Noise LFO waveform.

	Here are just 256 samples out of much longer data.

	It does NOT repeat every 256 samples on real chip and I wasnt able to find
	the point where it repeats (even in strings as long as 131072 samples).

	I only put it here because its better than nothing and perhaps
	someone might be able to figure out the real algorithm.


	Note that (due to the way the LFO output is calculated) it is quite
	possible that two values: 0x80 and 0x00 might be wrong in this table.
	To be exact:
		some 0x80 could be 0x81 as well as some 0x00 could be 0x01.
*/

static UInt8 lfo_noise_waveform[256] = {
0xFF,0xEE,0xD3,0x80,0x58,0xDA,0x7F,0x94,0x9E,0xE3,0xFA,0x00,0x4D,0xFA,0xFF,0x6A,
0x7A,0xDE,0x49,0xF6,0x00,0x33,0xBB,0x63,0x91,0x60,0x51,0xFF,0x00,0xD8,0x7F,0xDE,
0xDC,0x73,0x21,0x85,0xB2,0x9C,0x5D,0x24,0xCD,0x91,0x9E,0x76,0x7F,0x20,0xFB,0xF3,
0x00,0xA6,0x3E,0x42,0x27,0x69,0xAE,0x33,0x45,0x44,0x11,0x41,0x72,0x73,0xDF,0xA2,

0x32,0xBD,0x7E,0xA8,0x13,0xEB,0xD3,0x15,0xDD,0xFB,0xC9,0x9D,0x61,0x2F,0xBE,0x9D,
0x23,0x65,0x51,0x6A,0x84,0xF9,0xC9,0xD7,0x23,0xBF,0x65,0x19,0xDC,0x03,0xF3,0x24,
0x33,0xB6,0x1E,0x57,0x5C,0xAC,0x25,0x89,0x4D,0xC5,0x9C,0x99,0x15,0x07,0xCF,0xBA,
0xC5,0x9B,0x15,0x4D,0x8D,0x2A,0x1E,0x1F,0xEA,0x2B,0x2F,0x64,0xA9,0x50,0x3D,0xAB,

0x50,0x77,0xE9,0xC0,0xAC,0x6D,0x3F,0xCA,0xCF,0x71,0x7D,0x80,0xA6,0xFD,0xFF,0xB5,
0xBD,0x6F,0x24,0x7B,0x00,0x99,0x5D,0xB1,0x48,0xB0,0x28,0x7F,0x80,0xEC,0xBF,0x6F,
0x6E,0x39,0x90,0x42,0xD9,0x4E,0x2E,0x12,0x66,0xC8,0xCF,0x3B,0x3F,0x10,0x7D,0x79,
0x00,0xD3,0x1F,0x21,0x93,0x34,0xD7,0x19,0x22,0xA2,0x08,0x20,0xB9,0xB9,0xEF,0x51,

0x99,0xDE,0xBF,0xD4,0x09,0x75,0xE9,0x8A,0xEE,0xFD,0xE4,0x4E,0x30,0x17,0xDF,0xCE,
0x11,0xB2,0x28,0x35,0xC2,0x7C,0x64,0xEB,0x91,0x5F,0x32,0x0C,0x6E,0x00,0xF9,0x92,
0x19,0xDB,0x8F,0xAB,0xAE,0xD6,0x12,0xC4,0x26,0x62,0xCE,0xCC,0x0A,0x03,0xE7,0xDD,
0xE2,0x4D,0x8A,0xA6,0x46,0x95,0x0F,0x8F,0xF5,0x15,0x97,0x32,0xD4,0x28,0x1E,0x55
};




/* these variables stay here for speedup purposes only */
static MameYm2151 * PSG;


/* own PI definition */
#ifdef PI
	#undef PI
#endif
#define PI 3.14159265358979323846



static void init_tables(void)
{
	signed int i,x,n;
	DoubleT o,m;

	for (x=0; x<TL_RES_LEN; x++)
	{
		m = (1<<16) / pow(2, (x+1) * (ENV_STEP/4.0) / 8.0);
		m = floor(m);

		/* we never reach (1<<16) here due to the (x+1) */
		/* result fits within 16 bits at maximum */

		n = (int)m;		/* 16 bits here */
		n >>= 4;		/* 12 bits here */
		if (n&1)		/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;
						/* 11 bits here (rounded) */
		n <<= 2;		/* 13 bits here (as in real chip) */
		tl_tab[ x*2 + 0 ] = n;
		tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

		for (i=1; i<13; i++)
		{
			tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
			tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
		}
	#if 0
		logerror("tl %04i", x*2);
		for (i=0; i<13; i++)
			logerror(", [%02i] %4i", i*2, tl_tab[ x*2 /*+1*/ + i*2*TL_RES_LEN ]);
		logerror("\n");
	#endif
	}
	/*logerror("TL_TAB_LEN = %i (%i bytes)\n",TL_TAB_LEN, (int)sizeof(tl_tab));*/
	/*logerror("ENV_QUIET= %i\n",ENV_QUIET );*/


	for (i=0; i<SIN_LEN; i++)
	{
		/* non-standard sinus */
		m = sin( ((i*2)+1) * PI / SIN_LEN ); /* verified on the real chip */

		/* we never reach zero here due to ((i*2)+1) */

		if (m>0.0)
			o = 8*log(1.0/m)/log(2);	/* convert to 'decibels' */
		else
			o = 8*log(-1.0/m)/log(2);	/* convert to 'decibels' */

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)						/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;

		sin_tab[ i ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("sin [0x%4x]= %4i (tl_tab value=%8x)\n", i, sin_tab[i],tl_tab[sin_tab[i]]);*/
	}


	/* calculate d1l_tab table */
	for (i=0; i<16; i++)
	{
		m = (UInt32)((i!=15 ? i : i+16) * (4.0/ENV_STEP));   /* every 3 'dB' except for all bits = 1 = 45+48 'dB' */
		d1l_tab[i] = (UInt32)(m);
		/*logerror("d1l_tab[%02x]=%08x\n",i,d1l_tab[i] );*/
	}
}


static void init_chip_tables(MameYm2151 *chip)
{
	int i,j;
	DoubleT mult,phaseinc,Hz;
	DoubleT scaler;

	scaler = ( (DoubleT)chip->clock / 64.0 ) / ( (DoubleT)chip->sampfreq );
	/*logerror("scaler    = %20.15f\n", scaler);*/


	/* this loop calculates Hertz values for notes from c-0 to b-7 */
	/* including 64 'cents' (100/64 that is 1.5625 of real cent) per note */
	/* i*100/64/1200 is equal to i/768 */

	/* real chip works with 10 bits fixed point values (10.10) */
	mult = (1<<(FREQ_SH-10)); /* -10 because phaseinc_rom table values are already in 10.10 format */

	for (i=0; i<768; i++)
	{
		/* 3.4375 Hz is note A; C# is 4 semitones higher */
		Hz = 1000;

		phaseinc = phaseinc_rom[i];	/* real chip phase increment */
		phaseinc *= scaler;			/* adjust */


		/* octave 2 - reference octave */
		chip->freq[ 768+2*768+i ] = ((int)(phaseinc*mult)) & 0xffffffc0; /* adjust to X.10 fixed point */
		/* octave 0 and octave 1 */
		for (j=0; j<2; j++)
		{
			chip->freq[768 + j*768 + i] = (chip->freq[ 768+2*768+i ] >> (2-j) ) & 0xffffffc0; /* adjust to X.10 fixed point */
		}
		/* octave 3 to 7 */
		for (j=3; j<8; j++)
		{
			chip->freq[768 + j*768 + i] = chip->freq[ 768+2*768+i ] << (j-2);
		}
	}

	/* octave -1 (all equal to: oct 0, _KC_00_, _KF_00_) */
	for (i=0; i<768; i++)
	{
		chip->freq[ 0*768 + i ] = chip->freq[1*768+0];
	}

	/* octave 8 and 9 (all equal to: oct 7, _KC_14_, _KF_63_) */
	for (j=8; j<10; j++)
	{
		for (i=0; i<768; i++)
		{
			chip->freq[768+ j*768 + i ] = chip->freq[768 + 8*768 -1];
		}
	}

	mult = (1<<FREQ_SH);
	for (j=0; j<4; j++)
	{
		for (i=0; i<32; i++)
		{
			Hz = ( (DoubleT)dt1_tab[j*32+i] * ((DoubleT)chip->clock/64.0) ) / (DoubleT)(1<<20);

			/*calculate phase increment*/
			phaseinc = (Hz*SIN_LEN) / (DoubleT)chip->sampfreq;

			/*positive and negative values*/
			chip->dt1_freq[ (j+0)*32 + i ] = (Int32)(phaseinc * mult);
			chip->dt1_freq[ (j+4)*32 + i ] = -chip->dt1_freq[ (j+0)*32 + i ];
		}
	}

    chip->timer_A_val = 0;

    /* calculate noise periods table */
	scaler = ( (DoubleT)chip->clock / 64.0 ) / ( (DoubleT)chip->sampfreq );
	for (i=0; i<32; i++)
	{
		j = (i!=31 ? i : 30);				/* rate 30 and 31 are the same */
		j = 32-j;
		j = (int)(65536.0 / (DoubleT)(j*32.0));	/* number of samples per one shift of the shift register */
		/*chip->noise_tab[i] = j * 64;*/	/* number of chip clock cycles per one shift */
		chip->noise_tab[i] = (UInt32)(j * 64 * scaler);
		/*logerror("noise_tab[%02x]=%08x\n", i, chip->noise_tab[i]);*/
	}
}

#define KEY_ON(op, key_set){									\
		if (!(op)->key)											\
		{														\
			(op)->phase = 0;			/* clear phase */		\
			(op)->state = EG_ATT;		/* KEY ON = attack */	\
			(op)->volume += (~(op)->volume *					\
                           (eg_inc[(op)->eg_sel_ar + ((PSG->eg_cnt>>(op)->eg_sh_ar)&7)])	\
                          ) >>4;								\
			if ((op)->volume <= MIN_ATT_INDEX)					\
			{													\
				(op)->volume = MIN_ATT_INDEX;					\
				(op)->state = EG_DEC;							\
			}													\
		}														\
		(op)->key |= key_set;									\
}

#define KEY_OFF(op, key_clr){									\
		if ((op)->key)											\
		{														\
			(op)->key &= key_clr;								\
			if (!(op)->key)										\
			{													\
				if ((op)->state>EG_REL)							\
					(op)->state = EG_REL;/* KEY OFF = release */\
			}													\
		}														\
}

static void envelope_KONKOFF(YM2151Operator * op, int v)
{
	if (v&0x08)	/* M1 */
		KEY_ON (op+0, 1)
	else
		KEY_OFF(op+0,~1)

	if (v&0x20)	/* M2 */
		KEY_ON (op+1, 1)
	else
		KEY_OFF(op+1,~1)

	if (v&0x10)	/* C1 */
		KEY_ON (op+2, 1)
	else
		KEY_OFF(op+2,~1)

	if (v&0x40)	/* C2 */
		KEY_ON (op+3, 1)
	else
		KEY_OFF(op+3,~1)
}

void YM2151TimerCallback(MameYm2151 *chip, int timer)
{
    if (timer == 0) {
        if (chip->irq_enable & 0x04)
	    {
            if ((chip->status & 3) == 0) ym2151Irq(chip->ref, 1);
		    chip->status |= 1;
	    }
	    if (chip->irq_enable & 0x80)
		    chip->csm_req = 2;		/* request KEY ON / KEY OFF sequence */
    }
    if (timer == 1) {
	    if (chip->irq_enable & 0x08)
	    {
            if ((chip->status & 3) == 0) ym2151Irq(chip->ref, 1);
		    chip->status |= 2;
	    }
    }
}


static void set_connect( MameYm2151 *chip, YM2151Operator *om1, int cha, int v)
{
	YM2151Operator *om2 = om1+1;
	YM2151Operator *oc1 = om1+2;

	/* set connect algorithm */

	/* MEM is simply one sample delay */

	switch( v&7 )
	{
	case 0:
		/* M1---C1---MEM---M2---C2---OUT */
		om1->connect = &chip->c1;
		oc1->connect = &chip->mem;
		om2->connect = &chip->c2;
		om1->mem_connect = &chip->m2;
		break;

	case 1:
		/* M1------+-MEM---M2---C2---OUT */
		/*      C1-+                     */
		om1->connect = &chip->mem;
		oc1->connect = &chip->mem;
		om2->connect = &chip->c2;
		om1->mem_connect = &chip->m2;
		break;

	case 2:
		/* M1-----------------+-C2---OUT */
		/*      C1---MEM---M2-+          */
		om1->connect = &chip->c2;
		oc1->connect = &chip->mem;
		om2->connect = &chip->c2;
		om1->mem_connect = &chip->m2;
		break;

	case 3:
		/* M1---C1---MEM------+-C2---OUT */
		/*                 M2-+          */
		om1->connect = &chip->c1;
		oc1->connect = &chip->mem;
		om2->connect = &chip->c2;
		om1->mem_connect = &chip->c2;
		break;

	case 4:
		/* M1---C1-+-OUT */
		/* M2---C2-+     */
		/* MEM: not used */
		om1->connect = &chip->c1;
		oc1->connect = &chip->chanout[cha];
		om2->connect = &chip->c2;
		om1->mem_connect = &chip->mem;	/* store it anywhere where it will not be used */
		break;

	case 5:
		/*    +----C1----+     */
		/* M1-+-MEM---M2-+-OUT */
		/*    +----C2----+     */
		om1->connect = 0;	/* special mark */
		oc1->connect = &chip->chanout[cha];
		om2->connect = &chip->chanout[cha];
		om1->mem_connect = &chip->m2;
		break;

	case 6:
		/* M1---C1-+     */
		/*      M2-+-OUT */
		/*      C2-+     */
		/* MEM: not used */
		om1->connect = &chip->c1;
		oc1->connect = &chip->chanout[cha];
		om2->connect = &chip->chanout[cha];
		om1->mem_connect = &chip->mem;	/* store it anywhere where it will not be used */
		break;

	case 7:
		/* M1-+     */
		/* C1-+-OUT */
		/* M2-+     */
		/* C2-+     */
		/* MEM: not used*/
		om1->connect = &chip->chanout[cha];
		oc1->connect = &chip->chanout[cha];
		om2->connect = &chip->chanout[cha];
		om1->mem_connect = &chip->mem;	/* store it anywhere where it will not be used */
		break;
	}
}


static void refresh_EG(YM2151Operator * op)
{
	UInt32 kc;
	UInt32 v;

	kc = op->kc;

	/* v = 32 + 2*RATE + RKS = max 126 */

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
	{
		op->eg_sh_ar  = eg_rate_shift [op->ar  + v ];
		op->eg_sel_ar = eg_rate_select[op->ar  + v ];
	}
	else
	{
		op->eg_sh_ar  = 0;
		op->eg_sel_ar = 17*RATE_STEPS;
	}
	op->eg_sh_d1r = eg_rate_shift [op->d1r + v];
	op->eg_sel_d1r= eg_rate_select[op->d1r + v];
	op->eg_sh_d2r = eg_rate_shift [op->d2r + v];
	op->eg_sel_d2r= eg_rate_select[op->d2r + v];
	op->eg_sh_rr  = eg_rate_shift [op->rr  + v];
	op->eg_sel_rr = eg_rate_select[op->rr  + v];


	op+=1;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
	{
		op->eg_sh_ar  = eg_rate_shift [op->ar  + v ];
		op->eg_sel_ar = eg_rate_select[op->ar  + v ];
	}
	else
	{
		op->eg_sh_ar  = 0;
		op->eg_sel_ar = 17*RATE_STEPS;
	}
	op->eg_sh_d1r = eg_rate_shift [op->d1r + v];
	op->eg_sel_d1r= eg_rate_select[op->d1r + v];
	op->eg_sh_d2r = eg_rate_shift [op->d2r + v];
	op->eg_sel_d2r= eg_rate_select[op->d2r + v];
	op->eg_sh_rr  = eg_rate_shift [op->rr  + v];
	op->eg_sel_rr = eg_rate_select[op->rr  + v];

	op+=1;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
	{
		op->eg_sh_ar  = eg_rate_shift [op->ar  + v ];
		op->eg_sel_ar = eg_rate_select[op->ar  + v ];
	}
	else
	{
		op->eg_sh_ar  = 0;
		op->eg_sel_ar = 17*RATE_STEPS;
	}
	op->eg_sh_d1r = eg_rate_shift [op->d1r + v];
	op->eg_sel_d1r= eg_rate_select[op->d1r + v];
	op->eg_sh_d2r = eg_rate_shift [op->d2r + v];
	op->eg_sel_d2r= eg_rate_select[op->d2r + v];
	op->eg_sh_rr  = eg_rate_shift [op->rr  + v];
	op->eg_sel_rr = eg_rate_select[op->rr  + v];

	op+=1;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
	{
		op->eg_sh_ar  = eg_rate_shift [op->ar  + v ];
		op->eg_sel_ar = eg_rate_select[op->ar  + v ];
	}
	else
	{
		op->eg_sh_ar  = 0;
		op->eg_sel_ar = 17*RATE_STEPS;
	}
	op->eg_sh_d1r = eg_rate_shift [op->d1r + v];
	op->eg_sel_d1r= eg_rate_select[op->d1r + v];
	op->eg_sh_d2r = eg_rate_shift [op->d2r + v];
	op->eg_sel_d2r= eg_rate_select[op->d2r + v];
	op->eg_sh_rr  = eg_rate_shift [op->rr  + v];
	op->eg_sel_rr = eg_rate_select[op->rr  + v];
}


/* write a register on YM2151 chip number 'n' */
void YM2151WriteReg(MameYm2151 *chip, int r, int v)
{
	YM2151Operator *op = &chip->oper[ (r&0x07)*4+((r&0x18)>>3) ];

	/* adjust bus to 8 bits */
	r &= 0xff;
	v &= 0xff;

	switch(r & 0xe0){
	case 0x00:
		switch(r){
		case 0x01:	/* LFO reset(bit 1), Test Register (other bits) */
			chip->test = v;
			if (v&2) chip->lfo_phase = 0;
			break;

		case 0x08:
			PSG = chip; /* PSG is used in KEY_ON macro */
			envelope_KONKOFF(&chip->oper[ (v&7)*4 ], v );
			break;

		case 0x0f:	/* noise mode enable, noise period */
			chip->noise = v;
			chip->noise_f = chip->noise_tab[ v & 0x1f ];
			break;

        case 0x10:
            chip->timer_A_val &= 0x03;
            chip->timer_A_val |= v << 2;
			ym2151TimerSet(chip->ref, 0, 1 * (1024 - chip->timer_A_val));
            break;

        case 0x11:
            chip->timer_A_val &= 0x03fc;
            chip->timer_A_val |= v & 3;
			ym2151TimerSet(chip->ref, 0, 1 * (1024 - chip->timer_A_val));
            break;

        case 0x12:
			ym2151TimerSet(chip->ref, 1, 16 * (256 - v));
            break;

        case 0x14:	/* CSM, irq flag reset, irq enable, timer start/stop */

			chip->irq_enable = v;	/* bit 3-timer B, bit 2-timer A, bit 7 - CSM */

			if (v&0x10)	/* reset timer A irq flag */
			{
				chip->status &= ~1;
                if ((chip->status & 3) == 0) ym2151Irq(chip->ref, 0);
			}

			if (v&0x20)	/* reset timer B irq flag */
			{
				chip->status &= ~2;
                if ((chip->status & 3) == 0) ym2151Irq(chip->ref, 0);
			}

			ym2151TimerStart(chip->ref, 0, v & 4);
			ym2151TimerStart(chip->ref, 1, v & 8);
			break;

		case 0x18:	/* LFO frequency */
			{
				chip->lfo_overflow    = ( 1 << ((15-(v>>4))+3) ) * (1<<LFO_SH);
				chip->lfo_counter_add = 0x10 + (v & 0x0f);
			}
			break;

		case 0x19:	/* PMD (bit 7==1) or AMD (bit 7==0) */
			if (v&0x80)
				chip->pmd = v & 0x7f;
			else
				chip->amd = v & 0x7f;
			break;

		case 0x1b:	/* CT2, CT1, LFO waveform */
			chip->ct = v >> 6;
			chip->lfo_wsel = v & 3;
            ym2151WritePortCallback(chip->ref, 0 , chip->ct);
			break;

		default:
			break;
		}
		break;

	case 0x20:
		op = &chip->oper[ (r&7) * 4 ];
		switch(r & 0x18){
		case 0x00:	/* RL enable, Feedback, Connection */
			op->fb_shift = ((v>>3)&7) ? ((v>>3)&7)+6:0;
			chip->pan[ (r&7)*2    ] = (v & 0x40) ? ~0 : 0;
			chip->pan[ (r&7)*2 +1 ] = (v & 0x80) ? ~0 : 0;
			chip->connect[r&7] = v&7;
			set_connect(chip, op, r&7, v&7);
			break;

		case 0x08:	/* Key Code */
			v &= 0x7f;
			if (v != op->kc)
			{
				UInt32 kc, kc_channel;

				kc_channel = (v - (v>>2))*64;
				kc_channel += 768;
				kc_channel |= (op->kc_i & 63);

				(op+0)->kc = v;
				(op+0)->kc_i = kc_channel;
				(op+1)->kc = v;
				(op+1)->kc_i = kc_channel;
				(op+2)->kc = v;
				(op+2)->kc_i = kc_channel;
				(op+3)->kc = v;
				(op+3)->kc_i = kc_channel;

				kc = v>>2;

				(op+0)->dt1 = chip->dt1_freq[ (op+0)->dt1_i + kc ];
				(op+0)->freq = ( (chip->freq[ kc_channel + (op+0)->dt2 ] + (op+0)->dt1) * (op+0)->mul ) >> 1;

				(op+1)->dt1 = chip->dt1_freq[ (op+1)->dt1_i + kc ];
				(op+1)->freq = ( (chip->freq[ kc_channel + (op+1)->dt2 ] + (op+1)->dt1) * (op+1)->mul ) >> 1;

				(op+2)->dt1 = chip->dt1_freq[ (op+2)->dt1_i + kc ];
				(op+2)->freq = ( (chip->freq[ kc_channel + (op+2)->dt2 ] + (op+2)->dt1) * (op+2)->mul ) >> 1;

				(op+3)->dt1 = chip->dt1_freq[ (op+3)->dt1_i + kc ];
				(op+3)->freq = ( (chip->freq[ kc_channel + (op+3)->dt2 ] + (op+3)->dt1) * (op+3)->mul ) >> 1;

				refresh_EG( op );
			}
			break;

		case 0x10:	/* Key Fraction */
			v >>= 2;
			if (v !=  (op->kc_i & 63))
			{
				UInt32 kc_channel;

				kc_channel = v;
				kc_channel |= (op->kc_i & ~63);

				(op+0)->kc_i = kc_channel;
				(op+1)->kc_i = kc_channel;
				(op+2)->kc_i = kc_channel;
				(op+3)->kc_i = kc_channel;

				(op+0)->freq = ( (chip->freq[ kc_channel + (op+0)->dt2 ] + (op+0)->dt1) * (op+0)->mul ) >> 1;
				(op+1)->freq = ( (chip->freq[ kc_channel + (op+1)->dt2 ] + (op+1)->dt1) * (op+1)->mul ) >> 1;
				(op+2)->freq = ( (chip->freq[ kc_channel + (op+2)->dt2 ] + (op+2)->dt1) * (op+2)->mul ) >> 1;
				(op+3)->freq = ( (chip->freq[ kc_channel + (op+3)->dt2 ] + (op+3)->dt1) * (op+3)->mul ) >> 1;
			}
			break;

		case 0x18:	/* PMS, AMS */
			op->pms = (v>>4) & 7;
			op->ams = (v & 3);
			break;
		}
		break;

	case 0x40:		/* DT1, MUL */
		{
			UInt32 olddt1_i = op->dt1_i;
			UInt32 oldmul = op->mul;

			op->dt1_i = (v&0x70)<<1;
			op->mul   = (v&0x0f) ? (v&0x0f)<<1: 1;

			if (olddt1_i != op->dt1_i)
				op->dt1 = chip->dt1_freq[ op->dt1_i + (op->kc>>2) ];

			if ( (olddt1_i != op->dt1_i) || (oldmul != op->mul) )
				op->freq = ( (chip->freq[ op->kc_i + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		}
		break;

	case 0x60:		/* TL */
		op->tl = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
		break;

	case 0x80:		/* KS, AR */
		{
			UInt32 oldks = op->ks;
			UInt32 oldar = op->ar;

			op->ks = 5-(v>>6);
			op->ar = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

			if ( (op->ar != oldar) || (op->ks != oldks) )
			{
				if ((op->ar + (op->kc>>op->ks)) < 32+62)
				{
					op->eg_sh_ar  = eg_rate_shift [op->ar  + (op->kc>>op->ks) ];
					op->eg_sel_ar = eg_rate_select[op->ar  + (op->kc>>op->ks) ];
				}
				else
				{
					op->eg_sh_ar  = 0;
					op->eg_sel_ar = 17*RATE_STEPS;
				}
			}

			if (op->ks != oldks)
			{
				op->eg_sh_d1r = eg_rate_shift [op->d1r + (op->kc>>op->ks) ];
				op->eg_sel_d1r= eg_rate_select[op->d1r + (op->kc>>op->ks) ];
				op->eg_sh_d2r = eg_rate_shift [op->d2r + (op->kc>>op->ks) ];
				op->eg_sel_d2r= eg_rate_select[op->d2r + (op->kc>>op->ks) ];
				op->eg_sh_rr  = eg_rate_shift [op->rr  + (op->kc>>op->ks) ];
				op->eg_sel_rr = eg_rate_select[op->rr  + (op->kc>>op->ks) ];
			}
		}
		break;

	case 0xa0:		/* LFO AM enable, D1R */
		op->AMmask = (v&0x80) ? ~0 : 0;
		op->d1r    = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
		op->eg_sh_d1r = eg_rate_shift [op->d1r + (op->kc>>op->ks) ];
		op->eg_sel_d1r= eg_rate_select[op->d1r + (op->kc>>op->ks) ];
		break;

	case 0xc0:		/* DT2, D2R */
		{
			UInt32 olddt2 = op->dt2;
			op->dt2 = dt2_tab[ v>>6 ];
			if (op->dt2 != olddt2)
				op->freq = ( (chip->freq[ op->kc_i + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		}
		op->d2r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
		op->eg_sh_d2r = eg_rate_shift [op->d2r + (op->kc>>op->ks) ];
		op->eg_sel_d2r= eg_rate_select[op->d2r + (op->kc>>op->ks) ];
		break;

	case 0xe0:		/* D1L, RR */
		op->d1l = d1l_tab[ v>>4 ];
		op->rr  = 34 + ((v&0x0f)<<2);
		op->eg_sh_rr  = eg_rate_shift [op->rr  + (op->kc>>op->ks) ];
		op->eg_sel_rr = eg_rate_select[op->rr  + (op->kc>>op->ks) ];
		break;
	}
}



int YM2151ReadStatus(MameYm2151 *chip)
{
	return chip->status;
}


static void ym2151_state_save_register( MameYm2151 *chip )
{
}

MameYm2151* YM2151Create(void* ref, int clock, int rate)
{
    MameYm2151 *chip = (MameYm2151 *)calloc(1, sizeof(MameYm2151));

    chip->ref = ref;

	init_tables();

	chip->clock = clock;
	/*rate = clock/64;*/
	chip->sampfreq = rate ? rate : 44100;	/* avoid division by 0 in init_chip_tables() */
	init_chip_tables(chip);

	chip->lfo_timer_add = (UInt32)((1<<LFO_SH) * (clock/64.0) / chip->sampfreq);

	chip->eg_timer_add  = (UInt32)((1<<EG_SH)  * (clock/64.0) / chip->sampfreq);
	chip->eg_timer_overflow = ( 3 ) * (1<<EG_SH);

	YM2151ResetChip(chip);

	return chip;
}

void YM2151Destroy(MameYm2151* chip)
{
    free(chip);
}


void YM2151ResetChip(MameYm2151 *chip)
{
	int i;


	/* initialize hardware registers */
	for (i=0; i<32; i++)
	{
		memset(&chip->oper[i],'\0',sizeof(YM2151Operator));
		chip->oper[i].volume = MAX_ATT_INDEX;
		chip->oper[i].kc_i = 768; // min kc_i value
	}

	chip->eg_timer = 0;
	chip->eg_cnt   = 0;

	chip->lfo_timer  = 0;
	chip->lfo_counter= 0;
	chip->lfo_phase  = 0;
	chip->lfo_wsel   = 0;
	chip->pmd = 0;
	chip->amd = 0;
	chip->lfa = 0;
	chip->lfp = 0;

	chip->test= 0;

	chip->irq_enable = 0;
	ym2151TimerStart(chip->ref, 0, 0);
	ym2151TimerStart(chip->ref, 1, 0);

	chip->noise     = 0;
	chip->noise_rng = 0;
	chip->noise_p   = 0;
	chip->noise_f   = chip->noise_tab[0];

	chip->csm_req	= 0;
	chip->status    = 0;

	YM2151WriteReg(chip, 0x1b, 0);	/* only because of CT1, CT2 output pins */
	YM2151WriteReg(chip, 0x18, 0);	/* set LFO frequency */
	for (i=0x20; i<0x100; i++)		/* set the operators */
	{
		YM2151WriteReg(chip, i, 0);
	}
}



static signed int op_calc(YM2151Operator * OP, unsigned int env, signed int pm)
{
	UInt32 p;


	p = (env<<3) + sin_tab[ ( ((signed int)((OP->phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;

	return tl_tab[p];
}

static signed int op_calc1(YM2151Operator * OP, unsigned int env, signed int pm)
{
	UInt32 p;
	Int32  i;


	i = (OP->phase & ~FREQ_MASK) + pm;

/*logerror("i=%08x (i>>16)&511=%8i phase=%i [pm=%08x] ",i, (i>>16)&511, OP->phase>>FREQ_SH, pm);*/

	p = (env<<3) + sin_tab[ (i>>FREQ_SH) & SIN_MASK];

/*logerror("(p&255=%i p>>8=%i) out= %i\n", p&255,p>>8, tl_tab[p&255]>>(p>>8) );*/

	if (p >= TL_TAB_LEN)
		return 0;

	return tl_tab[p];
}



#define volume_calc(OP) ((OP)->tl + ((UInt32)(OP)->volume) + (AM & (OP)->AMmask))

static void chan_calc(MameYm2151* chip, unsigned int chan)
{
	YM2151Operator *op;
	unsigned int env;
	UInt32 AM = 0;

	chip->m2 = chip->c1 = chip->c2 = chip->mem = 0;
	op = &PSG->oper[chan*4];	/* M1 */

	*op->mem_connect = op->mem_value;	/* restore delayed sample (MEM) value to m2 or c2 */

	if (op->ams)
		AM = PSG->lfa << (op->ams-1);
	env = volume_calc(op);
	{
		Int32 out = op->fb_out_prev + op->fb_out_curr;
		op->fb_out_prev = op->fb_out_curr;

		if (!op->connect)
			/* algorithm 5 */
			chip->mem = chip->c1 = chip->c2 = op->fb_out_prev;
		else
			/* other algorithms */
			*op->connect = op->fb_out_prev;

		op->fb_out_curr = 0;
		if (env < ENV_QUIET)
		{
			if (!op->fb_shift)
				out=0;
			op->fb_out_curr = op_calc1(op, env, (out<<op->fb_shift) );
		}
	}

	env = volume_calc(op+1);	/* M2 */
	if (env < ENV_QUIET)
		*(op+1)->connect += op_calc(op+1, env, chip->m2);

	env = volume_calc(op+2);	/* C1 */
	if (env < ENV_QUIET)
		*(op+2)->connect += op_calc(op+2, env, chip->c1);

	env = volume_calc(op+3);	/* C2 */
	if (env < ENV_QUIET)
		chip->chanout[chan]    += op_calc(op+3, env, chip->c2);

	/* M1 */
	op->mem_value = chip->mem;
}
static void chan7_calc(MameYm2151* chip)
{
	YM2151Operator *op;
	unsigned int env;
	UInt32 AM = 0;

	chip->m2 = chip->c1 = chip->c2 = chip->mem = 0;
	op = &PSG->oper[7*4];		/* M1 */

	*op->mem_connect = op->mem_value;	/* restore delayed sample (MEM) value to m2 or c2 */

	if (op->ams)
		AM = PSG->lfa << (op->ams-1);
	env = volume_calc(op);
	{
		Int32 out = op->fb_out_prev + op->fb_out_curr;
		op->fb_out_prev = op->fb_out_curr;

		if (!op->connect)
			/* algorithm 5 */
			chip->mem = chip->c1 = chip->c2 = op->fb_out_prev;
		else
			/* other algorithms */
			*op->connect = op->fb_out_prev;

		op->fb_out_curr = 0;
		if (env < ENV_QUIET)
		{
			if (!op->fb_shift)
				out=0;
			op->fb_out_curr = op_calc1(op, env, (out<<op->fb_shift) );
		}
	}

	env = volume_calc(op+1);	/* M2 */
	if (env < ENV_QUIET)
		*(op+1)->connect += op_calc(op+1, env, chip->m2);

	env = volume_calc(op+2);	/* C1 */
	if (env < ENV_QUIET)
		*(op+2)->connect += op_calc(op+2, env, chip->c1);

	env = volume_calc(op+3);	/* C2 */
	if (PSG->noise & 0x80)
	{
		UInt32 noiseout;

		noiseout = 0;
		if (env < 0x3ff)
			noiseout = (env ^ 0x3ff) * 2;	/* range of the YM2151 noise output is -2044 to 2040 */
		chip->chanout[7] += ((PSG->noise_rng&0x10000) ? noiseout: (UInt32)-(Int32)noiseout); /* bit 16 -> output */
	}
	else
	{
		if (env < ENV_QUIET)
			chip->chanout[7] += op_calc(op+3, env, chip->c2);
	}
	/* M1 */
	op->mem_value = chip->mem;
}






/*
The 'rate' is calculated from following formula (example on decay rate):
  rks = notecode after key scaling (a value from 0 to 31)
  DR = value written to the chip register
  rate = 2*DR + rks; (max rate = 2*31+31 = 93)
Four MSBs of the 'rate' above are the 'main' rate (from 00 to 15)
Two LSBs of the 'rate' above are the value 'x' (the shape type).
(eg. '11 2' means that 'rate' is 11*4+2=46)

NOTE: A 'sample' in the description below is actually 3 output samples,
thats because the Envelope Generator clock is equal to internal_clock/3.

Single '-' (minus) character in the diagrams below represents one sample
on the output; this is for rates 11 x (11 0, 11 1, 11 2 and 11 3)

these 'main' rates:
00 x: single '-' = 2048 samples; (ie. level can change every 2048 samples)
01 x: single '-' = 1024 samples;
02 x: single '-' = 512 samples;
03 x: single '-' = 256 samples;
04 x: single '-' = 128 samples;
05 x: single '-' = 64 samples;
06 x: single '-' = 32 samples;
07 x: single '-' = 16 samples;
08 x: single '-' = 8 samples;
09 x: single '-' = 4 samples;
10 x: single '-' = 2 samples;
11 x: single '-' = 1 sample; (ie. level can change every 1 sample)

Shapes for rates 11 x look like this:
rate:		step:
11 0        01234567

level:
0           --
1             --
2               --
3                 --

rate:		step:
11 1        01234567

level:
0           --
1             --
2               -
3                -
4                 --

rate:		step:
11 2        01234567

level:
0           --
1             -
2              -
3               --
4                 -
5                  -

rate:		step:
11 3        01234567

level:
0           --
1             -
2              -
3               -
4                -
5                 -
6                  -


For rates 12 x, 13 x, 14 x and 15 x output level changes on every
sample - this means that the waveform looks like this: (but the level
changes by different values on different steps)
12 3        01234567

0           -
2            -
4             -
8              -
10              -
12               -
14                -
18                 -
20                  -

Notes about the timing:
----------------------

1. Synchronism

Output level of each two (or more) voices running at the same 'main' rate
(eg 11 0 and 11 1 in the diagram below) will always be changing in sync,
even if there're started with some delay.

Note that, in the diagram below, the decay phase in channel 0 starts at
sample #2, while in channel 1 it starts at sample #6. Anyway, both channels
will always change their levels at exactly the same (following) samples.

(S - start point of this channel, A-attack phase, D-decay phase):

step:
01234567012345670123456

channel 0:
  --
 |  --
 |    -
 |     -
 |      --
 |        --
|           --
|             -
|              -
|               --
AADDDDDDDDDDDDDDDD
S

01234567012345670123456
channel 1:
      -
     | -
     |  --
     |    --
     |      --
     |        -
    |          -
    |           --
    |             --
    |               --
    AADDDDDDDDDDDDDDDD
    S
01234567012345670123456


2. Shifted (delayed) synchronism

Output of each two (or more) voices running at different 'main' rate
(9 1, 10 1 and 11 1 in the diagrams below) will always be changing
in 'delayed-sync' (even if there're started with some delay as in "1.")

Note that the shapes are delayed by exactly one sample per one 'main' rate
increment. (Normally one would expect them to start at the same samples.)

See diagram below (* - start point of the shape).

cycle:
0123456701234567012345670123456701234567012345670123456701234567

rate 09 1
*-------
        --------
                ----
                    ----
                        --------
                                *-------
                                |       --------
                                |               ----
                                |                   ----
                                |                       --------
rate 10 1                       |
--                              |
  *---                          |
      ----                      |
          --                    |
            --                  |
              ----              |
                  *---          |
                  |   ----      |
                  |       --    | | <- one step (two samples) delay between 9 1 and 10 1
                  |         --  | |
                  |           ----|
                  |               *---
                  |                   ----
                  |                       --
                  |                         --
                  |                           ----
rate 11 1         |
-                 |
 --               |
   *-             |
     --           |
       -          |
        -         |
         --       |
           *-     |
             --   |
               -  || <- one step (one sample) delay between 10 1 and 11 1
                - ||
                 --|
                   *-
                     --
                       -
                        -
                         --
                           *-
                             --
                               -
                                -
                                 --
*/

static void advance_eg(void)
{
	YM2151Operator *op;
	unsigned int i;



	PSG->eg_timer += PSG->eg_timer_add;

	while (PSG->eg_timer >= PSG->eg_timer_overflow)
	{
		PSG->eg_timer -= PSG->eg_timer_overflow;

		PSG->eg_cnt++;

		/* envelope generator */
		op = &PSG->oper[0];	/* CH 0 M1 */
		i = 32;
		do
		{
			switch(op->state)
			{
			case EG_ATT:	/* attack phase */
				if ( !(PSG->eg_cnt & ((1<<op->eg_sh_ar)-1) ) )
				{
					op->volume += (~op->volume *
                                   (eg_inc[op->eg_sel_ar + ((PSG->eg_cnt>>op->eg_sh_ar)&7)])
                                  ) >>4;

					if (op->volume <= MIN_ATT_INDEX)
					{
						op->volume = MIN_ATT_INDEX;
						op->state = EG_DEC;
					}

				}
			break;

			case EG_DEC:	/* decay phase */
				if ( !(PSG->eg_cnt & ((1<<op->eg_sh_d1r)-1) ) )
				{
					op->volume += eg_inc[op->eg_sel_d1r + ((PSG->eg_cnt>>op->eg_sh_d1r)&7)];

					if ( (UInt32)op->volume >= op->d1l )
						op->state = EG_SUS;

				}
			break;

			case EG_SUS:	/* sustain phase */
				if ( !(PSG->eg_cnt & ((1<<op->eg_sh_d2r)-1) ) )
				{
					op->volume += eg_inc[op->eg_sel_d2r + ((PSG->eg_cnt>>op->eg_sh_d2r)&7)];

					if ( op->volume >= MAX_ATT_INDEX )
					{
						op->volume = MAX_ATT_INDEX;
						op->state = EG_OFF;
					}

				}
			break;

			case EG_REL:	/* release phase */
				if ( !(PSG->eg_cnt & ((1<<op->eg_sh_rr)-1) ) )
				{
					op->volume += eg_inc[op->eg_sel_rr + ((PSG->eg_cnt>>op->eg_sh_rr)&7)];

					if ( op->volume >= MAX_ATT_INDEX )
					{
						op->volume = MAX_ATT_INDEX;
						op->state = EG_OFF;
					}

				}
			break;
			}
			op++;
			i--;
		}while (i);
	}
}


static void advance(void)
{
	YM2151Operator *op;
	unsigned int i;
	int a,p;

	/* LFO */
	if (PSG->test&2)
		PSG->lfo_phase = 0;
	else
	{
		PSG->lfo_timer += PSG->lfo_timer_add;
		if (PSG->lfo_timer >= PSG->lfo_overflow)
		{
			PSG->lfo_timer   -= PSG->lfo_overflow;
			PSG->lfo_counter += PSG->lfo_counter_add;
			PSG->lfo_phase   += (PSG->lfo_counter>>4);
			PSG->lfo_phase   &= 255;
			PSG->lfo_counter &= 15;
		}
	}

	i = PSG->lfo_phase;
	/* calculate LFO AM and PM waveform value (all verified on real chip, except for noise algorithm which is impossible to analyse)*/
	switch (PSG->lfo_wsel)
	{
	case 0:
		/* saw */
		/* AM: 255 down to 0 */
		/* PM: 0 to 127, -127 to 0 (at PMD=127: LFP = 0 to 126, -126 to 0) */
		a = 255 - i;
		if (i<128)
			p = i;
		else
			p = i - 255;
		break;
	case 1:
		/* square */
		/* AM: 255, 0 */
		/* PM: 128,-128 (LFP = exactly +PMD, -PMD) */
		if (i<128){
			a = 255;
			p = 128;
		}else{
			a = 0;
			p = -128;
		}
		break;
	case 2:
		/* triangle */
		/* AM: 255 down to 1 step -2; 0 up to 254 step +2 */
		/* PM: 0 to 126 step +2, 127 to 1 step -2, 0 to -126 step -2, -127 to -1 step +2*/
		if (i<128)
			a = 255 - (i*2);
		else
			a = (i*2) - 256;

		if (i<64)						/* i = 0..63 */
			p = i*2;					/* 0 to 126 step +2 */
		else if (i<128)					/* i = 64..127 */
				p = 255 - i*2;			/* 127 to 1 step -2 */
			else if (i<192)				/* i = 128..191 */
					p = 256 - i*2;		/* 0 to -126 step -2*/
				else					/* i = 192..255 */
					p = i*2 - 511;		/*-127 to -1 step +2*/
		break;
	case 3:
	default:	/*keep the compiler happy*/
		/* random */
		/* the real algorithm is unknown !!!
			We just use a snapshot of data from real chip */

		/* AM: range 0 to 255    */
		/* PM: range -128 to 127 */

		a = lfo_noise_waveform[i];
		p = a-128;
		break;
	}
	PSG->lfa = a * PSG->amd / 128;
	PSG->lfp = p * PSG->pmd / 128;


	/*	The Noise Generator of the YM2151 is 17-bit shift register.
	*	Input to the bit16 is negated (bit0 XOR bit3) (EXNOR).
	*	Output of the register is negated (bit0 XOR bit3).
	*	Simply use bit16 as the noise output.
	*/
	PSG->noise_p += PSG->noise_f;
	i = (PSG->noise_p>>16);		/* number of events (shifts of the shift register) */
	PSG->noise_p &= 0xffff;
	while (i)
	{
		UInt32 j;
		j = ( (PSG->noise_rng ^ (PSG->noise_rng>>3) ) & 1) ^ 1;
		PSG->noise_rng = (j<<16) | (PSG->noise_rng>>1);
		i--;
	}


	/* phase generator */
	op = &PSG->oper[0];	/* CH 0 M1 */
	i = 8;
	do
	{
		if (op->pms)	/* only when phase modulation from LFO is enabled for this channel */
		{
			Int32 mod_ind = PSG->lfp;		/* -128..+127 (8bits signed) */
			if (op->pms < 6)
				mod_ind >>= (6 - op->pms);
			else
				mod_ind <<= (op->pms - 5);

			if (mod_ind)
			{
				UInt32 kc_channel =	op->kc_i + mod_ind;
				(op+0)->phase += ( (PSG->freq[ kc_channel + (op+0)->dt2 ] + (op+0)->dt1) * (op+0)->mul ) >> 1;
				(op+1)->phase += ( (PSG->freq[ kc_channel + (op+1)->dt2 ] + (op+1)->dt1) * (op+1)->mul ) >> 1;
				(op+2)->phase += ( (PSG->freq[ kc_channel + (op+2)->dt2 ] + (op+2)->dt1) * (op+2)->mul ) >> 1;
				(op+3)->phase += ( (PSG->freq[ kc_channel + (op+3)->dt2 ] + (op+3)->dt1) * (op+3)->mul ) >> 1;
			}
			else		/* phase modulation from LFO is equal to zero */
			{
				(op+0)->phase += (op+0)->freq;
				(op+1)->phase += (op+1)->freq;
				(op+2)->phase += (op+2)->freq;
				(op+3)->phase += (op+3)->freq;
			}
		}
		else			/* phase modulation from LFO is disabled */
		{
			(op+0)->phase += (op+0)->freq;
			(op+1)->phase += (op+1)->freq;
			(op+2)->phase += (op+2)->freq;
			(op+3)->phase += (op+3)->freq;
		}

		op+=4;
		i--;
	}while (i);


	/* CSM is calculated *after* the phase generator calculations (verified on real chip)
	* CSM keyon line seems to be ORed with the KO line inside of the chip.
	* The result is that it only works when KO (register 0x08) is off, ie. 0
	*
	* Interesting effect is that when timer A is set to 1023, the KEY ON happens
	* on every sample, so there is no KEY OFF at all - the result is that
	* the sound played is the same as after normal KEY ON.
	*/

	if (PSG->csm_req)			/* CSM KEYON/KEYOFF seqeunce request */
	{
		if (PSG->csm_req==2)	/* KEY ON */
		{
			op = &PSG->oper[0];	/* CH 0 M1 */
			i = 32;
			do
			{
				KEY_ON(op, 2);
				op++;
				i--;
			}while (i);
			PSG->csm_req = 1;
		}
		else					/* KEY OFF */
		{
			op = &PSG->oper[0];	/* CH 0 M1 */
			i = 32;
			do
			{
				KEY_OFF(op,~2);
				op++;
				i--;
			}while (i);
			PSG->csm_req = 0;
		}
	}
}



/*	Generate samples for one of the YM2151's
*
*	'num' is the number of virtual YM2151
*	'**buffers' is table of pointers to the buffers: left and right
*	'length' is the number of samples that should be generated
*/
void YM2151UpdateOne(MameYm2151* chip, Int16* bufL, Int16* bufR, int length)
{
	int i;
	signed int outl,outr;

	PSG = chip;

	for (i=0; i<length; i++)
	{
		advance_eg();

		chip->chanout[0] = 0;
		chip->chanout[1] = 0;
		chip->chanout[2] = 0;
		chip->chanout[3] = 0;
		chip->chanout[4] = 0;
		chip->chanout[5] = 0;
		chip->chanout[6] = 0;
		chip->chanout[7] = 0;

		chan_calc(chip, 0);
		chan_calc(chip, 1);
		chan_calc(chip, 2);
		chan_calc(chip, 3);
		chan_calc(chip, 4);
		chan_calc(chip, 5);
		chan_calc(chip, 6);
		chan7_calc(chip);

		outl = chip->chanout[0] & PSG->pan[0];
		outr = chip->chanout[0] & PSG->pan[1];
		outl += (chip->chanout[1] & PSG->pan[2]);
		outr += (chip->chanout[1] & PSG->pan[3]);
		outl += (chip->chanout[2] & PSG->pan[4]);
		outr += (chip->chanout[2] & PSG->pan[5]);
		outl += (chip->chanout[3] & PSG->pan[6]);
		outr += (chip->chanout[3] & PSG->pan[7]);
		outl += (chip->chanout[4] & PSG->pan[8]);
		outr += (chip->chanout[4] & PSG->pan[9]);
		outl += (chip->chanout[5] & PSG->pan[10]);
		outr += (chip->chanout[5] & PSG->pan[11]);
		outl += (chip->chanout[6] & PSG->pan[12]);
		outr += (chip->chanout[6] & PSG->pan[13]);
		outl += (chip->chanout[7] & PSG->pan[14]);
		outr += (chip->chanout[7] & PSG->pan[15]);

		outl >>= FINAL_SH;
		outr >>= FINAL_SH;
		if (outl > MAXOUT) outl = MAXOUT;
			else if (outl < MINOUT) outl = MINOUT;
		if (outr > MAXOUT) outr = MAXOUT;
			else if (outr < MINOUT) outr = MINOUT;
		((Int16*)bufL)[i] = (Int16)outl;
		((Int16*)bufR)[i] = (Int16)outr;

		advance();
	}
}


void YM2151LoadState(MameYm2151* chip)
{
    SaveState* state = saveStateOpenForRead("ym2151_core");
    char tag[32];
    int i;
    
    chip->eg_cnt            = saveStateGet(state, "eg_cnt",            0);
    chip->eg_timer          = saveStateGet(state, "eg_timer",          0);
    chip->eg_timer_add      = saveStateGet(state, "eg_timer_add",      0);
    chip->eg_timer_overflow = saveStateGet(state, "eg_timer_overflow", 0);
    chip->lfo_phase         = saveStateGet(state, "lfo_phase",         0);
    chip->lfo_timer         = saveStateGet(state, "lfo_timer",         0);
    chip->lfo_timer_add     = saveStateGet(state, "lfo_timer_add",     0);
    chip->lfo_overflow      = saveStateGet(state, "lfo_overflow",      0);
    chip->lfo_counter       = saveStateGet(state, "lfo_counter",       0);
    chip->lfo_counter_add   = saveStateGet(state, "lfo_counter_add",   0);
    chip->lfo_wsel          = (UInt8)saveStateGet(state, "lfo_wsel",   0);
    chip->amd               = (UInt8)saveStateGet(state, "amd",        0);
    chip->pmd               = (UInt8)saveStateGet(state, "pmd",        0);
    chip->lfa               = saveStateGet(state, "lfa",               0);
    chip->lfp               = saveStateGet(state, "lfp",               0);
    chip->test              = (UInt8)saveStateGet(state, "test",       0);
    chip->ct                = (UInt8)saveStateGet(state, "ct",         0);
    chip->noise             = saveStateGet(state, "noise",             0);
    chip->noise_rng         = saveStateGet(state, "noise_rng",         0);
    chip->noise_p           = saveStateGet(state, "noise_p",           0);
    chip->noise_f           = saveStateGet(state, "noise_f",           0);
    chip->csm_req           = saveStateGet(state, "csm_req",           0);
    chip->irq_enable        = saveStateGet(state, "irq_enable",        0);
    chip->status            = saveStateGet(state, "status",            0);
    chip->timer_A_val       = (UInt16)saveStateGet(state, "timer_A_val", 0);
    chip->m2                = saveStateGet(state, "m2",                0);
    chip->c1                = saveStateGet(state, "c1",                0);
    chip->c2                = saveStateGet(state, "c2",                0);
    chip->mem               = saveStateGet(state, "mem",               0);

    for (i = 0; i < sizeof(chip->pan) / sizeof(chip->pan[0]); i++) {
        sprintf(tag, "pan%d", i);
        chip->pan[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(chip->connect) / sizeof(chip->connect[0]); i++) {
        sprintf(tag, "connect%d", i);
        chip->connect[i] = (UInt8)saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(chip->chanout) / sizeof(chip->chanout[0]); i++) {
        sprintf(tag, "chanout%d", i);
        chip->chanout[i] = (UInt8)saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(chip->oper) / sizeof(chip->oper[0]); i++) {
        sprintf(tag, "phase%d", i);
        chip->oper[i].phase = saveStateGet(state, tag, 0);
        
        sprintf(tag, "freq%d", i);
        chip->oper[i].freq = saveStateGet(state, tag, 0);
        
        sprintf(tag, "dt1%d", i);
        chip->oper[i].dt1 = saveStateGet(state, tag, 0);
        
        sprintf(tag, "mul%d", i);
        chip->oper[i].mul = saveStateGet(state, tag, 0);
        
        sprintf(tag, "dt1_i%d", i);
        chip->oper[i].dt1_i = saveStateGet(state, tag, 0);
        
        sprintf(tag, "dt2%d", i);
        chip->oper[i].dt2 = saveStateGet(state, tag, 0);
        
        sprintf(tag, "mem_value%d", i);
        chip->oper[i].mem_value = saveStateGet(state, tag, 0);
        
        sprintf(tag, "fb_shift%d", i);
        chip->oper[i].fb_shift = saveStateGet(state, tag, 0);
        
        sprintf(tag, "fb_out_curr%d", i);
        chip->oper[i].fb_out_curr = saveStateGet(state, tag, 0);
        
        sprintf(tag, "fb_out_prev%d", i);
        chip->oper[i].fb_out_prev = saveStateGet(state, tag, 0);
        
        sprintf(tag, "kc%d", i);
        chip->oper[i].kc = saveStateGet(state, tag, 0);
        
        sprintf(tag, "kc_i%d", i);
        chip->oper[i].kc_i = saveStateGet(state, tag, 0);
        
        sprintf(tag, "pms%d", i);
        chip->oper[i].pms = saveStateGet(state, tag, 0);
        
        sprintf(tag, "ams%d", i);
        chip->oper[i].ams = saveStateGet(state, tag, 0);
        
        sprintf(tag, "AMmask%d", i);
        chip->oper[i].AMmask = saveStateGet(state, tag, 0);
        
        sprintf(tag, "state%d", i);
        chip->oper[i].state = saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sh_ar%d", i);
        chip->oper[i].eg_sh_ar = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sel_ar%d", i);
        chip->oper[i].eg_sel_ar = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "tl%d", i);
        chip->oper[i].tl = saveStateGet(state, tag, 0);
        
        sprintf(tag, "volume%d", i);
        chip->oper[i].volume = saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sh_d1r%d", i);
        chip->oper[i].eg_sh_d1r = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sel_d1r%d", i);
        chip->oper[i].eg_sel_d1r = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "d1l%d", i);
        chip->oper[i].d1l = saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sh_d2r%d", i);
        chip->oper[i].eg_sh_d2r = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sel_d2r%d", i);
        chip->oper[i].eg_sel_d2r = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sh_rr%d", i);
        chip->oper[i].eg_sh_rr = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "eg_sel_rr%d", i);
        chip->oper[i].eg_sel_rr = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "key%d", i);
        chip->oper[i].key = saveStateGet(state, tag, 0);
        
        sprintf(tag, "ks%d", i);
        chip->oper[i].ks = saveStateGet(state, tag, 0);
        
        sprintf(tag, "ar%d", i);
        chip->oper[i].ar = saveStateGet(state, tag, 0);
        
        sprintf(tag, "d1r%d", i);
        chip->oper[i].d1r = saveStateGet(state, tag, 0);
        
        sprintf(tag, "d2r%d", i);
        chip->oper[i].d2r = saveStateGet(state, tag, 0);
        
        sprintf(tag, "rr%d", i);
        chip->oper[i].rr = saveStateGet(state, tag, 0);
        
        sprintf(tag, "connect%d", i);
        if (chip->oper[i].connect != NULL) {
            chip->oper[i].connect += (int*)chip - (int*)0;
        }
        
        sprintf(tag, "mem_connect%d", i);
        if (chip->oper[i].mem_connect != NULL) {
            chip->oper[i].mem_connect +=  (int*)chip - (int*)0;
        }
    }

    saveStateClose(state);
}

void YM2151SaveState(MameYm2151* chip)
{
    SaveState* state = saveStateOpenForWrite("ym2151_core");
    char tag[32];
    int i;

    saveStateSet(state, "eg_cnt",            chip->eg_cnt);
    saveStateSet(state, "eg_timer",          chip->eg_timer);
    saveStateSet(state, "eg_timer_add",      chip->eg_timer_add);
    saveStateSet(state, "eg_timer_overflow", chip->eg_timer_overflow);
    saveStateSet(state, "lfo_phase",         chip->lfo_phase);
    saveStateSet(state, "lfo_timer",         chip->lfo_timer);
    saveStateSet(state, "lfo_timer_add",     chip->lfo_timer_add);
    saveStateSet(state, "lfo_overflow",      chip->lfo_overflow);
    saveStateSet(state, "lfo_counter",       chip->lfo_counter);
    saveStateSet(state, "lfo_counter_add",   chip->lfo_counter_add);
    saveStateSet(state, "lfo_wsel",          chip->lfo_wsel);
    saveStateSet(state, "amd",               chip->amd);
    saveStateSet(state, "pmd",               chip->pmd);
    saveStateSet(state, "lfa",               chip->lfa);
    saveStateSet(state, "lfp",               chip->lfp);
    saveStateSet(state, "test",              chip->test);
    saveStateSet(state, "ct",                chip->ct);
    saveStateSet(state, "noise",             chip->noise);
    saveStateSet(state, "noise_rng",         chip->noise_rng);
    saveStateSet(state, "noise_p",           chip->noise_p);
    saveStateSet(state, "noise_f",           chip->noise_f);
    saveStateSet(state, "csm_req",           chip->csm_req);
    saveStateSet(state, "irq_enable",        chip->irq_enable);
    saveStateSet(state, "status",            chip->status);
    saveStateSet(state, "timer_A_val",       chip->timer_A_val);
    saveStateSet(state, "m2",                chip->m2);
    saveStateSet(state, "c1",                chip->c1);
    saveStateSet(state, "c2",                chip->c2);
    saveStateSet(state, "mem",               chip->mem);

    for (i = 0; i < sizeof(chip->pan) / sizeof(chip->pan[0]); i++) {
        sprintf(tag, "pan%d", i);
        saveStateSet(state, tag, chip->pan[i]);
    }

    for (i = 0; i < sizeof(chip->connect) / sizeof(chip->connect[0]); i++) {
        sprintf(tag, "connect%d", i);
        saveStateSet(state, tag, chip->connect[i]);
    }

    for (i = 0; i < sizeof(chip->chanout) / sizeof(chip->chanout[0]); i++) {
        sprintf(tag, "chanout%d", i);
        saveStateSet(state, tag, chip->chanout[i]);
    }

    for (i = 0; i < sizeof(chip->oper) / sizeof(chip->oper[0]); i++) {
        sprintf(tag, "phase%d", i);
        saveStateSet(state, tag, chip->oper[i].phase);
        
        sprintf(tag, "freq%d", i);
        saveStateSet(state, tag, chip->oper[i].freq);
        
        sprintf(tag, "dt1%d", i);
        saveStateSet(state, tag, chip->oper[i].dt1);
        
        sprintf(tag, "mul%d", i);
        saveStateSet(state, tag, chip->oper[i].mul);
        
        sprintf(tag, "dt1_i%d", i);
        saveStateSet(state, tag, chip->oper[i].dt1_i);
        
        sprintf(tag, "dt2%d", i);
        saveStateSet(state, tag, chip->oper[i].dt2);
        
        sprintf(tag, "mem_value%d", i);
        saveStateSet(state, tag, chip->oper[i].mem_value);
        
        sprintf(tag, "fb_shift%d", i);
        saveStateSet(state, tag, chip->oper[i].fb_shift);
        
        sprintf(tag, "fb_out_curr%d", i);
        saveStateSet(state, tag, chip->oper[i].fb_out_curr);
        
        sprintf(tag, "fb_out_prev%d", i);
        saveStateSet(state, tag, chip->oper[i].fb_out_prev);
        
        sprintf(tag, "kc%d", i);
        saveStateSet(state, tag, chip->oper[i].kc);
        
        sprintf(tag, "kc_i%d", i);
        saveStateSet(state, tag, chip->oper[i].kc_i);
        
        sprintf(tag, "pms%d", i);
        saveStateSet(state, tag, chip->oper[i].pms);
        
        sprintf(tag, "ams%d", i);
        saveStateSet(state, tag, chip->oper[i].ams);
        
        sprintf(tag, "AMmask%d", i);
        saveStateSet(state, tag, chip->oper[i].AMmask);
        
        sprintf(tag, "state%d", i);
        saveStateSet(state, tag, chip->oper[i].state);
        
        sprintf(tag, "eg_sh_ar%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sh_ar);
        
        sprintf(tag, "eg_sel_ar%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sel_ar);
        
        sprintf(tag, "tl%d", i);
        saveStateSet(state, tag, chip->oper[i].tl);
        
        sprintf(tag, "volume%d", i);
        saveStateSet(state, tag, chip->oper[i].volume);
        
        sprintf(tag, "eg_sh_d1r%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sh_d1r);
        
        sprintf(tag, "eg_sel_d1r%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sel_d1r);
        
        sprintf(tag, "d1l%d", i);
        saveStateSet(state, tag, chip->oper[i].d1l);
        
        sprintf(tag, "eg_sh_d2r%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sh_d2r);
        
        sprintf(tag, "eg_sel_d2r%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sel_d2r);
        
        sprintf(tag, "eg_sh_rr%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sh_rr);
        
        sprintf(tag, "eg_sel_rr%d", i);
        saveStateSet(state, tag, chip->oper[i].eg_sel_rr);
        
        sprintf(tag, "key%d", i);
        saveStateSet(state, tag, chip->oper[i].key);
        
        sprintf(tag, "ks%d", i);
        saveStateSet(state, tag, chip->oper[i].ks);
        
        sprintf(tag, "ar%d", i);
        saveStateSet(state, tag, chip->oper[i].ar);
        
        sprintf(tag, "d1r%d", i);
        saveStateSet(state, tag, chip->oper[i].d1r);
        
        sprintf(tag, "d2r%d", i);
        saveStateSet(state, tag, chip->oper[i].d2r);
        
        sprintf(tag, "rr%d", i);
        saveStateSet(state, tag, chip->oper[i].rr);
        
        sprintf(tag, "connect%d", i);
        if (chip->oper[i].connect != NULL) {
            saveStateSet(state, tag, (int*)chip->oper[i].connect - (int*)chip);
        }
        else {
            saveStateSet(state, tag, 0);
        }
        
        sprintf(tag, "mem_connect%d", i);
        if (chip->oper[i].mem_connect != NULL) {
            saveStateSet(state, tag, (int*)chip->oper[i].mem_connect - (int*)chip);
        }
        else {
            saveStateSet(state, tag, 0);
        }
    }

    saveStateClose(state);
}
