#ifndef _TL_UTILS_H_
#define _TL_UTILS_H_

#include "tl_config.h"
#include "tl_exception.h"

#include "util_misc.h"


enum tl_panic_t {
    TL_PANIC_OK = 0,
    TL_PANIC_MEMORY,
    TL_PANIC_INTERNAL,
    TL_PANIC_SYNTAX,
    TL_PANIC_UNIMPLEMENT,
    TL_PANIC_TYPE,
    TL_PANIC_NAME,
};
extern const char* tl_panic_info[];

// clang-format off

#define tl_printf(...)                 util_printf(__VA_ARGS__)
#define tl_panic(eno, ...)             tl_printf("%s: ", tl_panic_info[eno]), tl_printf(__VA_ARGS__), tl_printf("\n"), tl_throw(eno)
#define tl_info(...)                   tl_printf("[INF] "), tl_printf(__VA_ARGS__), tl_printf("\n")
#define tl_none(...)

#if TL_DEBUG_OBJNAME_EN
#define TL_DEBUG_OBJNAME_FIELD         ; const char* name__
#define TL_DEBUG_OBJNAME(obj)          obj->name__
#define TL_DEBUG_OBJNAME_SET(obj, s)   obj->name__ = s
#else
#define TL_DEBUG_OBJNAME_FIELD
#define TL_DEBUG_OBJNAME(obj)          nullptr
#define TL_DEBUG_OBJNAME_SET(obj, s)
#endif
// clang-format on

#endif
