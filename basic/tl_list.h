#ifndef _TL_LIST_H_
#define _TL_LIST_H_

#include "tl_env.h"
#include "tl_values.h"

#define tl_list_size(list) (sizeof(tl_list_t) + list->capacity * sizeof(tl_value_t))

tl_list_t*  tl_list_new(void);
void        tl_list_free(tl_list_t* list);
void        tl_list_pushback(tl_list_t* list, tl_value_t* value);
tl_value_t* tl_list_popback(tl_list_t* list);
tl_value_t* tl_list_set(tl_list_t* list, int index, tl_value_t* value);
tl_value_t* tl_list_get(tl_list_t* list, int index);
void        tl_list_del(tl_list_t* list, int index);
tl_list_t*  tl_list_sublist(tl_list_t* list, int start, tl_size_t size);
void        tl_list_display(tl_list_t* list);

#endif
