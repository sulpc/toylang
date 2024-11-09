#include "tl_values.h"

typedef struct {
    const char* vname;
    tl_vtype_t  vtype;
} tl_builtin_typevar_t;

typedef struct {
    const char*    vname;
    tl_cfunction_t cfunc;
} tl_builtin_cfunc_t;

extern tl_builtin_typevar_t tl_builtin_typevars[];
extern tl_builtin_cfunc_t   tl_builtin_cfuncs[];
