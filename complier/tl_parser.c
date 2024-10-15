#include "tl_parser.h"
#include "tl_mem.h"
#include "tl_string.h"

#if 0
static tl_stat_t statcache[TL_PARSER_MAX_STAT_NUM];
static tl_expr_t exprcache[TL_PARSER_MAX_STAT_NUM];

static tl_stat_t* freestatlist;
static tl_expr_t* freeexprlist;
static int         freestatnum;
static int         freeexprnum;

static inline tl_stat_t* stat_new(void)
{
    tl_stat_t* stat = nullptr;
    if (freestatlist) {
        stat         = freestatlist;
        freestatlist = freestatlist->next;
        stat->next   = nullptr;
        freestatnum--;
        memset(stat, 0, sizeof(tl_stat_t));
    } else {
        // "no free stat ast"
    }

    return stat;
}
static inline void stat_del(tl_stat_t* stat)
{
    stat->next   = freestatlist;
    freestatlist = stat;
    freestatnum++;
}
static inline tl_expr_t* expr_new(void)
{
    tl_expr_t* expr = nullptr;
    if (freeexprlist) {
        expr         = freeexprlist;
        freeexprlist = freeexprlist->next;
        expr->next   = nullptr;
        freeexprnum--;
        memset(expr, 0, sizeof(tl_expr_t));
    } else {
        // "no free expr ast"
    }
    return expr;
}
static inline void expr_del(tl_expr_t* expr)
{
    expr->next   = freeexprlist;
    freeexprlist = expr;
    freeexprnum++;
}

static void astcacheinit(void)
{
    freestatlist = nullptr;
    for (int i = 0; i < util_arraylen(statcache); i++) {
        stat_del(&statcache[i]);
    }
    freeexprlist = nullptr;
    for (int i = 0; i < util_arraylen(exprcache); i++) {
        expr_del(&exprcache[i]);
    }
}

#endif

#define current          parser->token
#define token_postion    parser->lexer.line, parser->lexer.column
#define panic(eno, msg)  tl_panic(eno, "%s at [%d:%d]%s", msg, token_postion, parser->tokenvalue)
#define eat(token)       parser_eat(parser, token)
#define setposition(ast) ast->line = parser->lexer.line

static void       parser_eat(tl_parser_t* parser, tl_token_t token);
static tl_stat_t* parse_statlist(tl_parser_t* parser);
static tl_stat_t* parse_stat(tl_parser_t* parser);
static tl_stat_t* parse_emptystat(tl_parser_t* parser);
static tl_stat_t* parse_delstat(tl_parser_t* parser);
static tl_stat_t* parse_blockstat(tl_parser_t* parser);
static tl_stat_t* parse_vardeclstat(tl_parser_t* parser);
static tl_stat_t* parse_ifstat(tl_parser_t* parser);
static tl_stat_t* parse_switchstat(tl_parser_t* parser);
static tl_stat_t* parse_repeatstat(tl_parser_t* parser);
static tl_stat_t* parse_whilestat(tl_parser_t* parser);
static tl_stat_t* parse_forprefixstat(tl_parser_t* parser);
static tl_stat_t* parse_breakstat(tl_parser_t* parser);
static tl_stat_t* parse_continuestat(tl_parser_t* parser);
static tl_stat_t* parse_funcprefixstat(tl_parser_t* parser);
static tl_stat_t* parse_returnstat(tl_parser_t* parser);
#if PRINT_STAT_ENABLE
static tl_stat_t* parse_printstat(tl_parser_t* parser);
#endif
static tl_stat_t* parse_identifierprefixstat(tl_parser_t* parser);
static void       complete_forloopstat(tl_parser_t* parser, struct tl_forloopstat_t* stat);
static void       complete_foreachstat(tl_parser_t* parser, struct tl_foreachstat_t* stat);
static void       complete_assignstat(tl_parser_t* parser, struct tl_assignstat_t* stat);
static void       complete_compoundassignstat(tl_parser_t* parser, struct tl_compoundassignstat_t* stat);
static void       complete_funccall(tl_parser_t* parser, struct tl_funccall_t* stat);

#define parse_expr parse_selectexpr
static tl_expr_t* parse_selectexpr(tl_parser_t* parser);
static tl_expr_t* parse_logicorexpr(tl_parser_t* parser);
static tl_expr_t* parse_logicandexpr(tl_parser_t* parser);
static tl_expr_t* parse_checkexpr(tl_parser_t* parser);
static tl_expr_t* parse_relationexpr(tl_parser_t* parser);
static tl_expr_t* parse_bitorexpr(tl_parser_t* parser);
static tl_expr_t* parse_bitxorexpr(tl_parser_t* parser);
static tl_expr_t* parse_bitandexpr(tl_parser_t* parser);
static tl_expr_t* parse_bitshiftexpr(tl_parser_t* parser);
static tl_expr_t* parse_additionexpr(tl_parser_t* parser);
static tl_expr_t* parse_multipleexpr(tl_parser_t* parser);
static tl_expr_t* parse_unaryexpr(tl_parser_t* parser);
static tl_expr_t* parse_powexpr(tl_parser_t* parser);
static tl_expr_t* parse_primaryexpr(tl_parser_t* parser);
static tl_expr_t* parse_listctorexpr(tl_parser_t* parser);
static tl_expr_t* parse_mapsetctorexpr(tl_parser_t* parser);
static tl_expr_t* parse_lvalueexpr(tl_parser_t* parser);
static tl_expr_t* parse_namelist(tl_parser_t* parser);
static tl_expr_t* parse_exprlist(tl_parser_t* parser);
static tl_expr_t* parse_funcdef(tl_parser_t* parser);
static tl_expr_t* parse_name(tl_parser_t* parser);
static tl_expr_t* parse_literal(tl_parser_t* parser);
static tl_expr_t* make_binopexpr(tl_parser_t* parser, tl_expr_t* left, tl_expr_t* (*parse_func)(tl_parser_t*));
static void       free_statlist(tl_stat_t* stats);
static void       free_exprlist(tl_expr_t* exprs);


tl_stat_t* stat_all = nullptr;
tl_expr_t* expr_all = nullptr;

static inline tl_stat_t* stat_new(void)
{
    tl_stat_t* stat = (tl_stat_t*)tl_new(sizeof(tl_stat_t));
    memset(stat, 0, sizeof(tl_stat_t));

    stat->next     = nullptr;
    stat->prev_all = stat_all;
    stat_all       = stat;
    return stat;
}
static inline void stat_del(tl_stat_t* stat)
{
    tl_free(stat, sizeof(tl_stat_t));
}
static inline tl_expr_t* expr_new(void)
{
    tl_expr_t* expr = (tl_expr_t*)tl_new(sizeof(tl_expr_t));
    memset(expr, 0, sizeof(tl_expr_t));

    expr->next     = nullptr;
    expr->prev_all = expr_all;
    expr_all       = expr;
    return expr;
}
static inline void expr_del(tl_expr_t* expr)
{
    tl_free(expr, sizeof(tl_expr_t));
}


void tl_parser_init(tl_parser_t* parser, const char* code)
{
    if (parser == nullptr)
        return;

    tl_lexer_init(&parser->lexer, code, parser->tokenvalue);
    parser->token = tl_lexer_nexttoken(&parser->lexer);
}


tl_program_t tl_parser_parse(tl_parser_t* parser)
{
    tl_program_t program;
    program.stats = parse_statlist(parser);
    if (parser->token != TOKEN_EOF) {
        free_statlist(program.stats);
        program.stats = nullptr;
        panic(TL_PANIC_SYNTAX, "EOF expected");
    }
    return program;
}


void tl_parser_free(tl_program_t* program)
{
    free_statlist(program->stats);
    program->stats = nullptr;

    stat_all = nullptr;
    expr_all = nullptr;
}


void tl_parser_free_error()
{
    for (tl_stat_t* s = stat_all; s != nullptr;) {
        tl_stat_t* prev = s->prev_all;
        stat_del(s);
        s = prev;
    }
    for (tl_expr_t* e = expr_all; e != nullptr;) {
        tl_expr_t* prev = e->prev_all;

        if (e->type == AST_NAME) {
            tl_string_freeex(e->name.identifier);
        } else if (e->type == AST_STRINGLITERAL) {
            tl_string_freeex((tl_string_t*)e->literal.value.vv.gco);
        }
        expr_del(e);
        e = prev;
    }
    stat_all = nullptr;
    expr_all = nullptr;
}


static void parser_eat(tl_parser_t* parser, tl_token_t token)
{
    if (parser->token == token) {
        parser->token = tl_lexer_nexttoken(&parser->lexer);
    } else {
        panic(TL_PANIC_SYNTAX, "unexpected token");
    }
}


/**
 * @brief parse stat_list

    stat_list : (stat)+
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_statlist(tl_parser_t* parser)
{
    tl_stat_t* stats = nullptr;
    tl_stat_t* tail  = nullptr;
    while (current != TOKEN_RBRACE && current != TOKEN_EOF) {
        if (stats == nullptr) {
            stats = parse_stat(parser);
            tail  = stats;
        } else {
            tl_stat_t* s = parse_stat(parser);

            if (s) {
                s->next    = nullptr;
                tail->next = s;
                tail       = s;
            }
        }
    }
    return stats;
}


/**
 * @brief parse stat

    stat    : stat_re (SEMI)*
    stat_re : empty_stat
            | del_stat
            | block_stat
            | var_decl_stat
            | if_stat
            | switch_stat
            | repeat_stat
            | while_stat
            | forloop_stat
            | foreach_stat
            | break_stat
            | continue_stat
            | func_decl_stat
            | return_stat
            | assign_stat
            | compoundassign_stat
            | func_call_stat
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_stat(tl_parser_t* parser)
{
    tl_stat_t* stat;

    switch (current) {
    case TOKEN_SEMI:
        stat = parse_emptystat(parser);
        break;
    case TOKEN_DEL:
        stat = parse_delstat(parser);
        break;
    case TOKEN_LBRACE:   // { block_stat
        stat = parse_blockstat(parser);
        break;
    case TOKEN_VAR:
    case TOKEN_CONST:
        stat = parse_vardeclstat(parser);
        break;
    case TOKEN_IF:
        stat = parse_ifstat(parser);
        break;
    case TOKEN_SWITCH:
        stat = parse_switchstat(parser);
        break;
    case TOKEN_REPEAT:
        stat = parse_repeatstat(parser);
        break;
    case TOKEN_WHILE:
        stat = parse_whilestat(parser);
        break;
    case TOKEN_FOR:   // forloop_stat | foreach_stat
        stat = parse_forprefixstat(parser);
        break;
    case TOKEN_BREAK:
        stat = parse_breakstat(parser);
        break;
    case TOKEN_CONTINUE:
        stat = parse_continuestat(parser);
        break;
    case TOKEN_FUNC:
        stat = parse_funcprefixstat(parser);
        break;
    case TOKEN_RETURN:
        stat = parse_returnstat(parser);
        break;
#if PRINT_STAT_ENABLE
    case TOKEN_PRINT:
        stat = parse_printstat(parser);
        break;
#endif
    default:   // IDENTIFIER
        stat = parse_identifierprefixstat(parser);
        break;
    }

    while (current == TOKEN_SEMI) {
        eat(TOKEN_SEMI);
    }

    return stat;
}


/**
 * @brief parse empty_stat

    empty_stat : SEMI
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_emptystat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_EMPTYSTAT;
    eat(TOKEN_SEMI);
    return stat;
}


/**
 * @brief parse del_stat

    del_stat : DEL ...
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_delstat(tl_parser_t* parser)
{
    // TODO
    return nullptr;
}


/**
 * @brief block_stat

    block_stat : LBRACE stat_list RBRACE
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_blockstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_BLOCKSTAT;
    setposition(stat);

    eat(TOKEN_LBRACE);
    stat->blockstat.stats = parse_statlist(parser);
    eat(TOKEN_RBRACE);
    return stat;
}


/**
 * @brief parse var_decl_stat

    var_decl_stat : (VAR | CONST) name_list (ASSIGN expr_list)?
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_vardeclstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_VARDECLSTAT;
    setposition(stat);

    stat->vardeclstat.const_ = (current == TOKEN_CONST) ? true : false;
    eat(parser->token);   // eat CONST or VAR
    stat->vardeclstat.names = parse_namelist(parser);
    if (current == TOKEN_ASSIGN) {
        eat(TOKEN_ASSIGN);
        stat->vardeclstat.exprs = parse_exprlist(parser);
    } else {
        stat->vardeclstat.exprs = nullptr;
    }
    return stat;
}


/**
 * @brief parse if_stat

    if_stat : IF expr (COLON)? stat (ELIF expr (COLON)? stat)* (ELSE (COLON)? stat)?
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_ifstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_IFSTAT;
    setposition(stat);

    eat(TOKEN_IF);
    stat->ifstat.cond_exprs = parse_expr(parser);
    tl_expr_t* condtail     = stat->ifstat.cond_exprs;
    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);
    stat->ifstat.stats  = parse_stat(parser);
    tl_stat_t* stattail = stat->ifstat.stats;

    while (current == TOKEN_ELIF) {
        eat(TOKEN_ELIF);
        condtail->next = parse_expr(parser);
        condtail       = condtail->next;
        if (current == TOKEN_COLON)
            eat(TOKEN_COLON);
        stattail->next = parse_stat(parser);
        stattail       = stattail->next;
    }

    if (current == TOKEN_ELSE) {
        condtail->next = expr_new();
        condtail       = condtail->next;

        condtail->type                = AST_BOOLLITERAL;
        condtail->literal.value.vtype = TL_TYPE_BOOL;
        condtail->literal.value.vv.b  = true;
        setposition(condtail);

        eat(TOKEN_ELSE);
        if (current == TOKEN_COLON)
            eat(TOKEN_COLON);

        stattail->next = parse_stat(parser);
        stattail       = stattail->next;
    }

    condtail->next = nullptr;
    stattail->next = nullptr;

    return stat;
}


/**
 * @brief parse switch_stat

    switch_stat : SWITCH expr (COLON)? (CASE expr (COLON)? stat)+ (DEFAULT (COLON)? stat)?
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_switchstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_SWITCHSTAT;
    setposition(stat);

    eat(TOKEN_SWITCH);
    stat->switchstat.expr = parse_expr(parser);
    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);

    tl_expr_t** casetail = &stat->switchstat.case_exprs;
    tl_stat_t** stattail = &stat->switchstat.case_stats;

    do {
        eat(TOKEN_CASE);

        if (*casetail == nullptr) {
            *casetail = parse_expr(parser);
        } else {
            (*casetail)->next = parse_expr(parser);
            casetail          = &(*casetail)->next;
        }

        if (current == TOKEN_COLON) {
            eat(TOKEN_COLON);
        }

        if (*stattail == nullptr) {
            *stattail = parse_stat(parser);
        } else {
            (*stattail)->next = parse_stat(parser);
            stattail          = &(*stattail)->next;
        }
    } while (current == TOKEN_CASE);

    if (current == TOKEN_DEFAULT) {
        eat(TOKEN_DEFAULT);
        if (current == TOKEN_COLON) {
            eat(TOKEN_COLON);
        }
        stat->switchstat.default_stat = parse_stat(parser);
    } else {
        stat->switchstat.default_stat = nullptr;
    }

    return stat;
}


/**
 * @brief parse repeat_stat

    repeat_stat : REPEAT (COLON)? stat UNTIL expr
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_repeatstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_REPEATSTAT;
    setposition(stat);

    eat(TOKEN_REPEAT);
    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);

    stat->repeatstat.stat = parse_stat(parser);
    eat(TOKEN_UNTIL);
    stat->repeatstat.expr = parse_expr(parser);

    return stat;
}


/**
 * @brief parse while_stat

    while_stat : WHILE expr (COLON)? stat
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_whilestat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_WHILESTAT;
    setposition(stat);

    eat(TOKEN_WHILE);
    stat->whilestat.expr = parse_expr(parser);
    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);
    stat->whilestat.stat = parse_stat(parser);

    return stat;
}


/**
 * @brief parse forloop_stat | foreach_stat

    forloop_stat : FOR name IS expr COMMA expr (COMMA expr)? (COLON)? stat
    foreach_stat : FOR name (COMMA name)? IN expr (COLON)? stat
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_forprefixstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    setposition(stat);
    eat(TOKEN_FOR);
    tl_expr_t* name = parse_name(parser);

    if (current == TOKEN_IS) {   // for ... is
        stat->type                 = AST_FORLOOPSTAT;
        stat->forloopstat.var_name = name;
        complete_forloopstat(parser, &stat->forloopstat);
    } else {   // for ... in
        stat->type                 = AST_FOREACHSTAT;
        stat->foreachstat.key_name = name;
        complete_foreachstat(parser, &stat->foreachstat);
    }
    return stat;
}
static void complete_forloopstat(tl_parser_t* parser, struct tl_forloopstat_t* stat)
{
    eat(TOKEN_IS);
    stat->start_expr = parse_expr(parser);
    eat(TOKEN_COMMA);
    stat->end_expr = parse_expr(parser);
    if (current == TOKEN_COMMA) {
        eat(TOKEN_COMMA);
        stat->step_expr = parse_expr(parser);
    } else {
        stat->step_expr = nullptr;
    }

    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);
    stat->stat = parse_stat(parser);
}
static void complete_foreachstat(tl_parser_t* parser, struct tl_foreachstat_t* stat)
{
    if (current == TOKEN_COMMA) {
        eat(TOKEN_COMMA);
        stat->val_name = parse_name(parser);
    }
    eat(TOKEN_IN);
    stat->expr = parse_expr(parser);
    if (current == TOKEN_COLON)
        eat(TOKEN_COLON);
    stat->stat = parse_stat(parser);
}


/**
 * @brief parse break_stat

    break_stat : BREAK
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_breakstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_BREAKSTAT;
    setposition(stat);

    eat(TOKEN_BREAK);
    return stat;
}


/**
 * @brief parse continue_stat

    continue_stat : CONTINUE
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_continuestat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_CONTINUESTAT;
    setposition(stat);

    eat(TOKEN_CONTINUE);
    return stat;
}


/**
 * @brief parse func_prefix_stat:

    func_decl_stat: FUNC name func_def  => CONST name = FUNC func_def
    func_call_stat: FUNC func_def LPAREN (expr_list)? RPAREN
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_funcprefixstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    setposition(stat);

    eat(TOKEN_FUNC);
    if (current == TOKEN_IDENTIFIER) {   // func decl
        stat->type               = AST_VARDECLSTAT;
        stat->vardeclstat.const_ = true;
        stat->vardeclstat.names  = parse_name(parser);
        stat->vardeclstat.exprs  = parse_funcdef(parser);
    } else {
        stat->type               = AST_FUNCCALLSTAT;
        stat->funccall.func_expr = parse_funcdef(parser);
        complete_funccall(parser, &stat->funccall);
    }

    return stat;
}
static void complete_funccall(tl_parser_t* parser, struct tl_funccall_t* fcall)
{
    eat(TOKEN_LPAREN);
    if (current != TOKEN_RPAREN) {
        fcall->arg_exprs = parse_exprlist(parser);
    }
    eat(TOKEN_RPAREN);
}


/**
 * @brief parse return_stat

    return_stat : RETURN (expr | SEMI)
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_returnstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_RETURNSTAT;
    setposition(stat);

    eat(TOKEN_RETURN);
    if (current != TOKEN_SEMI) {
        stat->returnstat.expr = parse_expr(parser);
    } else {
        eat(TOKEN_SEMI);
    }

    return stat;
}

#if PRINT_STAT_ENABLE
static tl_stat_t* parse_printstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    stat->type      = AST_PRINTSTAT;
    setposition(stat);

    eat(TOKEN_PRINT);
    if (current != TOKEN_SEMI) {
        stat->printstat.exprs = parse_exprlist(parser);
    } else {
        eat(TOKEN_SEMI);
    }


    return stat;
}
#endif

/**
 * @brief parse identifier_prefix_stat

    assign_stat         : lvalue_expr (COMMA lvalue_expr)* ASSIGN expr_list
    compoundassign_stat : lvalue_expr compoundassign expr
    func_call_stat      : lvalue_expr LPAREN (expr_list)? RPAREN
 *
 * @param parser
 * @return tl_stat_t*
 */
static tl_stat_t* parse_identifierprefixstat(tl_parser_t* parser)
{
    tl_stat_t* stat = stat_new();
    tl_expr_t* lve  = parse_lvalueexpr(parser);

    setposition(stat);
    if (current == TOKEN_ASSIGN || current == TOKEN_COMMA) {   // assign
        stat->type                   = AST_ASSIGNSTAT;
        stat->assignstat.left_exprs  = lve;
        stat->assignstat.right_exprs = nullptr;
        complete_assignstat(parser, &stat->assignstat);
    } else if (current == TOKEN_LPAREN) {   // funccall
        stat->type               = AST_FUNCCALLSTAT;
        stat->funccall.func_expr = lve;
        stat->funccall.arg_exprs = nullptr;
        complete_funccall(parser, &stat->funccall);
    } else {
        stat->type                          = AST_COMPOUNDASSIGNSTAT;
        stat->compoundassignstat.left_expr  = lve;
        stat->compoundassignstat.right_expr = nullptr;
        complete_compoundassignstat(parser, &stat->compoundassignstat);
    }

    return stat;
}
static void complete_assignstat(tl_parser_t* parser, struct tl_assignstat_t* stat)
{
    tl_expr_t* lefttail = stat->left_exprs;
    while (current == TOKEN_COMMA) {
        eat(TOKEN_COMMA);
        lefttail->next = parse_lvalueexpr(parser);
        lefttail       = lefttail->next;
    }
    lefttail->next = nullptr;

    eat(TOKEN_ASSIGN);
    stat->right_exprs = parse_exprlist(parser);
}
static void complete_compoundassignstat(tl_parser_t* parser, struct tl_compoundassignstat_t* stat)
{
    if (current >= TOKEN_COMPOUNDASSIGN_START && current <= TOKEN_COMPOUNDASSIGN_END) {
        stat->opt = current;
        eat(current);

        stat->right_expr = parse_expr(parser);
    } else {
        panic(TL_PANIC_SYNTAX, "compoundassign expected");
    }
}


/**
 * @brief parse select_expr

    select_expr : logic_or_expr (QUERY expr COLON expr)?
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_selectexpr(tl_parser_t* parser)
{
    tl_expr_t* lorexpr = parse_logicorexpr(parser);
    if (current != TOKEN_QUERY) {
        return lorexpr;
    }

    tl_expr_t* selectexpr = expr_new();
    selectexpr->type      = AST_SELECTEXPR;
    setposition(selectexpr);
    selectexpr->selectexpr.cond = lorexpr;
    eat(TOKEN_QUERY);
    selectexpr->selectexpr.expr1 = parse_expr(parser);
    eat(TOKEN_COLON);
    selectexpr->selectexpr.expr2 = parse_expr(parser);
    return selectexpr;
}


/**
 * @brief parse logic_or_expr

    logic_or_expr : logic_and_expr (OR logic_and_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_logicorexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_logicandexpr(parser);
    while (current == TOKEN_OR) {
        expr = make_binopexpr(parser, expr, parse_logicandexpr);
    }

    return expr;
}


/**
 * @brief parse logic_and_expr

    logic_and_expr : check_expr (AND check_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_logicandexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_checkexpr(parser);
    while (current == TOKEN_AND) {
        expr = make_binopexpr(parser, expr, parse_checkexpr);
    }

    return expr;
}


/**
 * @brief parse check_expr

    check_expr : relation_expr ((IN | IS) relation_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_checkexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_relationexpr(parser);
    while (current == TOKEN_IN || current == TOKEN_IS) {
        expr = make_binopexpr(parser, expr, parse_relationexpr);
    }
    return expr;
}


/**
 * @brief parse relation_expr

    relation_expr : bit_or_expr ((EQ | NE | LT | LE | GT | GE) bit_or_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_relationexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_bitorexpr(parser);
    while (current >= TOKEN_COMPARE_START && current <= TOKEN_COMPARE_END) {
        expr = make_binopexpr(parser, expr, parse_bitorexpr);
    }
    return expr;
}


/**
 * @brief parse bit_or_expr

    bit_or_expr : bit_xor_expr (BOR bit_xor_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_bitorexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_bitxorexpr(parser);
    while (current == TOKEN_BOR) {
        expr = make_binopexpr(parser, expr, parse_bitxorexpr);
    }
    return expr;
}


/**
 * @brief parse bit_xor_expr

    bit_xor_expr : bit_and_expr (BXOR bit_and_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_bitxorexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_bitandexpr(parser);
    while (current == TOKEN_BXOR) {
        expr = make_binopexpr(parser, expr, parse_bitandexpr);
    }
    return expr;
}


/**
 * @brief parse bit_and_expr

    bit_and_expr : bit_shift_expr (BAND bit_shift_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_bitandexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_bitshiftexpr(parser);
    while (current == TOKEN_BAND) {
        expr = make_binopexpr(parser, expr, parse_bitshiftexpr);
    }
    return expr;
}


/**
 * @brief parse bit_shift_expr

    bit_shift_expr : addition_expr ((BSHL | BSHR) addition_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_bitshiftexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_additionexpr(parser);
    while (current == TOKEN_BSHL || current == TOKEN_BSHR) {
        expr = make_binopexpr(parser, expr, parse_additionexpr);
    }
    return expr;
}


/**
 * @brief parse addition_expr

    addition_expr : multiple_expr ((ADD | SUB) multiple_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_additionexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_multipleexpr(parser);
    while (current == TOKEN_ADD || current == TOKEN_SUB) {
        expr = make_binopexpr(parser, expr, parse_multipleexpr);
    }
    return expr;
}


/**
 * @brief parse multiple_expr

    multiple_expr : unary_expr ((MUL | DIV | MOD) unary_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_multipleexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_unaryexpr(parser);
    while (current == TOKEN_MUL || current == TOKEN_DIV || current == TOKEN_MOD) {
        expr = make_binopexpr(parser, expr, parse_unaryexpr);
    }
    return expr;
}


/**
 * @brief parse unary_expr

    unary_expr : (unop)* pow_expr
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_unaryexpr(tl_parser_t* parser)
{
    if (current == TOKEN_ADD || current == TOKEN_SUB || current == TOKEN_NOT || current == TOKEN_LEN ||
        current == TOKEN_BNOT) {
        tl_expr_t* expr = expr_new();
        expr->type      = AST_UNIOPEXPR;
        setposition(expr);

        expr->uniopexpr.opt = current;
        eat(current);
        expr->uniopexpr.expr = parse_unaryexpr(parser);
        return expr;
    }
    return parse_powexpr(parser);
}


/**
 * @brief parse pow_expr

    pow_expr : primary_expr (POW primary_expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_powexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_primaryexpr(parser);
    while (current == TOKEN_POW) {
        expr = make_binopexpr(parser, expr, parse_primaryexpr);
    }
    return expr;
}


/**
 * @brief parse primary_expr

    primary_expr  : INT_LITERAL | FLOAT_LITERAL | STRING_LITERAL | TRUE | FALSE | NULL | LPAREN expr RPAREN
                  | list_ctor_expr | map_ctor_expr | set_ctor_expr | func_def_expr |
                  | lvalue_expr | func_call_expr
    func_def_expr : FUNC func_def
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_primaryexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = nullptr;
    switch (current) {
    case TOKEN_INT_LITERAL:
    case TOKEN_FLOAT_LITERAL:
    case TOKEN_STRING_LITERAL:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_NULL:
        expr = parse_literal(parser);
        break;
    case TOKEN_LPAREN:   // ( ... )
        eat(TOKEN_LPAREN);
        expr = parse_expr(parser);
        eat(TOKEN_RPAREN);
        break;
    case TOKEN_LBRACK:   // [ ... ]
        expr = parse_listctorexpr(parser);
        break;
    case TOKEN_LBRACE:   // { ... }
        expr = parse_mapsetctorexpr(parser);
        break;
    case TOKEN_FUNC:   // func ...
        eat(TOKEN_FUNC);
        expr = parse_funcdef(parser);
        if (current == TOKEN_LPAREN) {   // TODO: chain call
            tl_expr_t* call = expr_new();
            call->type      = AST_FUNCCALLEXPR;
            setposition(call);
            call->funccall.func_expr = expr;
            complete_funccall(parser, &call->funccall);
            expr = call;
        }
        break;
    default:   // identifier
        expr = parse_lvalueexpr(parser);
        if (current == TOKEN_LPAREN) {
            tl_expr_t* call = expr_new();
            call->type      = AST_FUNCCALLEXPR;
            setposition(call);
            call->funccall.func_expr = expr;
            complete_funccall(parser, &call->funccall);
            expr = call;
        }
        break;
    }
    return expr;
}


/**
 * @brief parse list_ctor_expr

    list_ctor_expr : LBRACK (expr_list)? RBRACK
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_listctorexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = expr_new();
    expr->type      = AST_LISTCTOREXPR;
    setposition(expr);
    eat(TOKEN_LBRACK);
    if (current != TOKEN_RBRACK) {
        expr->listctorexpr.exprs = parse_exprlist(parser);
    }
    eat(TOKEN_RBRACK);
    return expr;
}


/**
 * @brief parse map_ctor_expr | set_ctor_expr

    map_ctor_expr : LBRACE (expr COLON expr (COMMA expr COLON expr)*)? RBRACE
    set_ctor_expr : LBRACE expr (COMMA expr)* RBRACE
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_mapsetctorexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = expr_new();
    setposition(expr);

    tl_expr_t* keys     = nullptr;
    tl_expr_t* vals     = nullptr;
    tl_expr_t* keystail = nullptr;
    tl_expr_t* valstail = nullptr;
    bool       is_set   = false;

    eat(TOKEN_LBRACE);
    if (current != TOKEN_RBRACE) {
        keys     = parse_expr(parser);
        keystail = keys;
        if (current == TOKEN_COLON) {
            eat(TOKEN_COLON);
            vals     = parse_expr(parser);
            valstail = vals;
        } else {
            is_set = true;
        }

        while (current == TOKEN_COMMA) {
            eat(TOKEN_COMMA);
            keystail->next = parse_expr(parser);
            keystail       = keystail->next;

            if (is_set == false) {
                eat(TOKEN_COLON);
                valstail->next = parse_expr(parser);
                valstail       = valstail->next;
            }
        }
    }
    eat(TOKEN_RBRACE);

    if (is_set) {
        expr->type              = AST_SETCTOREXPR;
        expr->setctorexpr.exprs = keys;
    } else {
        expr->type                    = AST_MAPCTOREXPR;
        expr->mapctorexpr.key_exprs   = keys;
        expr->mapctorexpr.value_exprs = vals;
    }
    return expr;
}


/**
 * @brief parse lvalue_expr

    lvalue_expr : name | lvalue_expr LBRACK expr RBRACK | lvalue_expr DOT name
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_lvalueexpr(tl_parser_t* parser)
{
    tl_expr_t* expr = parse_name(parser);
    while (current == TOKEN_LBRACK || current == TOKEN_DOT) {
        tl_expr_t* access = expr_new();
        access->type      = AST_ACCESSEXPR;
        setposition(access);
        access->accessexpr.expr = expr;

        if (current == TOKEN_LBRACK) {
            access->accessexpr.dot = false;
            eat(TOKEN_LBRACK);
            access->accessexpr.field_expr = parse_expr(parser);
            eat(TOKEN_RBRACK);
        } else {
            access->accessexpr.dot = true;
            eat(TOKEN_DOT);
            access->accessexpr.field_expr = parse_name(parser);
        }
        expr = access;
    }
    return expr;
}


/**
 * @brief parse name_list

    name_list : name (COMMA name)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_namelist(tl_parser_t* parser)
{
    tl_expr_t* names     = parse_name(parser);
    tl_expr_t* namestail = names;
    while (current == TOKEN_COMMA) {
        eat(TOKEN_COMMA);
        namestail->next = parse_name(parser);
        namestail       = namestail->next;
    }
    return names;
}


/**
 * @brief parse expr_list

    expr_list : expr (COMMA expr)*
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_exprlist(tl_parser_t* parser)
{
    tl_expr_t* exprs     = parse_expr(parser);
    tl_expr_t* exprstail = exprs;
    while (current == TOKEN_COMMA) {
        eat(TOKEN_COMMA);
        exprstail->next = parse_expr(parser);
        exprstail       = exprstail->next;
    }
    exprstail->next = nullptr;

    return exprs;
}


/**
 * @brief parse func_def

    func_def   : LPAREN (param_list)? RPAREN LBRACE stat_list RBRACE
    param_list : name_list
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_funcdef(tl_parser_t* parser)
{
    tl_expr_t* expr = expr_new();
    expr->type      = AST_FUNCDEF;
    setposition(expr);

    eat(TOKEN_LPAREN);
    if (current != TOKEN_RPAREN) {
        expr->funcdef.param_names = parse_namelist(parser);
    }
    eat(TOKEN_RPAREN);

    eat(TOKEN_LBRACE);
    expr->funcdef.body = parse_statlist(parser);
    eat(TOKEN_RBRACE);


    return expr;
}


/**
 * @brief parse name

    name : IDENTIFIER
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_name(tl_parser_t* parser)
{
    tl_expr_t* expr = expr_new();
    expr->type      = AST_NAME;
    setposition(expr);

    expr->name.identifier = tl_string_newex(parser->tokenvalue);
    eat(TOKEN_IDENTIFIER);
    return expr;
}


/**
 * @brief
 *
 * @param parser
 * @return tl_expr_t*
 */
static tl_expr_t* parse_literal(tl_parser_t* parser)
{
    tl_expr_t* expr = expr_new();
    setposition(expr);

    switch (current) {
    case TOKEN_INT_LITERAL:
        expr->type                = AST_NUMLITERAL;
        expr->literal.value.vtype = TL_TYPE_INT;
        expr->literal.value.vv.i  = *(tl_int_t*)parser->tokenvalue;
        break;
    case TOKEN_FLOAT_LITERAL:
        expr->type                = AST_NUMLITERAL;
        expr->literal.value.vtype = TL_TYPE_FLOAT;
        expr->literal.value.vv.n  = *(tl_float_t*)parser->tokenvalue;
        break;
    case TOKEN_STRING_LITERAL:
        expr->type                 = AST_STRINGLITERAL;
        expr->literal.value.vtype  = TL_TYPE_STRING;
        expr->literal.value.vv.gco = (tl_gcobj_t*)tl_string_newex(parser->tokenvalue);
        break;
    case TOKEN_TRUE:
        expr->type                = AST_BOOLLITERAL;
        expr->literal.value.vtype = TL_TYPE_BOOL;
        expr->literal.value.vv.b  = true;
        break;
    case TOKEN_FALSE:
        expr->type                = AST_BOOLLITERAL;
        expr->literal.value.vtype = TL_TYPE_BOOL;
        expr->literal.value.vv.b  = false;
        break;
    case TOKEN_NULL:
        expr->type                = AST_NULLLITERAL;
        expr->literal.value.vtype = TL_TYPE_NULL;
        break;
    default:
        // unreachable
        break;
    }
    eat(current);

    return expr;
}


/**
 * @brief
 *
 * @param parser
 * @param left
 * @param right_parser
 * @return tl_expr_t*
 */
static tl_expr_t* make_binopexpr(tl_parser_t* parser, tl_expr_t* left, tl_expr_t* (*right_parser)(tl_parser_t*))
{
    tl_expr_t* expr = expr_new();
    expr->type      = AST_BINOPEXPR;
    setposition(expr);

    expr->binopexpr.opt = current;
    eat(current);
    expr->binopexpr.left_expr  = left;
    expr->binopexpr.right_expr = right_parser(parser);

    return expr;
}


void free_stat(tl_stat_t* stat)
{
    if (stat == nullptr) {
        return;
    }

    switch (stat->type) {
    case AST_EMPTYSTAT:
        stat_del(stat);
        break;
    case AST_DELSTAT:
        stat_del(stat);
        break;
    case AST_BLOCKSTAT:
        free_statlist(stat->blockstat.stats);
        stat_del(stat);
        break;
    case AST_VARDECLSTAT:
        free_exprlist(stat->vardeclstat.exprs);
        free_exprlist(stat->vardeclstat.names);
        stat_del(stat);
        break;
    case AST_IFSTAT:
        free_exprlist(stat->ifstat.cond_exprs);
        free_statlist(stat->ifstat.stats);
        stat_del(stat);
        break;
    case AST_SWITCHSTAT:
        free_expr(stat->switchstat.expr);
        free_exprlist(stat->switchstat.case_exprs);
        free_statlist(stat->switchstat.case_stats);
        free_stat(stat->switchstat.default_stat);
        stat_del(stat);
        break;
    case AST_REPEATSTAT:
        free_expr(stat->repeatstat.expr);
        free_stat(stat->repeatstat.stat);
        stat_del(stat);
        break;
    case AST_WHILESTAT:
        free_expr(stat->whilestat.expr);
        free_stat(stat->whilestat.stat);
        stat_del(stat);
        break;
    case AST_FORLOOPSTAT:
        free_expr(stat->forloopstat.var_name);
        free_expr(stat->forloopstat.start_expr);
        free_expr(stat->forloopstat.end_expr);
        free_expr(stat->forloopstat.step_expr);
        free_stat(stat->forloopstat.stat);
        stat_del(stat);
        break;
    case AST_FOREACHSTAT:
        free_expr(stat->foreachstat.key_name);
        free_expr(stat->foreachstat.val_name);
        free_expr(stat->foreachstat.expr);
        free_stat(stat->foreachstat.stat);
        stat_del(stat);
        break;
    case AST_BREAKSTAT:
        stat_del(stat);
        break;
    case AST_CONTINUESTAT:
        stat_del(stat);
        break;
    case AST_RETURNSTAT:
        free_expr(stat->returnstat.expr);
        stat_del(stat);
        break;
#if PRINT_STAT_ENABLE
    case AST_PRINTSTAT:
        free_exprlist(stat->printstat.exprs);
        stat_del(stat);
        break;
#endif
    case AST_ASSIGNSTAT:
        free_exprlist(stat->assignstat.left_exprs);
        free_exprlist(stat->assignstat.right_exprs);
        stat_del(stat);
        break;
    case AST_COMPOUNDASSIGNSTAT:
        free_expr(stat->compoundassignstat.left_expr);
        free_expr(stat->compoundassignstat.right_expr);
        stat_del(stat);
        break;
    case AST_FUNCCALLSTAT:
        free_expr(stat->funccall.func_expr);
        free_exprlist(stat->funccall.arg_exprs);
        stat_del(stat);
        break;
    default:
        tl_panic(TL_PANIC_SYNTAX, "stat <%d> not support", stat->type);
    }
}
void free_expr(tl_expr_t* expr)
{
    if (expr == nullptr) {
        return;
    }

    switch (expr->type) {
    case AST_FUNCDEF:
        if (!expr->funcdef.in_gc) {
            free_exprlist(expr->funcdef.param_names);
            free_statlist(expr->funcdef.body);
            expr_del(expr);
        }
        break;
    case AST_FUNCCALLEXPR:
        free_expr(expr->funccall.func_expr);
        free_exprlist(expr->funccall.arg_exprs);
        expr_del(expr);
        break;
    case AST_SELECTEXPR:
        free_expr(expr->selectexpr.cond);
        free_expr(expr->selectexpr.expr1);
        free_expr(expr->selectexpr.expr2);
        expr_del(expr);
        break;
    case AST_BINOPEXPR:
        free_expr(expr->binopexpr.left_expr);
        free_expr(expr->binopexpr.right_expr);
        expr_del(expr);
        break;
    case AST_UNIOPEXPR:
        free_expr(expr->uniopexpr.expr);
        expr_del(expr);
        break;
    case AST_LISTCTOREXPR:
        free_exprlist(expr->listctorexpr.exprs);
        expr_del(expr);
        break;
    case AST_MAPCTOREXPR:
        free_exprlist(expr->mapctorexpr.key_exprs);
        free_exprlist(expr->mapctorexpr.value_exprs);
        expr_del(expr);
        break;
    case AST_SETCTOREXPR:
        free_exprlist(expr->setctorexpr.exprs);
        expr_del(expr);
        break;
    case AST_ACCESSEXPR:
        free_exprlist(expr->accessexpr.expr);
        free_exprlist(expr->accessexpr.field_expr);
        expr_del(expr);
        break;
    case AST_NAME:
        tl_string_freeex(expr->name.identifier);
        expr_del(expr);
        break;
    case AST_STRINGLITERAL:
        tl_string_freeex((tl_string_t*)expr->literal.value.vv.gco);
        expr_del(expr);
        break;
    case AST_NUMLITERAL:
    case AST_BOOLLITERAL:
    case AST_NULLLITERAL:
        expr_del(expr);
        break;
    default:
        tl_panic(TL_PANIC_SYNTAX, "expr <%d> not support", expr->type);
    }
}
static void free_exprlist(tl_expr_t* exprs)
{
    for (tl_expr_t* e = exprs; e != nullptr;) {
        tl_expr_t* next = e->next;
        free_expr(e);
        e = next;
    }
}
static void free_statlist(tl_stat_t* stats)
{
    for (tl_stat_t* s = stats; s != nullptr;) {
        tl_stat_t* next = s->next;
        free_stat(s);
        s = next;
    }
}
