#ifndef _TL_AST_H_
#define _TL_AST_H_

// #include "tl_env.h"
#include "tl_config.h"
#include "tl_lexer.h"
#include "tl_types.h"
#include "tl_values.h"

typedef struct tl_stat_t tl_stat_t;
typedef struct tl_expr_t tl_expr_t;

typedef struct tl_program_t {
    tl_stat_t* stats;
} tl_program_t;

typedef enum tl_ast_t {
    AST_PROGRAM,
    AST_EMPTYSTAT,   // stat
    AST_DELSTAT,
    AST_BLOCKSTAT,
    AST_VARDECLSTAT,
    AST_IFSTAT,
    AST_SWITCHSTAT,
    AST_REPEATSTAT,
    AST_WHILESTAT,
    AST_FORLOOPSTAT,
    AST_FOREACHSTAT,
    AST_BREAKSTAT,
    AST_CONTINUESTAT,
    AST_RETURNSTAT,
#if PRINT_STAT_ENABLE
    AST_PRINTSTAT,
#endif
    AST_ASSIGNSTAT,
    AST_COMPOUNDASSIGNSTAT,
    AST_FUNCCALLSTAT,
    AST_FUNCDEF,   // expr
    AST_FUNCCALLEXPR,
    AST_SELECTEXPR,
    AST_BINOPEXPR,
    AST_UNIOPEXPR,
    AST_LISTCTOREXPR,
    AST_MAPCTOREXPR,
    AST_SETCTOREXPR,
    AST_ACCESSEXPR,
    AST_NAME,
    AST_NUMLITERAL,
    AST_STRINGLITERAL,
    AST_BOOLLITERAL,
    AST_NULLLITERAL,
} tl_ast_t;


// struct tl_emptystat_t;

struct tl_delstat_t {
    // TODO
    void* notused;
};

struct tl_blockstat_t {
    tl_stat_t* stats;
};

struct tl_vardeclstat_t {
    tl_expr_t* names;
    tl_expr_t* exprs;
    bool       const_;
};

struct tl_ifstat_t {
    tl_expr_t* cond_exprs;
    tl_stat_t* stats;
};

struct tl_switchstat_t {
    tl_expr_t* expr;
    tl_expr_t* case_exprs;
    tl_stat_t* case_stats;
    tl_stat_t* default_stat;
};

struct tl_repeatstat_t {
    tl_expr_t* expr;
    tl_stat_t* stat;
};

struct tl_whilestat_t {
    tl_expr_t* expr;
    tl_stat_t* stat;
};

struct tl_forloopstat_t {
    tl_expr_t* var_name;
    tl_expr_t* start_expr;
    tl_expr_t* end_expr;
    tl_expr_t* step_expr;
    tl_stat_t* stat;
};

struct tl_foreachstat_t {
    tl_expr_t* key_name;
    tl_expr_t* val_name;
    tl_expr_t* expr;
    tl_stat_t* stat;
};

// struct tl_breakstat_t

// struct tl_continuestat_t

struct tl_returnstat_t {
    tl_expr_t* expr;
};


struct tl_printstat_t {
    tl_expr_t* exprs;
};

struct tl_assignstat_t {
    tl_expr_t* left_exprs;
    tl_expr_t* right_exprs;
};

struct tl_compoundassignstat_t {
    tl_token_t opt;
    tl_expr_t* left_expr;
    tl_expr_t* right_expr;
};

struct tl_funcdef_t {
    tl_expr_t* param_names;
    // bool        vararg;
    tl_stat_t* body;
    bool       in_gc;
};

struct tl_funccall_t {
    tl_expr_t* func_expr;
    tl_expr_t* arg_exprs;
};

struct tl_stat_t {
    tl_ast_t type;
    int16_t  line;
    union {
        // struct tl_emptystat_t          emptystat;
        // struct tl_breakstat_t          breakstat;
        // struct tl_continuestat_t       continuestat;
        struct tl_delstat_t            delstat;
        struct tl_blockstat_t          blockstat;
        struct tl_vardeclstat_t        vardeclstat;
        struct tl_ifstat_t             ifstat;
        struct tl_switchstat_t         switchstat;
        struct tl_repeatstat_t         repeatstat;
        struct tl_whilestat_t          whilestat;
        struct tl_forloopstat_t        forloopstat;
        struct tl_foreachstat_t        foreachstat;
        struct tl_returnstat_t         returnstat;
        struct tl_printstat_t          printstat;
        struct tl_assignstat_t         assignstat;
        struct tl_compoundassignstat_t compoundassignstat;
        // struct tl_funcdef_t            funcdef;
        struct tl_funccall_t funccall;
    };
    tl_stat_t* next;
    tl_stat_t* prev_all;
};


struct tl_selectexpr_t {
    tl_expr_t* cond;
    tl_expr_t* expr1;
    tl_expr_t* expr2;
};

struct tl_binopexpr_t {
    tl_token_t opt;
    tl_expr_t* left_expr;
    tl_expr_t* right_expr;
};

struct tl_uniopexpr_t {
    tl_token_t opt;
    tl_expr_t* expr;
};

struct tl_listctorexpr_t {
    tl_expr_t* exprs;
};

struct tl_mapctorexpr_t {
    tl_expr_t* key_exprs;
    tl_expr_t* value_exprs;
};

struct tl_setctorexpr_t {
    tl_expr_t* exprs;
};

struct tl_accessexpr_t {
    tl_expr_t* expr;
    tl_expr_t* field_expr;
    bool       dot;
};

struct tl_name_t {
    tl_string_t* identifier;
};

struct tl_literal_t {
    tl_value_t value;
};

struct tl_expr_t {
    tl_ast_t type;
    int16_t  line;
    union {
        struct tl_selectexpr_t   selectexpr;
        struct tl_binopexpr_t    binopexpr;
        struct tl_uniopexpr_t    uniopexpr;
        struct tl_listctorexpr_t listctorexpr;
        struct tl_mapctorexpr_t  mapctorexpr;
        struct tl_setctorexpr_t  setctorexpr;
        struct tl_accessexpr_t   accessexpr;
        struct tl_funccall_t     funccall;
        struct tl_funcdef_t      funcdef;
        struct tl_name_t         name;
        struct tl_literal_t      literal;   // int float string bool null
    };
    tl_expr_t* next;
    tl_expr_t* prev_all;
};

typedef struct tl_parser_t {
    tl_lexer_t lexer;
    tl_token_t token;
    union {
        char      tokenvalue[TL_LEXER_MAX_TOKEN_LEN];
        tl_size_t number_literal;   // aligin
    };
} tl_parser_t;

void         tl_parser_init(tl_parser_t* parser, const char* code);
tl_program_t tl_parser_parse(tl_parser_t* parser);
void         tl_parser_dump(tl_program_t* program);
void         tl_parser_free(tl_program_t* program);
void         tl_parser_free_error();
void         free_stat(tl_stat_t* stat);
void         free_expr(tl_expr_t* expr);

#endif
