#include "tl_func.h"
#include "tl_gc.h"
#include "tl_parser.h"


#define func_debug 0
#if func_debug
#define func_info tl_none
#else
#define func_info tl_info
#endif
#define mem_info tl_none

tl_func_t* tl_func_new(void* ast)
{
    tl_gcobj_t* gco  = tl_gc_newobj(TL_TYPE_FUNCTION, sizeof(tl_func_t));
    tl_func_t*  func = (tl_func_t*)gco;
    func->ast        = ast;

    TL_DEBUG_OBJNAME_SET(func, "cfunc");
    mem_info("func<%s> new@%p", TL_DEBUG_OBJNAME(func), func);
    return func;
}

void tl_func_free(tl_func_t* func)
{
    tl_expr_t* ast = func->ast;

    ast->funcdef.in_gc = false;
    free_expr(ast);

    mem_info("func<%s> free@%p", TL_DEBUG_OBJNAME(func), func);
    tl_free(func, sizeof(tl_func_t));
}
