#include "tl_parser.h"
#include "tl_string.h"

#define newline_indent(n)                                                                                              \
    do {                                                                                                               \
        tl_printf("\n");                                                                                               \
        for (int i__ = 0; i__ < (n); i__++)                                                                            \
            tl_printf("    ");                                                                                         \
    } while (0)

static void dump_stat(tl_stat_t* stats, int level, bool newline);
static void dump_expr(tl_expr_t* expr, int level);
static void dump_statlist(tl_stat_t* stats, int level);
static void dump_exprlist(tl_expr_t* expr, int level);

void tl_parser_dump(tl_program_t* program)
{
    for (tl_stat_t* s = program->stats; s != nullptr; s = s->next) {
        dump_stat(s, 0, true);
    }
    tl_printf("\n");
}

static void dump_stat(tl_stat_t* stat, int level, bool newline)
{
    tl_expr_t* c;
    tl_stat_t* s;

    if (stat == nullptr) {
        return;
    }

    if (newline) {
        newline_indent(level);
    }

    switch (stat->type) {
    case AST_EMPTYSTAT:
        tl_printf(";");
        break;
    case AST_DELSTAT:   // TODO
        tl_printf("del ...");
        break;
    case AST_BLOCKSTAT:
        tl_printf("{");
        dump_statlist(stat->blockstat.stats, level + 1);
        newline_indent(level);
        tl_printf("}");
        break;
    case AST_VARDECLSTAT: {
        tl_printf(stat->vardeclstat.const_ ? "const " : "var ");
        dump_exprlist(stat->vardeclstat.names, level);
        if (stat->vardeclstat.exprs != nullptr) {
            tl_printf(" = ");
            dump_exprlist(stat->vardeclstat.exprs, level);
        }
    } break;
    case AST_IFSTAT:
        c = stat->ifstat.cond_exprs;
        s = stat->ifstat.stats;

        tl_printf("if ");
        dump_expr(c, level);
        tl_printf(": ");
        dump_stat(s, level, false);
        c = c->next;
        s = s->next;

        while (c != nullptr && c->type != AST_BOOLLITERAL) {
            newline_indent(level);
            tl_printf("elif ");
            dump_expr(c, level);
            tl_printf(": ");
            dump_stat(s, level, false);
            c = c->next;
            s = s->next;
        }

        if (c != nullptr && c->type == AST_BOOLLITERAL) {
            newline_indent(level);
            tl_printf("else: ");
            dump_stat(s, level, false);
        }

        break;
    case AST_SWITCHSTAT:

        tl_printf("switch ");
        dump_expr(stat->switchstat.expr, level);
        tl_printf(":");

        c = stat->switchstat.case_exprs;
        s = stat->switchstat.case_stats;

        while (c != nullptr) {
            newline_indent(level);
            tl_printf("case ");
            dump_expr(c, level);
            tl_printf(": ");
            dump_stat(s, level, false);
            c = c->next;
            s = s->next;
        }

        s = stat->switchstat.default_stat;
        if (s != nullptr) {
            newline_indent(level);
            tl_printf("default: ");
            dump_stat(s, level, false);
        }
        break;
    case AST_REPEATSTAT:
        tl_printf("repeat: ");
        dump_stat(stat->repeatstat.stat, level, false);
        newline_indent(level);
        tl_printf("until ");
        dump_expr(stat->repeatstat.expr, level);
        break;
    case AST_WHILESTAT:
        tl_printf("while ");
        dump_expr(stat->whilestat.expr, level);
        tl_printf(": ");
        dump_stat(stat->whilestat.stat, level, false);
        break;
    case AST_FORLOOPSTAT:
        tl_printf("for ");
        dump_expr(stat->forloopstat.var_name, level);
        tl_printf(" is ");
        dump_expr(stat->forloopstat.start_expr, level);
        tl_printf(", ");
        dump_expr(stat->forloopstat.end_expr, level);
        if (stat->forloopstat.step_expr != nullptr) {
            tl_printf(", ");
            dump_expr(stat->forloopstat.step_expr, level);
        }
        tl_printf(": ");
        dump_stat(stat->forloopstat.stat, level, false);
        break;
    case AST_FOREACHSTAT:
        tl_printf("for ");
        dump_expr(stat->foreachstat.key_name, level);
        if (stat->foreachstat.val_name != nullptr) {
            tl_printf(", ");
            dump_expr(stat->foreachstat.val_name, level);
        }
        tl_printf(" in ");
        dump_expr(stat->foreachstat.expr, level);
        tl_printf(": ");
        dump_stat(stat->foreachstat.stat, level, false);
        break;
    case AST_BREAKSTAT:
        tl_printf("break");
        break;
    case AST_CONTINUESTAT:
        tl_printf("continue");
        break;
    case AST_RETURNSTAT:
        tl_printf("return");
        if (stat->returnstat.expr != nullptr) {
            tl_printf(" ");
            dump_expr(stat->returnstat.expr, level);
        }
        break;
#if PRINT_STAT_ENABLE
    case AST_PRINTSTAT:
        tl_printf("print ");
        dump_exprlist(stat->printstat.exprs, level);
        break;
#endif
    case AST_ASSIGNSTAT:
        dump_exprlist(stat->assignstat.left_exprs, level);
        tl_printf(" = ");
        dump_exprlist(stat->assignstat.right_exprs, level);
        break;
    case AST_COMPOUNDASSIGNSTAT:
        dump_expr(stat->compoundassignstat.left_expr, level);
        tl_printf(" %s ", tl_tokenstr(stat->compoundassignstat.opt));
        dump_expr(stat->compoundassignstat.right_expr, level);
        break;
    case AST_FUNCCALLSTAT:
        dump_expr(stat->funccall.func_expr, level);
        tl_printf("(");
        dump_exprlist(stat->funccall.arg_exprs, level);
        tl_printf(")");
        break;
    default:
        tl_panic(TL_PANIC_SYNTAX, "stat <%d> not support", stat->type);
    }
}
static void dump_expr(tl_expr_t* expr, int level)
{
    if (expr == nullptr) {
        return;
    }

    switch (expr->type) {
    case AST_FUNCDEF:
        tl_printf("func (");
        dump_exprlist(expr->funcdef.param_names, level);
        tl_printf(") {");
        for (tl_stat_t* s = expr->funcdef.body; s != nullptr; s = s->next) {
            dump_stat(s, level + 1, true);
        }
        newline_indent(level);
        tl_printf("}");
        break;
    case AST_FUNCCALLEXPR:
        dump_expr(expr->funccall.func_expr, level);
        tl_printf("(");
        dump_exprlist(expr->funccall.arg_exprs, level);
        tl_printf(")");
        break;
    case AST_SELECTEXPR:
        dump_expr(expr->selectexpr.cond, level);
        tl_printf("? ");
        dump_expr(expr->selectexpr.expr1, level);
        tl_printf(": ");
        dump_expr(expr->selectexpr.expr2, level);
        break;
    case AST_BINOPEXPR:
        tl_printf("(");
        dump_expr(expr->binopexpr.left_expr, level);
        tl_printf(" %s ", tl_tokenstr(expr->binopexpr.opt));
        dump_expr(expr->binopexpr.right_expr, level);
        tl_printf(")");
        break;
    case AST_UNIOPEXPR:
        tl_printf("(");
        tl_printf("%s", tl_tokenstr(expr->uniopexpr.opt));
        dump_expr(expr->uniopexpr.expr, level);
        tl_printf(")");
        break;
    case AST_LISTCTOREXPR:
        tl_printf("[");
        dump_exprlist(expr->listctorexpr.exprs, level);
        tl_printf("]");
        break;
    case AST_MAPCTOREXPR:
        do {
            tl_printf("{");
            tl_expr_t* k = expr->mapctorexpr.key_exprs;
            tl_expr_t* v = expr->mapctorexpr.value_exprs;
            if (k) {
                dump_expr(k, level);
                tl_printf(": ");
                dump_expr(v, level);
                for (k = k->next, v = v->next; k != nullptr; k = k->next, v = v->next) {
                    tl_printf(", ");
                    dump_expr(k, level);
                    tl_printf(": ");
                    dump_expr(v, level);
                }
            }
            tl_printf("}");
        } while (0);
        break;
    case AST_SETCTOREXPR:
        tl_printf("{");
        dump_exprlist(expr->setctorexpr.exprs, level);
        tl_printf("}");
        break;
    case AST_ACCESSEXPR:
        dump_expr(expr->accessexpr.expr, level);
        if (expr->accessexpr.dot) {
            tl_printf(".");
            dump_expr(expr->accessexpr.field_expr, level);
        } else {
            tl_printf("[");
            dump_expr(expr->accessexpr.field_expr, level);
            tl_printf("]");
        }
        break;
    case AST_NAME:
        tl_printf(expr->name.identifier->cstr);
        break;
    case AST_NUMLITERAL:
        if (expr->literal.value.vtype == TL_TYPE_INT) {
            tl_printf("%d", expr->literal.value.vv.i);
        } else {
            tl_printf("%f", expr->literal.value.vv.n);
        }
        break;
    case AST_STRINGLITERAL:
        tl_printf("'%s'", ((tl_string_t*)expr->literal.value.vv.gco)->cstr);
        break;
    case AST_BOOLLITERAL:
        tl_printf(expr->literal.value.vv.b ? "true" : "false");
        break;
    case AST_NULLLITERAL:
        tl_printf("null");
        break;
    default:
        tl_panic(TL_PANIC_SYNTAX, "expr <%d> not support", expr->type);
        break;
    }
}
static void dump_exprlist(tl_expr_t* exprs, int level)
{
    tl_expr_t* e = exprs;
    if (e) {
        dump_expr(e, level);
        for (e = e->next; e != nullptr; e = e->next) {
            tl_printf(", ");
            dump_expr(e, level);
        }
    }
}
static void dump_statlist(tl_stat_t* stats, int level)
{
    for (tl_stat_t* s = stats; s != nullptr; s = s->next) {
        dump_stat(s, level, true);
    }
}
