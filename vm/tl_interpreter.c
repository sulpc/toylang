#include "tl_interpreter.h"
#include "tl_builtin.h"
#include "tl_func.h"
#include "tl_list.h"
#include "tl_map.h"
#include "tl_mem.h"
#include "tl_ops.h"
#include "tl_string.h"

#define scope_debug 0
#if scope_debug
#define scope_info tl_info
#else
#define scope_info tl_none
#endif

#define interpreter_debug 0
#if interpreter_debug
#define interpreter_info tl_info
#else
#define interpreter_info tl_none
#endif

#define panic(eno, ast, msg) tl_panic(eno, "%s at line %d", msg, (ast)->line)

static void exec_stat(tl_stat_t* stat);
static void eval_expr(tl_expr_t* expr, tl_value_t* value, bool deref);

static void exec_delstat(tl_stat_t* stat);
static void exec_blockstat(tl_stat_t* stat);
static void exec_vardeclstat(tl_stat_t* stat);
static void exec_ifstat(tl_stat_t* stat);
static void exec_switchstat(tl_stat_t* stat);
static void exec_repeatstat(tl_stat_t* stat);
static void exec_whilestat(tl_stat_t* stat);
static void exec_forloopstat(tl_stat_t* stat);
static void exec_foreachstat(tl_stat_t* stat);
static void exec_breakstat(tl_stat_t* stat);
static void exec_continuestat(tl_stat_t* stat);
static void exec_returnstat(tl_stat_t* stat);
#if PRINT_STAT_ENABLE
static void exec_printstat(tl_stat_t* stat);
#endif
static void exec_assignstat(tl_stat_t* stat);
static void exec_compoundassignstat(tl_stat_t* stat);
static void exec_funccallstat(tl_stat_t* stat);
static void eval_funcdef(tl_expr_t* expr, tl_value_t* value);
static void eval_funccallexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_selectexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_binopexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_uniopexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_listctorexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_mapctorexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_setctorexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_accessexpr(tl_expr_t* expr, tl_value_t* value);
static void eval_name(tl_expr_t* expr, tl_value_t* value);
static void eval_funccall(struct tl_funccall_t* funccall, tl_value_t* value);


static tl_scope_t* scope_new(const char* name, tl_scope_type_t type);
static void        scope_del(tl_scope_t* scope);
static void        scope_enter(tl_scope_t* scope);
static void        scope_exit(void);
static void        scope_set(tl_scope_t* scope, tl_string_t* id, tl_value_t* value, bool const_);
static tl_value_t* scope_get(tl_scope_t* scope, tl_string_t* id);

/**
 * @brief scope interrupt(return/break/continue) handler
 *
 * @param scope
 * @return bool - ternimal or not
 */
static bool scope_int_handle(tl_scope_t* scope)
{
    tl_scope_state_t sstate = scope->state;

    scope->state = TL_SCOPE_STATE_NORMAL;

    switch (scope->type) {
    case TL_SCOPE_TYPE_BLOCK:
        if (sstate == TL_SCOPE_STATE_RETURNED) {
            interpreter_info("scope<%s> pass return", scope->name);
            return true;
        } else if (sstate == TL_SCOPE_STATE_BREAKED) {
            interpreter_info("scope<%s> pass break", scope->name);
            return true;
        } else if (sstate == TL_SCOPE_STATE_CONTINUED) {
            interpreter_info("scope<%s> pass continue", scope->name);
            return true;
        }
        break;

    case TL_SCOPE_TYPE_LOOP:
        if (sstate == TL_SCOPE_STATE_RETURNED) {
            interpreter_info("scope<%s> pass return", scope->name);
            return true;
        } else if (sstate == TL_SCOPE_STATE_BREAKED) {
            interpreter_info("scope<%s> handle break", scope->name);
            return true;
        } else if (sstate == TL_SCOPE_STATE_CONTINUED) {
            interpreter_info("scope<%s> handle continue", scope->name);
            return false;   // do nothing
        }
        break;

    case TL_SCOPE_TYPE_FUNCTION:
        if (sstate == TL_SCOPE_STATE_RETURNED) {
            interpreter_info("scope<%s> handle return", scope->name);
            return true;
        }
        break;

    default:
        break;
    }
    return false;
}


static void init_builtins(tl_scope_t* scope)
{
    tl_string_t* name;
    tl_value_t   value;

    for (tl_builtin_typevar_t* typevar = tl_builtin_typevars; typevar->vname != nullptr; typevar++) {
        value.vtype = TL_TYPE_TYPE;
        value.vv.t  = typevar->vtype;
        name        = tl_string_new(typevar->vname);
        scope_set(scope, name, &value, true);
    }
    for (tl_builtin_cfunc_t* cfunc = tl_builtin_cfuncs; cfunc->vname != nullptr; cfunc++) {
        value.vtype = TL_TYPE_CFUNCTION;
        value.vv.p  = cfunc->cfunc;
        name        = tl_string_new(cfunc->vname);
        scope_set(scope, name, &value, true);
    }
}


void tl_interpreter_start()
{
    // scope init
    tl_scope_t* scope = scope_new("global", TL_SCOPE_TYPE_GLOBAL);
    init_builtins(scope);
    scope_enter(scope);
}

void tl_interpreter_exit()
{
    tl_scope_t* scope = tl_env.cur_scope;
    scope_exit();
    scope_del(scope);
}

void tl_interpreter_interpret(tl_program_t* program)
{
    for (tl_stat_t* s = program->stats; s != nullptr; s = s->next) {
        exec_stat(s);
    }
}


static tl_scope_t* scope_new(const char* name, tl_scope_type_t type)
{
    tl_scope_t* scope = tl_new(sizeof(tl_scope_t));
    scope->name       = name;
    scope->members    = tl_map_new();
    scope->prev       = nullptr;
    scope->level      = 0;
    scope->state      = TL_SCOPE_STATE_NORMAL;
    scope->type       = type;
    scope->ret.vtype  = TL_TYPE_NULL;
    scope->ret.vv.i   = 0;

    TL_DEBUG_OBJNAME_SET(scope->members, "scope");
    return scope;
}
static void scope_del(tl_scope_t* scope)
{
    // tl_map_free(scope->members);    // free by gc
    tl_free(scope, sizeof(tl_scope_t));
}
static void scope_enter(tl_scope_t* scope)
{
    scope_info("----------------------------------------------------------------------");
    scope_info("will enter new scope: %s", scope->name);
    tl_scope_display(scope_debug);
    if (tl_env.cur_scope == nullptr) {
        scope->level = 0;
    } else {
        scope->level = tl_env.cur_scope->level + 1;
    }

    scope->prev      = tl_env.cur_scope;
    tl_env.cur_scope = scope;

    tl_scope_display(scope_debug);
    scope_info("----------------------------------------------------------------------");

    tl_value_t d = {.vtype = TL_TYPE_MAP, .vv.gco = (tl_gcobj_t*)scope->members};
    tl_list_pushback((tl_list_t*)tl_env.scopes.vv.gco, &d);
}
static void scope_exit(void)
{
    scope_info("----------------------------------------------------------------------");
    scope_info("will exit scope: %s", tl_env.cur_scope->name);

    tl_scope_display(scope_debug);
    tl_list_popback((tl_list_t*)tl_env.scopes.vv.gco);

    tl_env.cur_scope = tl_env.cur_scope->prev;
    tl_scope_display(scope_debug);

    scope_info("----------------------------------------------------------------------");
}
static void scope_set(tl_scope_t* scope, tl_string_t* id, tl_value_t* value, bool const_)
{
    tl_value_t k, v;
    k.vtype  = TL_TYPE_STRING;
    k.vv.gco = (tl_gcobj_t*)id;
    v.vtype  = value->vtype;
    v.vv     = value->vv;
    v.const_ = const_;

    tl_map_set(scope->members, &k, &v);
}
static tl_value_t* scope_get(tl_scope_t* scope, tl_string_t* id)
{
    tl_value_t k;
    k.vtype  = TL_TYPE_STRING;
    k.vv.gco = (tl_gcobj_t*)id;

    return tl_map_get(scope->members, &k);
}


static void exec_stat(tl_stat_t* stat)
{
    if (stat == nullptr) {
        return;
    }

    switch (stat->type) {
    case AST_EMPTYSTAT:
        break;
    case AST_DELSTAT:
        exec_delstat(stat);
        break;
    case AST_BLOCKSTAT:
        exec_blockstat(stat);
        break;
    case AST_VARDECLSTAT:
        exec_vardeclstat(stat);
        break;
    case AST_IFSTAT:
        exec_ifstat(stat);
        break;
    case AST_SWITCHSTAT:
        exec_switchstat(stat);
        break;
    case AST_REPEATSTAT:
        exec_repeatstat(stat);
        break;
    case AST_WHILESTAT:
        exec_whilestat(stat);
        break;
    case AST_FORLOOPSTAT:
        exec_forloopstat(stat);
        break;
    case AST_FOREACHSTAT:
        exec_foreachstat(stat);
        break;
    case AST_BREAKSTAT:
        exec_breakstat(stat);
        break;
    case AST_CONTINUESTAT:
        exec_continuestat(stat);
        break;
    case AST_RETURNSTAT:
        exec_returnstat(stat);
        break;
#if PRINT_STAT_ENABLE
    case AST_PRINTSTAT:
        exec_printstat(stat);
        break;
#endif
    case AST_ASSIGNSTAT:
        exec_assignstat(stat);
        break;
    case AST_COMPOUNDASSIGNSTAT:
        exec_compoundassignstat(stat);
        break;
    case AST_FUNCCALLSTAT:
        exec_funccallstat(stat);
        break;
    default:
        tl_panic(TL_PANIC_INTERNAL, "stat <%d> not support", stat->type);
    }
}
static void exec_delstat(tl_stat_t* stat)
{
    panic(TL_PANIC_UNIMPLEMENT, stat, "delstat not implemented");
}
static void exec_blockstat(tl_stat_t* stat)
{
    tl_scope_t* scope = scope_new("block", TL_SCOPE_TYPE_BLOCK);
    scope_enter(scope);

    tl_trycatch(
        {
            for (tl_stat_t* s = stat->blockstat.stats; s != nullptr; s = s->next) {
                exec_stat(s);
                if (scope_int_handle(scope))
                    break;
            }
        },
        {
            scope_exit();
            scope_del(scope);
            tl_throw(current_es);
        });

    scope_exit();
    scope_del(scope);
}
static void exec_vardeclstat(tl_stat_t* stat)
{
    tl_expr_t* name = stat->vardeclstat.names;
    tl_expr_t* expr = stat->vardeclstat.exprs;

    while (name != nullptr) {
        tl_string_t* id = name->name.identifier;

        if (scope_get(tl_env.cur_scope, id) != nullptr) {
            panic(TL_PANIC_NAME, stat, "variable redefinition");
        }

        tl_value_t value;
        if (expr == nullptr) {
            value.vtype = TL_TYPE_NULL;
            value.vv.t  = TL_TYPE_NULL;
        } else {
            eval_expr(expr, &value, true);
            expr = expr->next;
        }

        scope_set(tl_env.cur_scope, id, &value, stat->vardeclstat.const_);

        name = name->next;
    }
    //   tl_scope_display();
}
static void exec_ifstat(tl_stat_t* stat)
{
    tl_expr_t* e;
    tl_stat_t* s;
    tl_value_t v;
    for (e = stat->ifstat.cond_exprs, s = stat->ifstat.stats; e != nullptr; e = e->next, s = s->next) {
        eval_expr(e, &v, true);
        if (tl_op_conv2bool(&v)) {
            exec_stat(s);
            return;
        }
    }
}
static void exec_switchstat(tl_stat_t* stat)
{
    tl_value_t vexpr, vcase;
    tl_expr_t* e = stat->switchstat.case_exprs;
    tl_stat_t* s = stat->switchstat.case_stats;

    eval_expr(stat->switchstat.expr, &vexpr, true);

    for (; e != nullptr; e = e->next, s = s->next) {
        eval_expr(e, &vcase, true);
        if (tl_op_eq(&vexpr, &vcase)) {
            exec_stat(s);
            return;
        }
    }
    if (stat->switchstat.default_stat) {
        exec_stat(stat->switchstat.default_stat);
    }
}
static void exec_repeatstat(tl_stat_t* stat)
{
    tl_value_t  v;
    tl_scope_t* scope = scope_new("repeat", TL_SCOPE_TYPE_LOOP);
    scope_enter(scope);

    tl_trycatch(
        {
            while (true) {
                exec_stat(stat->repeatstat.stat);
                if (scope_int_handle(scope))
                    break;

                eval_expr(stat->repeatstat.expr, &v, true);
                if (tl_op_conv2bool(&v)) {
                    break;
                }
            }
        },
        {
            scope_exit();
            scope_del(scope);
            tl_throw(current_es);
        });

    scope_exit();
    scope_del(scope);
}
static void exec_whilestat(tl_stat_t* stat)
{
    tl_value_t  v;
    tl_scope_t* scope = scope_new("while", TL_SCOPE_TYPE_LOOP);
    scope_enter(scope);

    tl_trycatch(
        {
            while (true) {
                eval_expr(stat->whilestat.expr, &v, true);

                if (tl_op_conv2bool(&v)) {
                    exec_stat(stat->whilestat.stat);
                    if (scope_int_handle(scope))
                        break;

                } else {
                    break;
                }
            }
        },
        {
            scope_exit();
            scope_del(scope);
            tl_throw(current_es);
        });

    scope_exit();
    scope_del(scope);
}
static void exec_forloopstat(tl_stat_t* stat)
{
    tl_value_t  start, end, step, temp;
    tl_scope_t* scope = scope_new("forloop", TL_SCOPE_TYPE_LOOP);

    // cal value of start, end, step
    eval_expr(stat->forloopstat.start_expr, &start, true);
    if (start.vtype != TL_TYPE_INT && start.vtype != TL_TYPE_FLOAT) {
        scope_del(scope);
        tl_panic(TL_PANIC_TYPE, "expr not a number");
    }
    eval_expr(stat->forloopstat.end_expr, &end, true);
    if (end.vtype != TL_TYPE_INT && end.vtype != TL_TYPE_FLOAT) {

        scope_del(scope);
        tl_panic(TL_PANIC_TYPE, "expr not a number");
    }
    if (stat->forloopstat.step_expr) {
        eval_expr(stat->forloopstat.step_expr, &step, true);
        if (step.vtype != TL_TYPE_INT && step.vtype != TL_TYPE_FLOAT) {
            scope_del(scope);
            tl_panic(TL_PANIC_TYPE, "expr not a number");
        }
    } else {
        step.vtype = TL_TYPE_INT;
        step.vv.i  = 1;
    }

    // create index var
    scope_set(scope, stat->forloopstat.var_name->name.identifier, &start, true);

    scope_enter(scope);
    tl_trycatch(
        {
            while (true) {
                tl_value_t* idx_var = scope_get(scope, stat->forloopstat.var_name->name.identifier);
                if (idx_var == nullptr) {
                    tl_panic(TL_PANIC_INTERNAL, "get idx var fail");
                }

                tl_op_binop(TOKEN_LT, idx_var, &end, &temp);
                if (tl_op_conv2bool(&temp)) {
                    exec_stat(stat->forloopstat.stat);
                    if (scope_int_handle(scope))
                        break;
                    tl_op_compoundassign(TOKEN_SELFADD, idx_var, &step);
                } else {
                    break;
                }
            }
        },
        {
            scope_exit();
            scope_del(scope);
            tl_throw(current_es);
        });
    scope_exit();
    scope_del(scope);
}
static void exec_foreachstat(tl_stat_t* stat)
{
    tl_value_t c, k, v;
    eval_expr(stat->foreachstat.expr, &c, true);
    tl_int_t idx = 0;

    tl_scope_t* scope = scope_new("foreach", TL_SCOPE_TYPE_LOOP);
    scope_enter(scope);
    tl_trycatch(
        {
            while (true) {
                tl_op_next(&c, &idx, &k, &v);
                if (idx == 0) {
                    break;
                }
                if (stat->foreachstat.val_name) {
                    scope_set(scope, stat->foreachstat.key_name->name.identifier, &k, true);
                    scope_set(scope, stat->foreachstat.val_name->name.identifier, &v, true);
                } else {
                    scope_set(scope, stat->foreachstat.key_name->name.identifier, &v, true);
                }
                exec_stat(stat->foreachstat.stat);
                if (scope_int_handle(scope))
                    break;
            }
        },
        {
            scope_exit();
            scope_del(scope);
            tl_throw(current_es);
        });
    scope_exit();
    scope_del(scope);
}
static void exec_breakstat(tl_stat_t* stat)
{
    tl_scope_t* scope = tl_env.cur_scope;
    while (scope) {
        scope->state = TL_SCOPE_STATE_BREAKED;
        interpreter_info("scope<%s> set break", scope->name);

        if (scope->type == TL_SCOPE_TYPE_LOOP) {
            // good
            return;
        } else if (scope->type == TL_SCOPE_TYPE_FUNCTION) {
            // error
            break;
        } else {
            scope = scope->prev;
        }
    }
    panic(TL_PANIC_SYNTAX, stat, "break not in loop");
}
static void exec_continuestat(tl_stat_t* stat)
{
    tl_scope_t* scope = tl_env.cur_scope;
    while (scope) {
        scope->state = TL_SCOPE_STATE_CONTINUED;
        interpreter_info("scope<%s> set continue", scope->name);

        if (scope->type == TL_SCOPE_TYPE_LOOP) {
            // good
            return;
        } else if (scope->type == TL_SCOPE_TYPE_FUNCTION) {
            // error
            break;
        } else {
            scope = scope->prev;
        }
    }
    panic(TL_PANIC_SYNTAX, stat, "continue not in loop");
}
static void exec_returnstat(tl_stat_t* stat)
{
    tl_scope_t* scope = tl_env.cur_scope;
    while (scope) {
        scope->state = TL_SCOPE_STATE_RETURNED;
        interpreter_info("scope<%s> set return", scope->name);

        if (scope->type == TL_SCOPE_TYPE_FUNCTION) {
            if (stat->returnstat.expr) {
                // set return value
                eval_expr(stat->returnstat.expr, &scope->ret, true);
            }
            return;
        } else {
            scope = scope->prev;
        }
    }
    panic(TL_PANIC_SYNTAX, stat, "return not in function");
}
#if PRINT_STAT_ENABLE
static void exec_printstat(tl_stat_t* stat)
{
    tl_expr_t* expr = stat->printstat.exprs;
    tl_value_t v;
    bool       first = true;

    while (expr != nullptr) {
        if (first) {
            first = false;
        } else {
            tl_printf(" ");
        }

        eval_expr(expr, &v, true);
        tl_printvalue(&v);
        expr = expr->next;
    }
    tl_printf("\n");
}
#endif
static void exec_assignstat(tl_stat_t* stat)
{
    tl_expr_t* left  = stat->assignstat.left_exprs;
    tl_expr_t* right = stat->assignstat.right_exprs;

    while (left != nullptr) {
        tl_value_t vright = {.const_ = false};

        if (right != nullptr) {
            eval_expr(right, &vright, true);
            right = right->next;
        } else {
            vright.vtype = TL_TYPE_NULL;
            vright.vv.t  = TL_TYPE_NULL;
        }

        if (left->type == AST_NAME) {   // set name
            tl_value_t vleft;

            eval_expr(left, &vleft, false);
            if (vleft.vtype != TL_TYPE_REF) {
                tl_panic(TL_PANIC_TYPE, "left not ref");
            }
            if (vleft.vv.r->const_) {
                tl_panic(TL_PANIC_TYPE, "left is const");
            }

            vleft.vv.r->vtype = vright.vtype;
            vleft.vv.r->vv    = vright.vv;
        } else if (left->type == AST_ACCESSEXPR) {   //
            tl_value_t c, k;

            eval_expr(left->accessexpr.expr, &c, true);

            if (left->accessexpr.dot) {
                k.vtype  = TL_TYPE_STRING;
                k.vv.gco = (tl_gcobj_t*)left->accessexpr.field_expr->name.identifier;
            } else {
                eval_expr(left->accessexpr.field_expr, &k, true);
            }

            tl_op_setmember(&c, &k, &vright);
        } else {
            panic(TL_PANIC_INTERNAL, stat, "should not occur");
        }
        left = left->next;
    }

    // tl_scope_display();
}
static void exec_compoundassignstat(tl_stat_t* stat)
{
    tl_value_t a, b;

    eval_expr(stat->compoundassignstat.left_expr, &a, false);
    if (a.vtype != TL_TYPE_REF) {
        tl_panic(TL_PANIC_TYPE, "left expr not ref");
    }

    eval_expr(stat->compoundassignstat.right_expr, &b, true);

    tl_op_compoundassign(stat->compoundassignstat.opt, &a, &b);
}
static void exec_funccallstat(tl_stat_t* stat)
{
    tl_value_t v;
    eval_funccall(&stat->funccall, &v);
}

static void eval_expr(tl_expr_t* expr, tl_value_t* value, bool deref)
{
    if (expr == nullptr) {
        return;
    }

    switch (expr->type) {
    case AST_FUNCDEF:
        eval_funcdef(expr, value);
        break;
    case AST_FUNCCALLEXPR:
        eval_funccallexpr(expr, value);
        break;
    case AST_SELECTEXPR:
        eval_selectexpr(expr, value);
        break;
    case AST_BINOPEXPR:
        eval_binopexpr(expr, value);
        break;
    case AST_UNIOPEXPR:
        eval_uniopexpr(expr, value);
        break;
    case AST_LISTCTOREXPR:
        eval_listctorexpr(expr, value);
        break;
    case AST_MAPCTOREXPR:
        eval_mapctorexpr(expr, value);
        break;
    case AST_SETCTOREXPR:
        eval_setctorexpr(expr, value);
        break;
    case AST_ACCESSEXPR:
        eval_accessexpr(expr, value);
        break;
    case AST_NAME:
        eval_name(expr, value);
        break;
    case AST_NUMLITERAL:
    case AST_STRINGLITERAL:
    case AST_BOOLLITERAL:
    case AST_NULLLITERAL:
        value->vtype = expr->literal.value.vtype;
        value->vv    = expr->literal.value.vv;
        break;
    default:
        tl_panic(TL_PANIC_INTERNAL, "expr <%d> not support", expr->type);
    }

    if (deref && value->vtype == TL_TYPE_REF) {
        value->vtype = value->vv.r->vtype;
        value->vv    = value->vv.r->vv;
    }
}
static void eval_funcdef(tl_expr_t* expr, tl_value_t* value)
{
    expr->funcdef.in_gc = true;
    value->vtype        = TL_TYPE_FUNCTION;
    value->vv.gco       = (tl_gcobj_t*)tl_func_new((void*)expr);
}
static void eval_funccall(struct tl_funccall_t* funccall, tl_value_t* value)
{
    tl_value_t f;
    eval_expr(funccall->func_expr, &f, true);

    if (funccall->func_expr->type == AST_ACCESSEXPR && funccall->func_expr->accessexpr.dot) {
        // todo: object call, like a.func(...) => a.func(a, ...)
    }

    tl_func_t* func = (tl_func_t*)f.vv.gco;

    if (f.vtype == TL_TYPE_FUNCTION) {
        tl_expr_t*  ast   = func->ast;
        tl_scope_t* scope = scope_new("func", TL_SCOPE_TYPE_FUNCTION);

        tl_trycatch(
            {
                // init args
                tl_expr_t* arg_name  = ast->funcdef.param_names;
                tl_expr_t* arg_value = funccall->arg_exprs;
                while (arg_name) {
                    tl_value_t tmp = {.vtype = TL_TYPE_NULL};
                    if (arg_value) {
                        eval_expr(arg_value, &tmp, true);
                        arg_value = arg_value->next;
                    }
                    scope_set(scope, arg_name->name.identifier, &tmp, false);
                    arg_name = arg_name->next;
                }
            },
            {
                scope_del(scope);
                tl_throw(current_es);
            });

        scope_enter(scope);
        tl_trycatch(
            {
                for (tl_stat_t* s = ast->funcdef.body; s != nullptr; s = s->next) {
                    exec_stat(s);
                    if (scope_int_handle(scope))
                        break;
                }
            },
            {
                scope_exit();
                scope_del(scope);
                tl_throw(current_es);
            });
        scope_exit();
        scope_del(scope);

        value->vtype = scope->ret.vtype;
        value->vv    = scope->ret.vv;

        return;
    } else if (f.vtype == TL_TYPE_CFUNCTION) {
        tl_list_t* args = tl_list_new();

        tl_expr_t* arg_expr = funccall->arg_exprs;
        while (arg_expr) {
            tl_value_t value;
            eval_expr(arg_expr, &value, true);
            arg_expr = arg_expr->next;
            tl_list_pushback(args, &value);
        }

        tl_cfunction_t pf = (tl_cfunction_t)f.vv.p;

        pf(args, value);
        return;
    }

    tl_panic(TL_PANIC_TYPE, "expr not a function");
}
static void eval_funccallexpr(tl_expr_t* expr, tl_value_t* value)
{
    eval_funccall(&expr->funccall, value);
}
static void eval_selectexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_value_t cond;
    eval_expr(expr->selectexpr.cond, &cond, true);

    if (tl_op_conv2bool(&cond)) {
        eval_expr(expr->selectexpr.expr1, value, true);
    } else {
        eval_expr(expr->selectexpr.expr2, value, true);
    }
}
static void eval_binopexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_value_t a, b;
    eval_expr(expr->binopexpr.left_expr, &a, true);
    eval_expr(expr->binopexpr.right_expr, &b, true);
    tl_op_binop(expr->binopexpr.opt, &a, &b, value);
}
static void eval_uniopexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_value_t v;
    eval_expr(expr->uniopexpr.expr, &v, true);
    tl_op_uniop(expr->binopexpr.opt, &v, value);
}
static void eval_listctorexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_list_t* l = tl_list_new();
    tl_value_t v;

    for (tl_expr_t* e = expr->listctorexpr.exprs; e != nullptr; e = e->next) {
        eval_expr(e, &v, true);
        tl_list_pushback(l, &v);
    }

    value->vtype  = TL_TYPE_LIST;
    value->vv.gco = (tl_gcobj_t*)l;
}
static void eval_mapctorexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_map_t*  m = tl_map_new();
    tl_value_t k, v;
    tl_expr_t* ke;
    tl_expr_t* ve;
    for (ke = expr->mapctorexpr.key_exprs, ve = expr->mapctorexpr.value_exprs; ke != nullptr;
         ke = ke->next, ve = ve->next) {
        eval_expr(ke, &k, true);
        eval_expr(ve, &v, true);
        tl_map_set(m, &k, &v);
    }

    value->vtype  = TL_TYPE_MAP;
    value->vv.gco = (tl_gcobj_t*)m;
}
static void eval_setctorexpr(tl_expr_t* expr, tl_value_t* value)
{
    panic(TL_PANIC_UNIMPLEMENT, expr, "setctorexpr not implemented");
}
static void eval_accessexpr(tl_expr_t* expr, tl_value_t* value)
{
    tl_value_t c, k;
    eval_expr(expr->accessexpr.expr, &c, true);
    if (expr->accessexpr.dot) {
        k.vtype  = TL_TYPE_STRING;
        k.vv.gco = (tl_gcobj_t*)expr->accessexpr.field_expr->name.identifier;
    } else {
        eval_expr(expr->accessexpr.field_expr, &k, true);
    }
    tl_op_getmember(&c, &k, value);
}
static void eval_name(tl_expr_t* expr, tl_value_t* value)
{
    tl_scope_t*  scope = tl_env.cur_scope;
    tl_string_t* id    = expr->name.identifier;

    while (scope) {
        tl_value_t* old = scope_get(scope, id);
        if (old) {
            value->vtype = TL_TYPE_REF;
            value->vv.r  = old;
            return;
        } else {
            scope = scope->prev;
        }
    }
    panic(TL_PANIC_NAME, expr, "name not found");
}
