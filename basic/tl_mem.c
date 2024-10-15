#include "tl_mem.h"

#define mem_debug 0
#define mem_info  tl_none

#define mem_strategy_stdlib   0
#define mem_strategy_myheap   1
#define mem_strategy_freertos 2

#ifdef HOST_DEBUG
#define mem_strategy mem_strategy_myheap
#else
#define mem_strategy mem_strategy_freertos
#endif

#if mem_strategy == mem_strategy_stdlib
#include <stdlib.h>
#define mem_use_realloc 1
#elif mem_strategy == mem_strategy_myheap
#include "util_heap.h"
#define mem_use_realloc 1
#define free            util_free
#define malloc          util_malloc
#define realloc         util_realloc
#elif mem_strategy == mem_strategy_freertos
#include "FreeRTOS.h"
#define mem_use_realloc 0
#define free            vPortFree
#define malloc          pvPortMalloc
#else
#error "invalid memory strategy"
#endif


#if TL_DEBUG_MEMINFO_EN
typedef struct addr_field_t {
    tl_byte_t*           start;
    tl_size_t            size;
    struct addr_field_t* next;
} addr_field_t;

static addr_field_t  headnode = {0, 0, nullptr};
static addr_field_t* memlist  = &headnode;

static void mnode_use(tl_byte_t* start, tl_size_t size)
{
    if (start == nullptr || size == 0)
        return;

    addr_field_t* pre = memlist;
    addr_field_t* ptr = memlist->next;

    while (ptr != nullptr && start > ptr->start) {
        pre = pre->next;
        ptr = ptr->next;
    }

    if (ptr != nullptr && (start == ptr->start || start + size > ptr->start)) {
        tl_panic(TL_PANIC_MEMORY, "mem conlict: [%p, %p) with [%p, %p)", start, start + size, ptr->start,
                 ptr->start + ptr->size);
    }

    addr_field_t* newnode = (addr_field_t*)malloc(sizeof(addr_field_t));

    newnode->start = start;
    newnode->size  = size;
    newnode->next  = ptr;
    pre->next      = newnode;
    mem_info("mem use [%p, %p)", start, start + size);
}

static void mnode_free(tl_byte_t* start, tl_size_t size)
{
    if (start == nullptr || size == 0)
        return;

    addr_field_t* pre = memlist;
    addr_field_t* ptr = pre->next;

    while (ptr != nullptr && start != ptr->start) {
        pre = pre->next;
        ptr = ptr->next;
    }

    if (ptr == nullptr || ptr->size != size) {
        tl_panic(TL_PANIC_MEMORY, "mem miss: [%p, %p)", start, start + size);
    }

    pre->next = ptr->next;
    free(ptr);
    mem_info("mem free [%p, %p)", start, start + size);
}

void tl_meminfo_display(void)
{
    addr_field_t* ptr  = memlist->next;
    tl_size_t     used = 0;
    tl_printf("MEMINFO:\n");
    while (ptr != nullptr) {
        tl_printf("    [%p, %p) %d\n", ptr->start, ptr->start + ptr->size, ptr->size);
        used += ptr->size;
        ptr = ptr->next;
    }
    tl_printf("  total %d bytes\n", used);
}

tl_size_t tl_mem_used(void)
{
    addr_field_t* ptr  = memlist->next;
    tl_size_t     used = 0;
    while (ptr != nullptr) {
        used += ptr->size;
        ptr = ptr->next;
    }
    return used;
}
#endif

void* tl_new(tl_size_t size)
{
    void* ptr = malloc(size);

    if (ptr == nullptr && size != 0) {
        tl_panic(TL_PANIC_MEMORY, "tl_new failed");
    }
    memset(ptr, 0, size);

#if TL_DEBUG_MEMINFO_EN
    mnode_use((tl_byte_t*)ptr, size);
    mem_info("mem new : %p<%d>", ptr, size);
#endif
    if (tl_env.gc_inited)
        tl_env.gc_debt += size;
    return ptr;
}

void tl_free(void* ptr, tl_size_t size)
{
#if TL_DEBUG_MEMINFO_EN
    mnode_free((tl_byte_t*)ptr, size);
    mem_info("mem free: %p<%d>", ptr, size);
#endif
    free(ptr);
    if (tl_env.gc_inited)
        tl_env.gc_debt -= size;
}

void* tl_realloc(void* optr, tl_size_t osize, tl_size_t nsize)
{
    void* nptr = nullptr;
    osize      = optr ? osize : 0;

#if mem_use_realloc
    nptr = realloc(optr, nsize);
    if (nptr == nullptr && nsize != 0) {
        tl_panic(TL_PANIC_MEMORY, "tl_alloc failed");
    }
#else
    if (nsize) {
        nptr = malloc(nsize);
        memcpy(nptr, optr, util_min2(osize, nsize));
    }
    free(optr);
#endif
    if (nsize > osize) {
        memset((char*)nptr + osize, 0, nsize - osize);
    }
#if TL_DEBUG_MEMINFO_EN
    mnode_free((tl_byte_t*)optr, osize);
    mnode_use((tl_byte_t*)nptr, nsize);
    mem_info("mem realloc: %p<%d> -> %p<%d>", optr, osize, nptr, nsize);
#endif
    if (tl_env.gc_inited)
        tl_env.gc_debt += nsize - osize;
    return nptr;
}
