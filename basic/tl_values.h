#ifndef _TL_VALUES_H_
#define _TL_VALUES_H_

#include "tl_types.h"
#include "tl_utils.h"

typedef enum {
    TL_TYPE_INVALID = 0,
    TL_TYPE_TYPE,
    TL_TYPE_NULL,
    TL_TYPE_BOOL,
    TL_TYPE_INT,
    TL_TYPE_FLOAT,
    TL_TYPE_GCO_START,
    TL_TYPE_STRING = TL_TYPE_GCO_START,
    TL_TYPE_LIST,
    TL_TYPE_MAP,
    TL_TYPE_FUNCTION,
    TL_TYPE_GCO_END = TL_TYPE_FUNCTION,
    TL_TYPE_CFUNCTION,
    TL_TYPE_REF,
    // TL_TYPE_CONST = 0x80,
} tl_vtype_t;

// #define tl_vtype_const(vtype)   (vtype | TL_TYPE_CONST)
// #define tl_vtype_unconst(vtype) (vtype & (~TL_TYPE_CONST))
// #define tl_vtype_isconst(vtype) ((vtype & TL_TYPE_CONST) != 0)

typedef struct tl_gcobj_t tl_gcobj_t;

// all gc object must has this fieled at head
#define gc_header                                                                                                      \
    tl_gcobj_t*      nextgco;                                                                                          \
    tl_vtype_t       vtype;                                                                                            \
    tl_byte_t marked TL_DEBUG_OBJNAME_FIELD

#define gc_header_container                                                                                            \
    gc_header;                                                                                                         \
    tl_gcobj_t* nextgray

struct tl_gcobj_t {
    gc_header;
};

struct tl_value_t;
typedef union tl_vv_t {
    tl_gcobj_t*        gco;   // gc types: string, list, map, ...
    bool               b;     // bool
    tl_int_t           i;     // int
    tl_float_t         n;     // float number
    tl_vtype_t         t;     // type
    struct tl_value_t* r;     // ref to tl_value_t
    void*              p;

    // void*        p;     //
    // char*        s;     //
} tl_vv_t;

// generic data type
typedef struct tl_value_t {
    tl_vv_t    vv;
    tl_vtype_t vtype;   // TL_TYPE_***
    bool       const_;
} tl_value_t;

// string gco
typedef struct tl_string_t {
    gc_header;
    tl_byte_t           extra;
    tl_size_t           hash;
    tl_size_t           len;
    struct tl_string_t* next;
    char                cstr[0];
} tl_string_t;

// container: list, map, ...

typedef struct tl_container_t {
    gc_header_container;
} tl_container_t;

// list gco

typedef struct tl_list_t {
    gc_header_container;
    tl_size_t   capacity;
    tl_size_t   size;
    tl_value_t* data;
} tl_list_t;

// map

typedef struct tl_node_t {
    tl_value_t key;
    tl_value_t value;
    int        next;   // offset to next node with same hash, 0 mean no next
} tl_node_t;

typedef struct tl_map_t {
    gc_header_container;
    tl_size_t  capacity;
    tl_size_t  count;
    tl_node_t* data;
    tl_node_t* lastfree;   // next of the real last node
} tl_map_t;

// func

typedef struct tl_func_t {
    gc_header_container;
    void* ast;
} tl_func_t;

typedef void (*tl_cfunction_t)(tl_list_t* args, tl_value_t* ret);

#define tl_value2gco(value) ((value)->vv.gco)
#define tl_isgco(value)     ((value)->vtype >= TL_TYPE_GCO_START && (value)->vtype <= TL_TYPE_GCO_END)

#ifdef THIS_IS_IN_TL_GC_C
const char* tl_type_names[] = {
    "unused", "type", "null", "bool", "int", "float", "string", "list", "map", "func", "cfunc",
};
#else
extern const char* tl_type_names[];
#endif
#define tl_typename(obj) (obj ? tl_type_names[obj->vtype] : "nullptr")

#define tl_setvalue(dst, src) (dst)->vtype = (src)->vtype, (dst)->vv = (src)->vv, (dst)->const_ = (src)->const_

void tl_printvalue(tl_value_t* v);

#endif
