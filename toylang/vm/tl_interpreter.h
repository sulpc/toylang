#ifndef _TL_INTERPRETER_H_
#define _TL_INTERPRETER_H_

#include "tl_env.h"
#include "tl_parser.h"
#include "tl_values.h"

void tl_interpreter_start(void);
void tl_interpreter_exit(void);
void tl_interpreter_interpret(tl_program_t* program);

#endif   // _TL_INTERPRETER_H_
