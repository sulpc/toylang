#ifndef _TL_GC_H_
#define _TL_GC_H_

#include "tl_env.h"
#include "tl_mem.h"
#include "tl_values.h"

void        tl_gc_init(void);
void        tl_gc_deinit(void);
void        tl_gc_cleanall(void);
tl_gcobj_t* tl_gc_newobj(tl_vtype_t vtype, tl_size_t size);
void        tl_gc_process(void);
void        tl_gc_trigger(void);
void        tl_gc_listbarrier(struct tl_list_t* list, const tl_value_t* value);
void        tl_gc_mapbarrier(struct tl_map_t* map, const tl_value_t* key, const tl_value_t* val);
void        tl_gc_display(bool on);
int32_t     tl_gc_memused(void);

#endif
