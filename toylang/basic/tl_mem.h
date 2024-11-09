#ifndef _TL_MEM_H_
#define _TL_MEM_H_

#include "tl_env.h"
#include "tl_values.h"

void* tl_new(tl_size_t size);
void  tl_free(void* ptr, tl_size_t size);
void* tl_realloc(void* optr, tl_size_t osize, tl_size_t nsize);

#define tl_newvector(size, type)                  tl_new(size * sizeof(type))
#define tl_freevector(ptr, size, type)            tl_free(ptr, size * sizeof(type))
#define tl_reallocvector(ptr, osize, nsize, type) (type*)tl_realloc(ptr, osize * sizeof(type), nsize * sizeof(type))


#if TL_DEBUG_MEMINFO_EN
void      tl_meminfo_display(void);
tl_size_t tl_mem_used(void);
#else
#define tl_meminfo_display()
#define tl_mem_used() 0
#endif

#endif
