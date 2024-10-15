#include "tl_list.h"
#include "tl_gc.h"

#define list_debug 0
#define list_info  tl_none
#define mem_info   tl_none

static void list_resize(tl_list_t* list, tl_size_t nsize)
{
    tl_size_t ncapacity = TL_LIST_CAPACITY_DEFALUT;

    while (ncapacity < nsize)
        ncapacity <<= 1;

    if (ncapacity == list->capacity)
        return;

    list->data = tl_reallocvector(list->data, list->capacity, ncapacity, tl_value_t);

    mem_info("list<%s> data resize@%p, capacity %d->%d", TL_DEBUG_OBJNAME(list), list->data, list->capacity, ncapacity);
    list->capacity = ncapacity;
}


tl_list_t* tl_list_new(void)
{
    tl_gcobj_t* gco  = tl_gc_newobj(TL_TYPE_LIST, sizeof(tl_list_t));
    tl_list_t*  list = (tl_list_t*)gco;

    list->size     = 0;
    list->capacity = 0;
    list->data     = nullptr;
    list->nextgray = nullptr;

    mem_info("list<%s> new@%p", TL_DEBUG_OBJNAME(list), list);

    return list;
}


void tl_list_free(tl_list_t* list)
{
    if (list->capacity > 0) {
        tl_freevector(list->data, list->capacity, tl_value_t);
        mem_info("list<%s> data free@%p, capacity=%d", TL_DEBUG_OBJNAME(list), list->data, list->capacity);
    }
    mem_info("list<%s> free@%p", TL_DEBUG_OBJNAME(list), list);
    tl_free(list, sizeof(tl_list_t));
}


void tl_list_pushback(tl_list_t* list, tl_value_t* value)
{
    if (list->size >= list->capacity) {
        list_resize(list, list->capacity * 2);
    }
    list->data[list->size].vtype = value->vtype;
    list->data[list->size].vv    = value->vv;

    // tl_printf("[INF] list<%s> set [%d] = ", list->name__, list->size), tl_printvalue(value), tl_printf("\n");

    tl_gc_listbarrier(list, value);

    list->size++;
}


tl_value_t* tl_list_popback(tl_list_t* list)
{
    tl_value_t* last = nullptr;
    if (list->size > 0) {
        last = &list->data[list->size - 1];
        list->size--;
    }
    if (list->size <= list->capacity / 2) {
        list_resize(list, list->capacity / 2);
    }

    return last;
}


tl_value_t* tl_list_set(tl_list_t* list, int index, tl_value_t* value)
{
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (tl_size_t)index >= list->size) {
        return nullptr;
    }
    list->data[index].vtype = value->vtype;
    list->data[index].vv    = value->vv;

    // tl_printf("[INF] list<%s> set [%d]= ", list->name__, index), tl_printvalue(value), tl_printf("\n");

    tl_gc_listbarrier(list, value);

    return &list->data[index];
}


tl_value_t* tl_list_get(tl_list_t* list, int index)
{
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (tl_size_t)index >= list->size) {
        return nullptr;
    }
    return &list->data[index];
}

void tl_list_del(tl_list_t* list, int index)
{
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (tl_size_t)index >= list->size) {
        return;
    }

    for (int i = index; i < list->size - 1; i++) {
        tl_setvalue(&list->data[i], &list->data[i + 1]);
    }
    list->size--;

    if (list->size <= list->capacity / 2) {
        list_resize(list, list->capacity / 2);
    }
}

tl_list_t* tl_list_sublist(tl_list_t* list, int start, tl_size_t size)
{
    if (start < 0) {
        start += list->size;
    }
    if (start < 0 || (tl_size_t)start >= list->size) {
        return nullptr;
    }
    tl_list_t* sublist = tl_list_new();
    list_resize(sublist, size);
    memcpy(sublist->data, &list->data[start], size * sizeof(tl_value_t));
    return sublist;
}


void tl_list_display(tl_list_t* list)
{
    tl_value_t val;
    val.vtype  = TL_TYPE_LIST;
    val.vv.gco = (tl_gcobj_t*)list;

    tl_printf("%s = ", TL_DEBUG_OBJNAME(list));
    tl_printvalue(&val);
    tl_printf("\n");
}
