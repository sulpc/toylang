#ifndef _TL_OPS_H_
#define _TL_OPS_H_

#include "tl_env.h"
#include "tl_token.h"
#include "tl_values.h"

void tl_op_binop(tl_token_t opt, tl_value_t* a, tl_value_t* b, tl_value_t* ret);
void tl_op_uniop(tl_token_t opt, tl_value_t* v, tl_value_t* ret);
void tl_op_compoundassign(tl_token_t opt, tl_value_t* a, tl_value_t* b);
bool tl_op_conv2bool(tl_value_t* v);
bool tl_op_eq(tl_value_t* a, tl_value_t* b);
void tl_op_next(tl_value_t* c, tl_int_t* idx, tl_value_t* k, tl_value_t* v);
void tl_op_setmember(tl_value_t* c, tl_value_t* k, tl_value_t* v);
void tl_op_getmember(tl_value_t* c, tl_value_t* k, tl_value_t* v);

#endif
