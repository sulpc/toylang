#ifndef _TL_TOKEN_H_
#define _TL_TOKEN_H_

#define PRINT_STAT_ENABLE 0
// clang-format off
typedef enum {
    TOKEN_IF,                                            // if
    TOKEN_ELIF,                                          // elif
    TOKEN_ELSE,                                          // else
    TOKEN_SWITCH,                                        // switch
    TOKEN_CASE,                                          // case
    TOKEN_DEFAULT,                                       // default
    TOKEN_LABEL,                                         // label
    TOKEN_GOTO,                                          // goto
    TOKEN_BREAK,                                         // break
    TOKEN_CONTINUE,                                      // continue
    TOKEN_REPEAT,                                        // repeat
    TOKEN_UNTIL,                                         // until
    TOKEN_WHILE,                                         // while
    TOKEN_FOR,                                           // for
    TOKEN_FUNC,                                          // func
    TOKEN_RETURN,                                        // return
#if PRINT_STAT_ENABLE
    TOKEN_PRINT,                                         // print
#endif
    TOKEN_VAR,                                           // var
    TOKEN_CONST,                                         // const
    TOKEN_TRUE,                                          // true
    TOKEN_FALSE,                                         // false
    TOKEN_NULL,                                          // null
    TOKEN_DEL,                                           // del
    TOKEN_IDENTIFIER,                                    // identifier       [misc]
    TOKEN_INT_LITERAL,                                   // integer literal
    TOKEN_FLOAT_LITERAL,                                 // float literal
    TOKEN_STRING_LITERAL,                                // string literal
    TOKEN_EOF,                                           // eof
    TOKEN_LPAREN,                                        // (                [sep]
    TOKEN_RPAREN,                                        // )
    TOKEN_LBRACK,                                        // [
    TOKEN_RBRACK,                                        // ]
    TOKEN_LBRACE,                                        // {
    TOKEN_RBRACE,                                        // }
    TOKEN_COMMA,                                         // ,
    TOKEN_DOT,                                           // .
    TOKEN_COLON,                                         // :
    TOKEN_SEMI,                                          // ;
    TOKEN_QUERY,                                         // ?
    TOKEN_VARARG,                                        // ...
    TOKEN_BINOP_START,                                   //                  [binop]
    TOKEN_BINOP_LOGIC_START = TOKEN_BINOP_START,         //
    TOKEN_OR                = TOKEN_BINOP_LOGIC_START,   // or
    TOKEN_AND,                                           // and
    TOKEN_IS,                                            // is
    TOKEN_IN,                                            // in
    TOKEN_BINOP_LOGIC_END = TOKEN_IN,                    //
    TOKEN_COMPARE_START,                                 //
    TOKEN_EQ = TOKEN_COMPARE_START,                      // ==               [compare]
    TOKEN_NE,                                            // !=
    TOKEN_LT,                                            // <
    TOKEN_LE,                                            // <=
    TOKEN_GT,                                            // >
    TOKEN_GE,                                            // >=
    TOKEN_COMPARE_END = TOKEN_GE,                        //
    TOKEN_ARITH_START,                                   //
    TOKEN_BOR = TOKEN_ARITH_START,                       // |                [bit]
    TOKEN_BXOR,                                          // ^
    TOKEN_BAND,                                          // &
    TOKEN_BSHL,                                          // <<
    TOKEN_BSHR,                                          // >>
    TOKEN_ADD,                                           // +                [cal]
    TOKEN_SUB,                                           // -
    TOKEN_MUL,                                           // *
    TOKEN_DIV,                                           // /
    TOKEN_MOD,                                           // %
    TOKEN_POW,                                           // **
    TOKEN_ARITH_END = TOKEN_POW,                         //
    TOKEN_BINOP_END = TOKEN_ARITH_END,                   //
    TOKEN_NOT,                                           // not              [uniop]
    TOKEN_BNOT,                                          // ~
    TOKEN_LEN,                                           // #
    TOKEN_ASSIGN,                                        // =                [assign]
    TOKEN_COMPOUNDASSIGN_START,                          //
    TOKEN_SELFADD = TOKEN_COMPOUNDASSIGN_START,          // +=               [compoundassign]
    TOKEN_SELFSUB,                                       // -=
    TOKEN_SELFMUL,                                       // *=
    TOKEN_SELFDIV,                                       // /=
    TOKEN_SELFPOW,                                       // **=
    TOKEN_SELFMOD,                                       // %=
    TOKEN_SELFBSHL,                                      // <<=
    TOKEN_SELFBSHR,                                      // >>=
    TOKEN_SELFBAND,                                      // &=
    TOKEN_SELFBXOR,                                      // ^=
    TOKEN_SELFBOR,                                       // |=
    TOKEN_COMPOUNDASSIGN_END = TOKEN_SELFBOR,            //
} tl_token_t;
// clang-format on

#ifdef THIS_IS_IN_TL_LEXER_C
// the order must be the same as tl_token_t
const char* tl_token_strings[] = {
    "if",     "elif",  "else",  "switch", "case", "default", "label", "goto",      "break",   "continue",
    "repeat", "until", "while", "for",    "func", "return",
#if PRINT_STAT_ENABLE
    "print",
#endif
    "var",    "const", "true",  "false",  "null", "del",     "<id>",  "<integer>", "<float>", "<string>",
    "<eof>",  "(",     ")",     "[",      "]",    "{",       "}",     ",",         ".",       ":",
    ";",      "?",     "...",   "or",     "and",  "is",      "in",    "==",        "!=",      "<",
    "<=",     ">",     ">=",    "|",      "^",    "&",       "<<",    ">>",        "+",       "-",
    "*",      "/",     "%",     "**",     "not",  "~",       "#",     "=",         "+=",      "-=",
    "*=",     "/=",    "**=",   "%=",     "<<=",  ">>=",     "&=",    "^=",        "|=",
};

typedef struct symbol_token_t {
    const char* token;
    tl_token_t  type;
    char        len;
} symbol_token_t;
static const symbol_token_t symbol_tokens[] = {
    {"==", TOKEN_EQ, 2},        {"=", TOKEN_ASSIGN, 1},    {"...", TOKEN_VARARG, 3},  {".", TOKEN_DOT, 1},
    {"+=", TOKEN_SELFADD, 2},   {"+", TOKEN_ADD, 1},       {"-=", TOKEN_SELFSUB, 2},  {"-", TOKEN_SUB, 1},
    {"*=", TOKEN_SELFMUL, 2},   {"**=", TOKEN_SELFPOW, 3}, {"**", TOKEN_POW, 2},      {"*", TOKEN_MUL, 1},
    {"/=", TOKEN_SELFDIV, 2},   {"/", TOKEN_DIV, 1},       {"%=", TOKEN_SELFMOD, 2},  {"%", TOKEN_MOD, 1},
    {"<<=", TOKEN_SELFBSHL, 3}, {"<=", TOKEN_LE, 2},       {"<<", TOKEN_BSHL, 2},     {"<", TOKEN_LT, 1},
    {">>=", TOKEN_SELFBSHR, 3}, {">=", TOKEN_GE, 2},       {">>", TOKEN_BSHR, 2},     {">", TOKEN_GT, 1},
    {"&=", TOKEN_SELFBAND, 2},  {"&", TOKEN_BAND, 1},      {"^=", TOKEN_SELFBXOR, 2}, {"^", TOKEN_BXOR, 1},
    {"|=", TOKEN_SELFBOR, 2},   {"|", TOKEN_BOR, 1},       {"(", TOKEN_LPAREN, 1},    {")", TOKEN_RPAREN, 1},
    {"[", TOKEN_LBRACK, 1},     {"]", TOKEN_RBRACK, 1},    {"{", TOKEN_LBRACE, 1},    {"}", TOKEN_RBRACE, 1},
    {",", TOKEN_COMMA, 1},      {":", TOKEN_COLON, 1},     {";", TOKEN_SEMI, 1},      {"?", TOKEN_QUERY, 1},
    {"!=", TOKEN_NE, 2},        {"~", TOKEN_BNOT, 1},      {"#", TOKEN_LEN, 1},
};

typedef struct keyword_token_t {
    const char* token;
    tl_token_t  type;
} keyword_token_t;
static const keyword_token_t keyword_tokens[] = {
    {"and", TOKEN_AND},           {"not", TOKEN_NOT},       {"or", TOKEN_OR},         {"if", TOKEN_IF},
    {"elif", TOKEN_ELIF},         {"else", TOKEN_ELSE},     {"switch", TOKEN_SWITCH}, {"case", TOKEN_CASE},
    {"default", TOKEN_DEFAULT},   {"label", TOKEN_LABEL},   {"goto", TOKEN_GOTO},     {"break", TOKEN_BREAK},
    {"continue", TOKEN_CONTINUE}, {"repeat", TOKEN_REPEAT}, {"until", TOKEN_UNTIL},   {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},           {"is", TOKEN_IS},         {"in", TOKEN_IN},         {"func", TOKEN_FUNC},
    {"return", TOKEN_RETURN},
#if PRINT_STAT_ENABLE
    {"print", TOKEN_PRINT},
#endif
    {"var", TOKEN_VAR},           {"const", TOKEN_CONST},   {"true", TOKEN_TRUE},     {"false", TOKEN_FALSE},
    {"null", TOKEN_NULL},         {"del", TOKEN_DEL},
};
#else
extern const char* tl_token_strings[];
#endif

#endif
