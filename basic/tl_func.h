#ifndef _TL_FUNC_H_
#define _TL_FUNC_H_

#include "tl_env.h"
#include "tl_values.h"

tl_func_t* tl_func_new(void* ast);
void       tl_func_free(tl_func_t* func);

#endif
