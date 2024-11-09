/**
 * @file tos_heap.c
 * @brief memory management
 *
 */
#include "util_heap.h"
#include "util_misc.h"
#include "util_queue.h"

// #pragma anon_unions

// clang-format off
#define heap_log(...)                   // util_printf(__VA_ARGS__),util_printf('\n')
#define heap_err(...)                   // util_printf("ERR: "), util_printf(__VA_ARGS__), util_printf(" in %s", __func__), util_printf("\n")
#define cond_check(err_cond, action)    do { if ((err_cond)) { heap_err(#err_cond); action; } } while (0)
// clang-format on

/*
 block head:
    size       : for free block: block_size; for used block: (MAGIC | user_size)
    all_link   : link block in heap_all_blocks
 */
#define HEAP_BLK_HEAD                                                                                                  \
    util_size_t       size;                                                                                            \
    util_queue_node_t all_link

typedef struct {
    HEAP_BLK_HEAD;
} blkhead_t;

typedef struct {
    HEAP_BLK_HEAD;
    union {
        util_queue_node_t free_link;   // link block in heap_free_blocks
        uint8_t           user_space[4];
    };
} memblk_t;

// clang-format off
#define HEAP_SIZE                  UTIL_HEAP_BUFFER_SIZE  // heap size
#define HEAP_ADDR_ALIGN            4                      //
#define HEAP_SMALL_BLK_MAX         512u                   // max small block size
#define HEAP_BLK_SIZE_UNIT         8u                     // block size will round up to a multiple of HEAP_BLK_SIZE_UNIT

#define HEAP_ADDR_START            ((uint8_t*)heap_space)
#define HEAP_ADDR_END              ((uint8_t*)heap_space + sizeof(heap_space))
#define HEAP_BLK_MAGIC_MASK        0xA5000000             // used to identify the allocated blocks
#define HEAP_BLK_SIZE_MASK         0x00FFFFFF
#define HEAP_BLK_HEAD_SIZE         sizeof(blkhead_t)
#define HEAP_BLK_MIN_SIZE          sizeof(memblk_t)

#define HEAP_BLK_SLOT_NUM          (HEAP_SMALL_BLK_MAX / HEAP_BLK_SIZE_UNIT + 1)
#define HEAP_LARGE_BLK_IDX         (HEAP_SMALL_BLK_MAX / HEAP_BLK_SIZE_UNIT)
#define HEAP_SMALL_BLK_IDX(nbytes) ((nbytes) / HEAP_BLK_SIZE_UNIT - 1)
#define blk_slot_idx(nbytes)       ((nbytes) > HEAP_SMALL_BLK_MAX ? HEAP_LARGE_BLK_IDX : HEAP_SMALL_BLK_IDX(nbytes))

#define blk_get_size(blk)          (blk->size & HEAP_BLK_SIZE_MASK)
#define blk_set_size(blk, nbytes)  (blk->size = ((nbytes) & HEAP_BLK_SIZE_MASK) | HEAP_BLK_MAGIC_MASK)
#define blk_chk_magic(blk)         ((blk->size & HEAP_BLK_MAGIC_MASK) == HEAP_BLK_MAGIC_MASK)
#define blk_size_roundup(nbytes)   (((nbytes) + HEAP_BLK_SIZE_UNIT - 1) & ~(HEAP_BLK_SIZE_UNIT - 1))

#define blk2userptr(blk)           blk->user_space
#define userptr2blk(uptr)          (memblk_t*)((uint8_t*)uptr - HEAP_BLK_HEAD_SIZE)
// clang-format on


static uint32_t          heap_space[HEAP_SIZE / 4];
static util_queue_node_t heap_free_blocks[HEAP_BLK_SLOT_NUM];   //
static util_queue_node_t heap_all_blocks;                       // list of blk, order by addr
static util_size_t       heap_free_size = 0;
static bool              heap_inited    = false;


static void      heap_init(void);
static memblk_t* heap_alloc_blk(util_size_t nbytes);
static void      heap_free_blk(memblk_t* blk);
static memblk_t* heap_get_blk_from_slot(util_size_t nbytes, util_size_t slot);
static void      heap_add_free_blk_by_size(memblk_t* blk);


void* util_malloc(util_size_t nbytes)
{
    if (nbytes == 0) {
        return nullptr;
    }

    util_size_t blk_size = nbytes + HEAP_BLK_HEAD_SIZE;
    if (blk_size < HEAP_BLK_MIN_SIZE)
        blk_size = HEAP_BLK_MIN_SIZE;
    blk_size = blk_size_roundup(blk_size);

    if (!heap_inited) {
        heap_init();
    }

    // check nbytes valid
    cond_check((blk_size > heap_free_size), return nullptr);

    memblk_t* blk = heap_alloc_blk(blk_size);

    if (blk != nullptr) {
        heap_log("- malloc %d bytes @0x%08x\n", blk->size, (util_size_t)blk);
        blk_set_size(blk, blk->size - HEAP_BLK_HEAD_SIZE);
        return blk->user_space;
    } else {
        heap_err("! malloc fail\n");
        return nullptr;
    }
}


void util_free(void* ptr)
{
    if (ptr == nullptr) {
        return;
    }

    cond_check((!heap_inited), return);

    // check addr align
    cond_check((((util_size_t)ptr & (HEAP_ADDR_ALIGN - 1)) != 0), return);

    // check addr in heap field
    cond_check(((uint8_t*)ptr < (uint8_t*)HEAP_ADDR_START || (uint8_t*)ptr > (uint8_t*)HEAP_ADDR_END), return);

    memblk_t* blk = userptr2blk(ptr);

    // check block magic
    cond_check((!blk_chk_magic(blk)), return);

    util_size_t blk_size = blk_get_size(blk) + HEAP_BLK_HEAD_SIZE;

    // check block size
    cond_check(((uint8_t*)blk + blk_size > (uint8_t*)HEAP_ADDR_END), return);

    blk->size = blk_size;
    heap_free_blk(blk);
}


void* util_realloc(void* optr, util_size_t nsize)
{
    if (!heap_inited) {
        heap_init();
    }

    void* nptr = nullptr;

    if (nsize) {
        nptr = util_malloc(nsize);
    }

    if (nptr && optr) {
        memblk_t* nblk = userptr2blk(nptr);
        memblk_t* oblk = userptr2blk(optr);
        memcpy(nptr, optr, util_min2(blk_get_size(oblk), blk_get_size(nblk)));
    }

    if (optr) {
        util_free(optr);
    }

    return nptr;
}


util_size_t util_heap_freesize(void)
{
    return heap_free_size;
}


static void heap_init(void)
{
    memblk_t* blk = (memblk_t*)heap_space;

    blk->size = sizeof(heap_space);
    util_queue_init(&blk->all_link);
    util_queue_init(&blk->free_link);

    for (int i = 0; i < util_arraylen(heap_free_blocks); i++) {
        util_queue_init(&heap_free_blocks[i]);
    }
    util_queue_init(&heap_all_blocks);

    heap_add_free_blk_by_size(blk);
    util_queue_insert(&heap_all_blocks, &blk->all_link);

    heap_free_size = sizeof(heap_space);
    heap_inited    = true;
}


static memblk_t* heap_alloc_blk(util_size_t nbytes)
{
    // for small block, to reduce the number of table lookups, the lookup order is:
    //   1. best match: get a block from the list at slot `blk_slot_idx(nbytes)`
    //   2. divide a larger block from the big block list
    //   3. divide a larger block from the block with other size

    util_size_t        best_slot = blk_slot_idx(nbytes);
    util_queue_node_t* free_list = &heap_free_blocks[best_slot];
    memblk_t*          blk       = nullptr;

    // 1.
    if (nbytes < HEAP_SMALL_BLK_MAX) {
        if (!util_queue_empty(free_list)) {
            blk = util_containerof(memblk_t, free_link, free_list->next);
            util_queue_remove(&blk->free_link);
            heap_free_size -= blk->size;
            return blk;
        }
    }

    // 2.
    blk = heap_get_blk_from_slot(nbytes, HEAP_LARGE_BLK_IDX);
    if (blk != nullptr) {
        return blk;
    }

    // 3.
    for (util_size_t slot = best_slot + 1; slot <= blk_slot_idx(HEAP_SMALL_BLK_MAX); slot++) {
        blk = heap_get_blk_from_slot(nbytes, slot);
        if (blk != nullptr) {
            return blk;
        }
    }

    return blk;
}


static void heap_free_blk(memblk_t* blk)
{
    memblk_t* prev_blk =
        (blk->all_link.prev == &heap_all_blocks) ? nullptr : util_containerof(memblk_t, all_link, blk->all_link.prev);
    memblk_t* next_blk =
        (blk->all_link.next == &heap_all_blocks) ? nullptr : util_containerof(memblk_t, all_link, blk->all_link.next);
    util_queue_node_t* next = blk->all_link.next;

    heap_free_size += blk->size;

    // rm blk from all list
    util_queue_remove(&blk->all_link);

    // if prev block is free, merge it
    if (prev_blk && !blk_chk_magic(prev_blk) && (uint8_t*)prev_blk + prev_blk->size == (uint8_t*)blk) {
        util_queue_remove(&prev_blk->all_link);
        util_queue_remove(&prev_blk->free_link);

        heap_log("- merge block %p & %p\n", prev_blk, blk);

        prev_blk->size = prev_blk->size + blk->size;
        blk            = prev_blk;
    }

    // if next block is free, merge it
    if (next_blk && !blk_chk_magic(next_blk) && (uint8_t*)blk + blk->size == (uint8_t*)next_blk) {
        next = next->next;

        util_queue_remove(&next_blk->all_link);
        util_queue_remove(&next_blk->free_link);

        heap_log("- merge block %p & %p\n", blk, next_blk);

        blk->size = blk->size + next_blk->size;
    }

    heap_add_free_blk_by_size(blk);
    util_queue_insert(next, &blk->all_link);
}


static memblk_t* heap_get_blk_from_slot(util_size_t nbytes, util_size_t slot)
{
    util_queue_node_t* free_list = &heap_free_blocks[slot];

    if (util_queue_empty(free_list)) {
        return nullptr;
    }

    // get a free blk in this free list
    memblk_t* blk = util_containerof(memblk_t, free_link, free_list->next);

    // for nbytes > HEAP_SMALL_BLK_MAX, travel the list to find a suitable block
    // for nbytes <= HEAP_SMALL_BLK_MAX, the first block would suitable
    while (blk->size < nbytes) {
        if (blk->free_link.next == free_list) {
            // travel end, no suitable blk
            return nullptr;
        }
        blk = util_containerof(memblk_t, free_link, blk->free_link.next);
    }

    // get a free block
    util_queue_remove(&blk->free_link);

    // the block is larger, divide this block. ensure the remaining block size
    if (blk->size >= nbytes + HEAP_BLK_MIN_SIZE) {
        util_queue_node_t* next = blk->all_link.next;
        memblk_t*          blk2 = (memblk_t*)((uint8_t*)blk + nbytes);

        // blk2 is a new free block
        blk2->size = blk->size - nbytes;
        blk->size  = nbytes;
        heap_add_free_blk_by_size(blk2);

        util_queue_remove(&blk->all_link);                    // rm blk from addr list
        util_queue_insert(next, &blk2->all_link);             // insert blk2 before next
        util_queue_insert(&blk2->all_link, &blk->all_link);   // insert blk before blk2
    }

    heap_free_size -= blk->size;
    return blk;
}


static void heap_add_free_blk_by_size(memblk_t* blk)
{
    util_size_t        slot      = blk_slot_idx(blk->size);
    util_queue_node_t* free_list = &heap_free_blocks[slot];

    if (blk->size <= HEAP_SMALL_BLK_MAX) {
        // add free blk to table
        util_queue_insert(free_list, &blk->free_link);
    } else {
        util_queue_node_t* next = free_list->next;

        while (next != free_list && util_containerof(memblk_t, free_link, next)->size < blk->size) {
            next = next->next;
        }

        util_queue_insert(next, &blk->free_link);
    }
}


void util_heapinfo(void)
{
    if (!heap_inited) {
        heap_init();
    }

    util_queue_node_t* node;
    memblk_t*          blk;

    heap_log("+----------------------------------------+");
    heap_log("heap:");
    heap_log("         [0x%08x, 0x%08x)  %d bytes free", (util_size_t)HEAP_ADDR_START, (util_size_t)HEAP_ADDR_END,
             heap_free_size);
    heap_log("all blocks:");
    node = heap_all_blocks.next;

    util_size_t next_start = (util_size_t)heap_space;

    while (node != &heap_all_blocks) {
        blk = util_containerof(memblk_t, all_link, node);

        bool        busy  = blk_chk_magic(blk);
        util_size_t start = (util_size_t)blk;
        util_size_t size  = busy ? blk_get_size(blk) + HEAP_BLK_HEAD_SIZE : blk_get_size(blk);

        heap_log("    %s  [0x%08x, 0x%08x)  %5d bytes", busy ? "[+]" : "[ ]", start, start + size, size);

        if (next_start != start) {
            heap_err("!!! address discontinuity");
            extern void exit(int);
            exit(0);
        }

        next_start = start + size;

        node = node->next;
    }

    heap_log("free blocks:\n");
    for (util_size_t slot = 0; slot < HEAP_BLK_SLOT_NUM; slot++) {
        node = heap_free_blocks[slot].next;

        while (node != &heap_free_blocks[slot]) {
            blk = util_containerof(memblk_t, free_link, node);

            bool        busy  = blk_chk_magic(blk);
            util_size_t start = (util_size_t)blk;
            util_size_t size  = blk_get_size(blk);

            start = start;
            size  = size;
            busy  = busy;

            heap_log("    [ ]  [0x%08x, 0x%08x)  %5d bytes %s\n", start, start + size, size, busy ? "ERROR" : "");

            node = node->next;
        }
    }
    heap_log("+----------------------------------------+\n");
}
