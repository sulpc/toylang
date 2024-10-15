#include "tl_map.h"
#include "tl_gc.h"
#include "tl_ops.h"

#define map_debug 0
#define map_info  tl_none
#define mem_info  tl_none

static void map_resize(tl_map_t* map, tl_size_t nsize)
{
    tl_size_t ncapacity = TL_MAP_CAPACITY_DEFALUT;

    while (ncapacity < nsize)
        ncapacity <<= 1;

    if (ncapacity == map->capacity)
        return;

    tl_node_t* odata = map->data;
    tl_size_t  osize = map->capacity;

    map->data     = tl_newvector(ncapacity, tl_node_t);
    map->capacity = ncapacity;
    map->lastfree = &map->data[ncapacity];
    map->count    = 0;

    for (tl_node_t* kv = odata; kv < odata + osize; kv++) {
        if (kv->key.vtype != TL_TYPE_INVALID)
            tl_map_set(map, &kv->key, &kv->value);
    }

    tl_freevector(odata, osize, tl_node_t);

    mem_info("map<%s> data resize@%p, capacity=%d, count=%d", TL_DEBUG_OBJNAME(map), map->data, map->capacity,
             map->count);
}


static tl_node_t* getposition(tl_map_t* map, tl_value_t* key)
{
    switch (key->vtype) {
    case TL_TYPE_TYPE:   // TODO
    case TL_TYPE_NULL:
        return &map->data[0];
    case TL_TYPE_BOOL:
        return &map->data[key->vv.b % map->capacity];
    case TL_TYPE_INT:
        return &map->data[key->vv.i % map->capacity];
    case TL_TYPE_FLOAT: {
        tl_int_t i = *(tl_int_t*)&(key->vv.n);
        return &map->data[i % map->capacity];
    }
    case TL_TYPE_STRING: {
        tl_string_t* str = (tl_string_t*)key->vv.gco;
        return &map->data[str->hash % map->capacity];
    }
    case TL_TYPE_FUNCTION:
        // TODO
    case TL_TYPE_CFUNCTION:
        // TODO
    case TL_TYPE_LIST:
    case TL_TYPE_MAP:
    case TL_TYPE_INVALID:
    default:
        // unreachable here
        return nullptr;
    }
}


static tl_node_t* getfree(tl_map_t* map)
{
    if (map->lastfree == nullptr) {
        tl_panic(TL_PANIC_MEMORY, "getfree fail");
        return nullptr;
    }

    while (true) {
        if (map->lastfree == map->data) {
            tl_panic(TL_PANIC_MEMORY, "getfree fail");
            return nullptr;
        }

        map->lastfree--;
        if (map->lastfree->key.vtype == TL_TYPE_INVALID) {
            return map->lastfree;
        }
    }
}


tl_map_t* tl_map_new(void)
{
    tl_gcobj_t* gco = tl_gc_newobj(TL_TYPE_MAP, sizeof(tl_map_t));
    tl_map_t*   map = (tl_map_t*)gco;

    map->count    = 0;
    map->capacity = 0;
    map->nextgray = nullptr;
    map->data     = nullptr;
    map->lastfree = nullptr;

    mem_info("map<%s> new@%p", TL_DEBUG_OBJNAME(map), map);

    return map;
}


void tl_map_free(tl_map_t* map)
{
    if (map->capacity > 0) {
        tl_freevector(map->data, map->capacity, tl_node_t);
        mem_info("map<%s> data free@%p, capacity=%d", TL_DEBUG_OBJNAME(map), map->data, map->capacity);
    }
    mem_info("map<%s> free@%p", TL_DEBUG_OBJNAME(map), map);
    tl_free(map, sizeof(tl_map_t));
}


tl_value_t* tl_map_set(tl_map_t* map, tl_value_t* key, tl_value_t* value)
{
    tl_value_t* v = tl_map_get(map, key);

    // key not exist
    if (v == nullptr) {
        if (map->count == map->capacity) {
            map_resize(map, map->capacity * 2);
        }

        tl_node_t* mainpos = getposition(map, key);

        // hash conflict
        if (mainpos->key.vtype != TL_TYPE_INVALID) {
            tl_node_t* newnode       = getfree(map);
            tl_node_t* mainpos_other = getposition(map, &mainpos->key);

            if (mainpos_other == mainpos) {
                // add the new node to the chain
                newnode->next = (mainpos->next == 0) ? 0 : mainpos + mainpos->next - newnode;
                mainpos->next = newnode - mainpos;
                // this position will be used by the new kv
                mainpos = newnode;
            } else {
                // other is here because of hash conflict, move it to another position
                // find prev node of this
                while (mainpos_other + mainpos_other->next != mainpos)
                    mainpos_other += mainpos_other->next;
                // move this node to new place
                tl_setvalue(&newnode->key, &mainpos->key);
                tl_setvalue(&newnode->value, &mainpos->value);
                newnode->next       = (mainpos->next == 0) ? 0 : mainpos + mainpos->next - newnode;
                mainpos_other->next = newnode - mainpos_other;
                // this position will be used by the new kv
                mainpos->next = 0;
            }
        }
        tl_setvalue(&mainpos->key, key);
        v = &mainpos->value;
        map->count++;
    }

    tl_setvalue(v, value);

    tl_gc_mapbarrier(map, key, value);

    return v;
}

tl_value_t* tl_map_get(tl_map_t* map, tl_value_t* key)
{
    if (key->vtype == TL_TYPE_INVALID || key->vtype == TL_TYPE_LIST || TL_TYPE_LIST == TL_TYPE_MAP) {
        tl_panic(TL_PANIC_TYPE, "wrong key type");
    }

    if (map->count == 0) {
        return nullptr;
    }

    tl_node_t* node = getposition(map, key);

    while (true) {
        if (tl_op_eq(&node->key, key)) {
            return &node->value;
        }

        if (node->next == 0) {   // no next
            return nullptr;
        }
        node += node->next;
    }
}


void tl_map_del(tl_map_t* map, tl_value_t* key)
{
    if (key->vtype == TL_TYPE_INVALID || key->vtype == TL_TYPE_LIST || TL_TYPE_LIST == TL_TYPE_MAP) {
        tl_panic(TL_PANIC_TYPE, "wrong key type");
    }
    if (map->count == 0) {
        return;
    }

    tl_node_t* node = getposition(map, key);
    tl_node_t* pre  = node;

    // find key
    while (true) {
        if (tl_op_eq(&node->key, key)) {
            break;
        }

        if (node->next == 0) {   // no next
            return;
        }
        pre = node;
        node += node->next;
    }

    if (node == pre) {           // the node is at mainposition
        if (node->next != 0) {   // move next node to mainposition, release next node
            node += node->next;
            tl_setvalue(&pre->key, &node->key);
            tl_setvalue(&pre->value, &node->value);
        }
    }
    pre->next       = (node->next == 0) ? 0 : (node + node->next - pre);
    node->key.vtype = TL_TYPE_INVALID;

    map->count--;

    if (map->lastfree <= node) {
        map->lastfree = node + 1;
    }

    if (map->count <= map->capacity / 2) {
        map_resize(map, map->capacity / 2);
    }
}

void tl_map_display(tl_map_t* map)
{
#if 0
    tl_printf("%s:\n", TL_DEBUG_OBJNAME(map) ? TL_DEBUG_OBJNAME(map) : "map");
    int i;
    for (i = 0; i < map->capacity; i++) {
        tl_node_t* kv = &map->data[i];
        tl_printf(kv == map->lastfree ? "   *" : "    ");
        if (kv->key.vtype != TL_TYPE_INVALID) {
            tl_printf("[%2d] -> [%2d]    ", i, i + kv->next);
            tl_printvalue(&kv->key);
            tl_printf(": ");
            tl_printvalue(&kv->value);
            tl_printf("\n");
        } else {
            tl_printf("[%2d]\n", i);
        }
    }
    if (i > 0 && map->lastfree == map->data + i) {
        tl_printf("   *\n");
    }
#else
    for (tl_node_t* kv = map->data; kv != map->data + map->capacity; kv++) {
        if (kv->key.vtype != TL_TYPE_INVALID) {
            tl_printf("    ");
            tl_printvalue(&kv->key);
            tl_printf(": ");
            tl_printvalue(&kv->value);
            tl_printf("\n");
        }
    }
#endif
}
