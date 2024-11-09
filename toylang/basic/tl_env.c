#include "tl_env.h"
#include "tl_gc.h"
#include "tl_list.h"
#include "tl_map.h"
#include "tl_string.h"


tl_env_t tl_env;


void tl_env_init(void)
{
    // init string pool
    tl_stringpool_init();

    // init gc fields
    tl_gc_init();

    tl_env.cur_scope     = nullptr;
    tl_env.scopes.vtype  = TL_TYPE_LIST;
    tl_env.scopes.vv.gco = (tl_gcobj_t*)tl_list_new();
    TL_DEBUG_OBJNAME_SET(tl_env.scopes.vv.gco, "scopes");
    // tl_env.arstack.vtype     = TL_TYPE_LIST;
    // tl_env.arstack.vv.gco    = (tl_gcobj_t*)tl_list_new();
}


void tl_env_deinit(void)
{
    // deinit globals
    tl_env.scopes.vtype = TL_TYPE_NULL;

    // free all gco
    if (tl_env.gc_debt + tl_env.gc_threshold > 0) {
        tl_gc_deinit();
    }

    // free string pool
    tl_stringpool_deinit();
}


void tl_env_display(void)
{
    tl_printf("================ ENV INFO ==============\n");
    tl_gc_display(true);
    tl_scope_display(true);

    tl_stringpool_display();
    tl_meminfo_display();
    tl_printf("========================================\n\n");
}

void tl_scope_display(bool on)
{
    if (!on)
        return;

    tl_printf("SCOPES:\n");
    tl_scope_t* scope = tl_env.cur_scope;
    while (scope != nullptr) {
        tl_printf("-------------------- SCOPE [%d]\n", scope->level);
        tl_map_display(scope->members);
        scope = scope->prev;
    }
}
