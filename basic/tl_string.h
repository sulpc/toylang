#ifndef _TL_STRING_H_
#define _TL_STRING_H_

#include "tl_env.h"

#define tl_string_size(str) (sizeof(tl_string_t) + str->len + 1)

void         tl_stringpool_init(void);
void         tl_stringpool_deinit(void);
tl_string_t* tl_string_new(const char* cstr);
void         tl_string_free(tl_string_t* tstr);
tl_string_t* tl_string_newex(const char* cstr);
void         tl_string_freeex(tl_string_t* tstr);
bool         tl_string_eq(tl_string_t* s1, tl_string_t* s2);
void         tl_stringpool_display(void);

#endif
