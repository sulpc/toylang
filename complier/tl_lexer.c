#define THIS_IS_IN_TL_LEXER_C

#include "tl_lexer.h"
#include "tl_config.h"
#include "tl_utils.h"

#define current             (*lexer->code)
#define advance(n)          lexer_advance(lexer, n)
#define panic(eno, msg)     tl_panic(eno, "%s at [%d:%d]%c", msg, lexer->line, lexer->column, *lexer->code)
#define startwith(s)        (strncmp(lexer->code, s, strlen(s)) == 0)
#define tokensizecheck(len) (len >= TL_LEXER_MAX_TOKEN_LEN - 1) ? panic(TL_PANIC_SYNTAX, "token over size") : (void)0


static void       lexer_advance(tl_lexer_t* lexer, int n);
static void       lexer_skipwhitespaces(tl_lexer_t* lexer);
static void       lexer_skipcomment(tl_lexer_t* lexer);
static tl_token_t lexer_parsenumber(tl_lexer_t* lexer);
static tl_token_t lexer_parsestring(tl_lexer_t* lexer);
static tl_token_t lexer_parseword(tl_lexer_t* lexer);
static char       lexer_escape(tl_lexer_t* lexer);


void tl_lexer_init(tl_lexer_t* lexer, const char* code, char* buffer)
{
    // tl_size_t len = strlen(code);

    if (lexer == nullptr || code == nullptr)
        return;

    lexer->code        = code;
    lexer->tokenbuf    = buffer;
    lexer->tokenbuf[0] = '\0';
    lexer->line        = 1;
    lexer->column      = 0;
}


tl_token_t tl_lexer_nexttoken(tl_lexer_t* lexer)
{
    lexer_skipwhitespaces(lexer);

    // end
    if (current == 0) {
        lexer->tokenbuf[0] = 0;
        return TOKEN_EOF;
    }
    // digit -> number_literal
    if (isdigit(current)) {
        return lexer_parsenumber(lexer);
    }

    // '"` -> string_literal
    if (current == '\'' || current == '"' || current == '`') {
        return lexer_parsestring(lexer);
    }

    // alpha -> identifier/keyword
    if (isalnum(current) || current == '_') {
        return lexer_parseword(lexer);
    }

    // operators
    for (int i = 0; i < util_arraylen(symbol_tokens); i++) {
        if (current == symbol_tokens[i].token[0]) {
            if (startwith(symbol_tokens[i].token)) {
                strcpy(lexer->tokenbuf, symbol_tokens[i].token);
                advance(symbol_tokens[i].len);
                return symbol_tokens[i].type;
            }
        }
    }

    // other
    panic(TL_PANIC_SYNTAX, "unrecognized char");
    return TOKEN_EOF;
}


static void lexer_advance(tl_lexer_t* lexer, int n)
{
    while (n > 0) {
        if (current == 0) {
            panic(TL_PANIC_SYNTAX, "unexpected end");
        }
        lexer->column++;
        if (current == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        lexer->code++;
        n--;
    }
}


static void lexer_skipwhitespaces(tl_lexer_t* lexer)
{
    while (current != 0) {
        if (isspace(current)) {
            advance(1);
        } else if (startwith("//") || startwith("/*")) {
            lexer_skipcomment(lexer);
        } else {
            break;
        }
    }
}


static void lexer_skipcomment(tl_lexer_t* lexer)
{
    advance(1);
    if (current == '*') {   // multiline comment /*  */
        advance(1);
        while (current) {
            if (current == '*' && startwith("*/")) {
                advance(2);
                break;
            }
            advance(1);
        }
    } else {   //  oneline comment //
        advance(1);
        while (current) {
            if (current == '\n') {
                advance(1);
                break;
            }
            advance(1);
        }
    }
}


static tl_token_t lexer_parsenumber(tl_lexer_t* lexer)
{
    tl_int_t i    = 0;
    tl_int_t base = 10;

    // check base
    if (current == '0' && lexer->code[1] != '\0') {
        if (lexer->code[1] == 'b' || lexer->code[1] == 'B') {
            base = 2;
            advance(2);
        } else if (lexer->code[1] == 'x' || lexer->code[1] == 'X') {
            base = 16;
            advance(2);
        } else if (lexer->code[1] == 'o' || lexer->code[1] == 'O') {
            base = 8;
            advance(2);
        } else if (isdigit(lexer->code[1])) {
            panic(TL_PANIC_SYNTAX, "invalid num format");
        }
    }

    // integer part
    do {
        if (isdigit(current)) {
            if (current - '0' < base) {
                i = i * base + current - '0';
                advance(1);
            } else {
                panic(TL_PANIC_SYNTAX, "invalid num format");
            }
        } else if (current >= 'a' && current <= 'f' && base == 16) {
            i = (i << 4) + 10 + current - 'a';
            advance(1);
        } else if (current >= 'A' && current <= 'F' && base == 16) {
            i = (i << 4) + 10 + current - 'A';
            advance(1);
        } else {
            break;
        }
    } while (current);

    if (current != '.' || current == 0) {
        *(tl_int_t*)lexer->tokenbuf = i;
        return TOKEN_INT_LITERAL;
    }

    // fractional part
    advance(1);   // '.'
    tl_float_t f     = 0;
    tl_float_t fbase = 1.0 / base;

    while (current) {
        if (isdigit(current)) {
            if (current - '0' < base) {
                f += (current - '0') * fbase;
                fbase /= base;
                advance(1);
            } else {
                panic(TL_PANIC_SYNTAX, "invalid num format");
            }
        } else if (current >= 'a' && current <= 'f' && base == 16) {
            f += (current - 'a' + 10) * fbase;
            fbase /= base;
            advance(1);
        } else if (current >= 'A' && current <= 'F' && base == 16) {
            f += (current - 'A' + 10) * fbase;
            fbase /= base;
            advance(1);
        } else {
            break;
        }
    };

    *(tl_float_t*)lexer->tokenbuf = f + i;
    return TOKEN_FLOAT_LITERAL;
}


static tl_token_t lexer_parsestring(tl_lexer_t* lexer)
{
    int  wrpos = 0;
    char start = current;
    advance(1);   // eat ' " `

    while (current != start) {
        if ((current == 0) || (start != '`' && current == '\n')) {
            panic(TL_PANIC_SYNTAX, "literal string not end");
        }
        tokensizecheck(wrpos);
        if (current == '\r') {   // ignore
            advance(1);
        } else if (current == '\\') {   // escape char
            lexer->tokenbuf[wrpos++] = lexer_escape(lexer);
        } else {
            lexer->tokenbuf[wrpos++] = current;
            advance(1);
        }
    }
    advance(1);   // eat ' " `
    lexer->tokenbuf[wrpos] = '\0';
    return TOKEN_STRING_LITERAL;
}


static tl_token_t lexer_parseword(tl_lexer_t* lexer)
{
    int wrpos = 0;
    while (current) {
        if (isalnum(current) || current == '_') {
            tokensizecheck(wrpos);
            lexer->tokenbuf[wrpos++] = current;
            advance(1);
        } else {
            break;
        }
    }
    lexer->tokenbuf[wrpos] = '\0';

    for (int i = 0; i < util_arraylen(keyword_tokens); i++) {
        if (strcmp(lexer->tokenbuf, keyword_tokens[i].token) == 0) {
            return keyword_tokens[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}


static char lexer_escape(tl_lexer_t* lexer)
{
    advance(1);   // eat '\'
    char c = current;
    if (c == '"' || c == '\'' || c == '`') {
        advance(1);
        return c;
    }

    if (c == 'n') {
        advance(1);
        return '\n';
    }

    panic(TL_PANIC_SYNTAX, "unsupport escape");
    return 0;
}
