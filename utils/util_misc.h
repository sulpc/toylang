/**
 * @file util_misc.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

#include <ctype.h>    // isdigit
#include <math.h>     // powf
#include <setjmp.h>   //
#include <stdlib.h>   // atoi, atof
#include <string.h>   // strcmp, memset

#define util_arraylen(array)               (sizeof(array) / sizeof(array[0]))
#define util_min2(a, b)                    ((a) < (b) ? (a) : (b))
#define util_min3(a, b, c)                 ((a) < (b) ? util_min2(a, c) : util_min2(b, c))
#define util_max2(a, b)                    ((a) > (b) ? (a) : (b))
#define util_max3(a, b, c)                 ((a) > (b) ? util_max2(a, c) : util_max2(b, c))
#define util_bitmask(n)                    (1 << n)
#define util_clrbits(x, bm)                ((x) &= ~(bm))
#define util_setbits(x, bm)                ((x) |= (bm))
#define util_chkbits(x, bm)                ((x) & (bm))
#define util_mod(x, n)                     ((x) & ((n) - 1))   // n must pow of 2
#define util_containerof(type, field, ptr) ((type*)((uint8_t*)(ptr) - (uint32_t) & ((type*)0)->field))
#define util_fieldoffset(type, field)      ((uint32_t) & (((type*)0)->field))
#define util_checkvalue(v, min, max)       ((v) > (min) && v < (max))
#define util_limitvalue(v, min, max)       ((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v))
#define util_getbigendian2(buf)            (((uint16_t)buf[0] << 8) | ((uint16_t)buf[1] << 0))
#define util_getbigendian4(buf)            (((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3])
#define util_setbigendian2(buf, val)                                                                                   \
    do {                                                                                                               \
        buf[0] = (val >> 8) & 0xff;                                                                                    \
        buf[1] = val & 0xff;                                                                                           \
    } while (0)
#define util_setbigendian4(buf, val)                                                                                   \
    do {                                                                                                               \
        buf[0] = (val >> 24) & 0xff;                                                                                   \
        buf[1] = (val >> 16) & 0xff;                                                                                   \
        buf[2] = (val >> 8) & 0xff;                                                                                    \
        buf[3] = val & 0xff;                                                                                           \
    } while (0)

#ifdef HOST_DEBUG
#include <stdio.h>
#define xprintf printf
#else
// todo
extern void xprintf(const char* fmt, ...);
#endif

#define util_printf xprintf

#endif
