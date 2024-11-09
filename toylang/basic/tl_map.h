#ifndef _TL_MAP_H_
#define _TL_MAP_H_

#include "tl_env.h"
#include "tl_values.h"

#define tl_map_size(map) (sizeof(tl_map_t) + map->capacity * sizeof(tl_node_t))

tl_map_t*   tl_map_new(void);
void        tl_map_free(tl_map_t* map);
tl_value_t* tl_map_set(tl_map_t* map, tl_value_t* key, tl_value_t* value);
tl_value_t* tl_map_get(tl_map_t* map, tl_value_t* key);
void        tl_map_del(tl_map_t* map, tl_value_t* key);
void        tl_map_display(tl_map_t* map);

#endif
