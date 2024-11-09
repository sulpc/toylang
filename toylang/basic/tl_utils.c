#include "tl_utils.h"

const char* tl_panic_info[] = {
    [TL_PANIC_MEMORY]      = "Memory Error",          //
    [TL_PANIC_INTERNAL]    = "Internal Error",        //
    [TL_PANIC_SYNTAX]      = "Syntax Error",          //
    [TL_PANIC_UNIMPLEMENT] = "Unimplemented Error",   //
    [TL_PANIC_TYPE]        = "Type Error",            //
    [TL_PANIC_NAME]        = "Name Error",            //
};
