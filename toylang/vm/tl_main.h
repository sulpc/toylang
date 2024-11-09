#pragma once

#include "tl_input.h"

void tl_startup(void);
void tl_cleanup(void);
int  tl_interpret(const char* code, bool format);
