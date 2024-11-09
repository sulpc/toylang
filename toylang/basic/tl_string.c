#include "tl_string.h"
#include "tl_gc.h"
#include "tl_utils.h"

#define string_debug 0
#define string_info  tl_none
#define mem_info     tl_none

#define strlen2size(len) (sizeof(tl_string_t) + len + 1)


static uint32_t string_hash(const char* str, uint32_t len)
{
#if 0
    // by codegeex
    uint32_t hash = 0;
    for (uint32_t i = 0; i < len; i++) {
        hash       = (hash << 4) + str[i];
        uint32_t g = hash & 0xF0000000;
        if (g != 0) {
            hash ^= g >> 24;
        }
        hash &= ~g;
    }
    return hash;
#else
    // by lua
    uint32_t hash = TL_STRING_HASH_SEED ^ len;
    size_t   step = (len >> 5) + 1;
    for (; len >= step; len -= step)
        hash ^= (hash << 5) + (hash >> 2) + str[len - 1];
    return hash;
#endif
}


static void stringpool_resize(tl_size_t nsize)
{
    tl_size_t osize = tl_env.stringpool.slot;
    if (osize == nsize)
        return;

    // expand hashtable
    if (nsize > osize) {
        tl_env.stringpool.hashtable = tl_reallocvector(tl_env.stringpool.hashtable, osize, nsize, tl_string_t*);
        for (tl_size_t i = osize; i < nsize; i++) {
            tl_env.stringpool.hashtable[i] = nullptr;
        }
    }

    if (nsize != 0) {
        // traverse old hashtable
        for (tl_size_t i = 0; i < osize; i++) {
            tl_string_t* s                 = tl_env.stringpool.hashtable[i];
            tl_env.stringpool.hashtable[i] = nullptr;   // notice

            // traverse string list, rehash all
            while (s != nullptr) {
                tl_string_t* next = s->next;                    // store next
                tl_size_t    idx  = util_mod(s->hash, nsize);   // new slot index

                // insert string in new list
                s->next                          = tl_env.stringpool.hashtable[idx];
                tl_env.stringpool.hashtable[idx] = s;
                string_info("string<%s> in slot[%d] move to [%d]", TL_DEBUG_OBJNAME(s), i, idx);
                s = next;
            }
        }
    }

    // compress
    if (nsize < osize) {
        tl_env.stringpool.hashtable = tl_reallocvector(tl_env.stringpool.hashtable, osize, nsize, tl_string_t*);
    }

    mem_info("stringpool hashtable resize@%p, slots %d->%d", tl_env.stringpool.hashtable, osize, nsize);

    tl_env.stringpool.slot = nsize;
}


void tl_stringpool_init(void)
{
    tl_env.stringpool.hashtable = nullptr;
    tl_env.stringpool.count     = 0;
    tl_env.stringpool.slot      = 0;
#if TL_STRING_INTERALIZE_ENABLE
    stringpool_resize(TL_STRING_POOL_SLOTS_DEFAULT);
#endif
}


void tl_stringpool_deinit(void)
{
    if (tl_env.stringpool.count == 0) {
#if TL_STRING_INTERALIZE_ENABLE
        stringpool_resize(0);
#endif
        tl_env.stringpool.hashtable = nullptr;
        tl_env.stringpool.count     = 0;
        tl_env.stringpool.slot      = 0;
    } else {
        tl_panic(TL_PANIC_MEMORY, "stringpool deinit but not empty!");
    }
}


tl_string_t* tl_string_new(const char* cstr)
{
    if (cstr == nullptr)
        return nullptr;

    tl_string_t* s    = nullptr;
    tl_size_t    len  = strlen(cstr);
    uint32_t     hash = string_hash(cstr, len);

#if TL_STRING_INTERALIZE_ENABLE
    // find if current string existed in string pool
    string_pool_t* pool = &tl_env.stringpool;
    tl_size_t      idx  = util_mod(hash, pool->slot);

    for (s = pool->hashtable[idx]; s != nullptr; s = s->next) {
        if (s->len == len && s->hash == hash && memcmp(s->cstr, cstr, len) == 0) {
            // found cstr
            string_info("string <%s>:`%s` cache hit", TL_DEBUG_OBJNAME(s), s->cstr);
            return s;
        }
    }
#endif

    // not found, create a new string
    tl_gcobj_t* gco = tl_gc_newobj(TL_TYPE_STRING, strlen2size(len));

    s = (tl_string_t*)gco;
    memcpy(s->cstr, cstr, len);
    s->cstr[len] = '\0';
    s->len       = len;
    s->hash      = hash;
    s->extra     = 0;

    string_info("string<%s> init, len=%d, size=%d, hash=0x%08x", TL_DEBUG_OBJNAME(s), len, strlen2size(len), hash);

#if TL_STRING_INTERALIZE_ENABLE
    // add to string list
    s->next              = pool->hashtable[idx];
    pool->hashtable[idx] = s;

    // resize string pool
    pool->count++;
    if (pool->count >= pool->slot && pool->slot < UINT32_MAX / 2) {
        stringpool_resize(pool->slot * 2);
    }
#endif
    return s;
}


tl_string_t* tl_string_newex(const char* cstr)
{
    tl_string_t* s = tl_string_new(cstr);

    if (s == nullptr) {
        tl_panic(TL_PANIC_MEMORY, "tl_string_newex fail");
    }
    s->extra += 1;
    return s;
}
void tl_string_freeex(tl_string_t* tstr)
{
    if (tstr != 0 && tstr->extra > 0) {
        tstr->extra -= 1;
    }
}

void tl_string_free(tl_string_t* tstr)
{
#if TL_STRING_INTERALIZE_ENABLE
    // remove tstr from pool
    string_pool_t* pool = &tl_env.stringpool;
    tl_string_t**  pre  = &pool->hashtable[util_mod(tstr->hash, pool->slot)];
    tl_string_t*   s    = nullptr;

    for (s = *pre; s != nullptr; s = s->next) {
        if (s == tstr) {
            // find tstr, remove it
            *pre = s->next;
            pool->count--;
            break;
        }
        pre = &(*pre)->next;
    }

    if (pool->count < pool->slot / 2 && pool->slot > TL_STRING_POOL_SLOTS_DEFAULT) {
        stringpool_resize(pool->slot / 2);
    }
#endif

    mem_info("string<%s> free@%p, len=%d, size=%d", TL_DEBUG_OBJNAME(tstr), tstr, tstr->len, tl_string_size(tstr));

    if (s == nullptr) {
        tl_panic(TL_PANIC_MEMORY, "string<%s>@%p free failed, not find it", TL_DEBUG_OBJNAME(tstr), tstr);
    } else {
        tl_free(tstr, tl_string_size(tstr));
    }
}


void tl_stringpool_display(void)
{
    string_pool_t* pool = &tl_env.stringpool;
    tl_printf("STRINGPOOL:\n");
    for (tl_size_t i = 0; i < pool->slot; i++) {
        tl_string_t* s = pool->hashtable[i];
        if (s == nullptr) {
            continue;
        }

        tl_printf("    [%2d] ", i);

        bool first = true;
        while (s != nullptr) {
            first ? first = false : tl_printf(" -> ");
            tl_printf("'%s'<%d>", s->cstr, s->extra);   //   TL_DEBUG_OBJNAME(s)
            s = s->next;
        }
        tl_printf("\n");
    }
}

bool tl_string_eq(tl_string_t* s1, tl_string_t* s2)
{
    return (s1 == s2) || (s1->hash == s2->hash && strcmp(s1->cstr, s2->cstr) == 0);
}
