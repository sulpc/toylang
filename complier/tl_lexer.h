#ifndef _TL_LEXER_H_
#define _TL_LEXER_H_

// #include "tl_values.h"

#include "tl_token.h"
#include "tl_types.h"

typedef struct {
    const char* code;       // source code
    char*       tokenbuf;   // buffer size >= TL_LEXER_MAX_TOKEN_LEN
    uint16_t    line;
    uint16_t    column;
} tl_lexer_t;

void       tl_lexer_init(tl_lexer_t* lexer, const char* code, char* buffer);
tl_token_t tl_lexer_nexttoken(tl_lexer_t* lexer);

#define tl_lexer_line(lexer)   (lexer)->line
#define tl_lexer_column(lexer) (lexer)->column
#define tl_tokenstr(token)     tl_token_strings[token]
#endif
