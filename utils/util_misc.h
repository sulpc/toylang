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
#include "util_types.h"

#define util_arraylen(array)                 (sizeof(array) / sizeof(array[0]))

#define util_min2(a, b)                      ((a) < (b) ? (a) : (b))
#define util_min3(a, b, c)                   ((a) < (b) ? util_min2(a, c) : util_min2(b, c))
#define util_max2(a, b)                      ((a) > (b) ? (a) : (b))
#define util_max3(a, b, c)                   ((a) > (b) ? util_max2(a, c) : util_max2(b, c))
#define util_checkvalue(v, min, max)         ((v) > (min) && v < (max))
#define util_limitvalue(v, min, max)         ((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v))

#define util_bitmask(n)                      (1 << n)
#define util_clrbits(x, bm)                  ((x) &= ~(bm))
#define util_setbits(x, bm)                  ((x) |= (bm))
#define util_chkbits(x, bm)                  ((x) & (bm))

#define util_mod(x, n)                       ((x) & ((n) - 1))   // n must pow of 2
#define util_display(value)                  util_printf("%s = 0x%x, %d\n", #value, value, value)
#define util_unused(v)                       (void)v

#define util_bitmap_set(u32bitmaparray, pos) u32bitmaparray[pos >> 5] |= ((uint32_t)0x1 << (pos & 0x1F))
#define util_bitmap_clr(u32bitmaparray, pos) u32bitmaparray[pos >> 5] &= ~((uint32_t)0x1 << (pos & 0x1F))
#define util_bitmap_chk(u32bitmaparray, pos) (u32bitmaparray[pos >> 5] & ((uint32_t)0x1 << (pos & 0x1F)))

#define util_containerof(type, field, ptr)   ((type*)((uint8_t*)(ptr) - (uint32_t) & ((type*)0)->field))
#define util_fieldoffset(type, field)        ((uint32_t) & (((type*)0)->field))

#define util_getbigendian2(buf)              (((uint16_t)buf[0] << 8) | ((uint16_t)buf[1] << 0))
#define util_getbigendian4(buf)              (((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3])
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

/**
 * @brief bytes to uint, little endian
 *
 * @param buf
 * @param size
 * @return uint32_t
 */
static inline uint32_t util_bytes2uint_le(uint8_t* buf, uint8_t size)
{
    union {
        uint32_t u32;
        uint8_t  u8[4];
    } value = {.u32 = 0};
    for (uint8_t i = 0; i < size; i++) {
        value.u8[i] = buf[i];
    }
    return value.u32;
}

static inline uint32_t util_bytes2uint_be(uint8_t* buf, uint8_t size)
{
    union {
        uint32_t u32;
        uint8_t  u8[4];
    } value = {.u32 = 0};
    for (uint8_t i = 0; i <= size; i++) {
        value.u8[size - i - 1] = buf[i];
    }
    return value.u32;
}

#ifdef HOST_DEBUG
#include <stdio.h>
#define util_printf printf
#else
#include "util_log.h"
#endif

#endif
