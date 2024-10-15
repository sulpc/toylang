#ifndef _TL_ENV_H_
#define _TL_ENV_H_

#include "tl_values.h"


typedef struct string_pool_t {
    tl_string_t** hashtable;
    uint32_t      slot;
    uint32_t      count;
} string_pool_t;

typedef enum {
    TL_SCOPE_TYPE_GLOBAL,
    TL_SCOPE_TYPE_BLOCK,
    TL_SCOPE_TYPE_LOOP,
    TL_SCOPE_TYPE_FUNCTION,
} tl_scope_type_t;

typedef enum {
    TL_SCOPE_STATE_NORMAL,
    TL_SCOPE_STATE_BREAKED,
    TL_SCOPE_STATE_CONTINUED,
    TL_SCOPE_STATE_RETURNED,
} tl_scope_state_t;

typedef struct tl_scope_t {
    const char*        name;
    int                level;
    tl_map_t*          members;
    struct tl_scope_t* prev;
    tl_scope_type_t    type;
    tl_scope_state_t   state;
    tl_value_t         ret;
} tl_scope_t;

typedef struct tl_env_t {
    // global table
    tl_value_t global_list;   // to-delete
    // string pool
    string_pool_t stringpool;   // stringpool not managed by gc, it's more basic
    // gc fileds
    uint8_t      gc_state;          // gc state machine
    uint8_t      gc_currentwhite;   // which white now, changed after atomic
    tl_gcobj_t*  gc_all;            // all gc obj, new obj add into head
    tl_gcobj_t*  gc_gray;           // gray list
    tl_gcobj_t*  gc_grayagain;      // grayagain list, for list, map, etc
    tl_gcobj_t** gc_sweepptr;       // traverse allgc list in sweep step, use double pointer to delete node
    tl_int_t     gc_debt;           // debt>0, trigger gc; usually negtive
    tl_int_t     gc_debt_last;      //
    tl_int_t     gc_threshold;      // gc threshold, the real mem size is threshold+debt
    tl_size_t    gc_processed;      // processed mem size each gc
    bool         gc_inited;
    // activation field
    tl_scope_t* cur_scope;
    tl_value_t  scopes;
    // tl_value_t  arstack;
} tl_env_t;


void tl_env_init(void);
void tl_env_deinit(void);
void tl_env_display(void);
void tl_scope_display(bool on);


extern tl_env_t tl_env;

#endif
