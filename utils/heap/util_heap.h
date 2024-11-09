/**
 * @file util_heap.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 * a new version of `tos_mem`
 */
#ifndef _UTIL_HEAP_H_
#define _UTIL_HEAP_H_

#include "util_types.h"

#define UTIL_HEAP_BUFFER_SIZE (120 * 1024)

void*       util_malloc(util_size_t nbytes);
void        util_free(void* ptr);
void*       util_realloc(void* optr, util_size_t nsize);
util_size_t util_heap_freesize(void);
void        util_heapinfo(void);

#endif
