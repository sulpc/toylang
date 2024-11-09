#include "tl_builtin.h"
#include "tl_input.h"
#include "tl_list.h"
#include "tl_map.h"
#include "tl_string.h"

#include "util_misc.h"


#define BUILTIN_END_TAG {.vname = nullptr}


// typedef void (*tl_cfunction_t)(tl_list_t* args, tl_value_t* ret);


#define return_null()     ret->vtype = TL_TYPE_NULL, ret->vv.i = 0
#define return_int(v_)    ret->vtype = TL_TYPE_INT, ret->vv.i = (v_)
#define return_float(v_)  ret->vtype = TL_TYPE_FLOAT, ret->vv.n = (v_)
#define return_type(v_)   ret->vtype = TL_TYPE_TYPE, ret->vv.t = (v_)
#define return_string(v_) ret->vtype = TL_TYPE_STRING, ret->vv.gco = (tl_gcobj_t*)(v_)


// print(...)
static void cfunc_print(tl_list_t* args, tl_value_t* ret)
{
    for (tl_size_t i = 0; i < args->size; i++) {
        tl_value_t* v = tl_list_get(args, i);

        if (i != 0)
            tl_printf(" ");
        tl_printvalue(v);
    }
    return_null();
}

// println(...)
static void cfunc_println(tl_list_t* args, tl_value_t* ret)
{
    cfunc_print(args, ret);
    tl_printf("\n");
}

// type(x)
static void cfunc_type(tl_list_t* args, tl_value_t* ret)
{
    if (args->size != 0) {
        tl_value_t* v = tl_list_get(args, 0);
        return_type(v->vtype);
    } else {
        return_type(TL_TYPE_INVALID);
    }
}

// input('input width', int)
static void cfunc_input(tl_list_t* args, tl_value_t* ret)
{
    char buf[128];
    if (args->size != 0) {
        tl_value_t* v = tl_list_get(args, 0);
        tl_printvalue(v);
        tl_getline(buf, sizeof(buf));

        if (args->size >= 2) {
            v = tl_list_get(args, 1);

            if (v->vtype == TL_TYPE_TYPE) {
                if (v->vv.t == TL_TYPE_INT) {
                    return_int(atoi(buf));
                    return;
                } else if (v->vv.t == TL_TYPE_FLOAT) {
                    return_float(atof(buf));
                    return;
                }
                tl_printf("invalid type");
            }
        }
        return_string(tl_string_new(buf));
    } else {
        return_null();
    }
}


tl_builtin_typevar_t tl_builtin_typevars[] = {
    {.vname = "null", .vtype = TL_TYPE_NULL},       {.vname = "bool", .vtype = TL_TYPE_BOOL},
    {.vname = "int", .vtype = TL_TYPE_INT},         {.vname = "float", .vtype = TL_TYPE_FLOAT},
    {.vname = "string", .vtype = TL_TYPE_STRING},   {.vname = "list", .vtype = TL_TYPE_LIST},
    {.vname = "map", .vtype = TL_TYPE_MAP},         {.vname = "func", .vtype = TL_TYPE_FUNCTION},
    {.vname = "cfunc", .vtype = TL_TYPE_CFUNCTION}, BUILTIN_END_TAG,
};

tl_builtin_cfunc_t tl_builtin_cfuncs[] = {
    {.vname = "println", .cfunc = cfunc_println},
    {.vname = "print", .cfunc = cfunc_print},
    {.vname = "type", .cfunc = cfunc_type},
    {.vname = "input", .cfunc = cfunc_input},
    BUILTIN_END_TAG,
};
