#ifndef _TL_EXCEPTION_H_
#define _TL_EXCEPTION_H_

#include "util_misc.h"

typedef struct tl_exception_t {
    struct tl_exception_t* previous;
    jmp_buf                jb;
    int                    status;
} tl_exception_t;

extern tl_exception_t* g_exception__;

#define tl_try(stat)                                                                                                   \
    tl_exception_t exception = {                                                                                       \
        .previous = g_exception__,                                                                                     \
        .status   = 0,                                                                                                 \
    };                                                                                                                 \
    g_exception__ = &exception;                                                                                        \
    if (setjmp(g_exception__->jb) == 0) {                                                                              \
        stat                                                                                                           \
    }
#define tl_catch(eno, stat)                                                                                            \
    {                                                                                                                  \
        int eno       = g_exception__->status;                                                                         \
        g_exception__ = exception.previous;                                                                            \
        if (eno != 0) {                                                                                                \
            stat                                                                                                       \
        }                                                                                                              \
    }
#define tl_trycatch(try_stat, catch_stat)                                                                              \
    {                                                                                                                  \
        tl_exception_t exception__ = {.previous = g_exception__, .status = 0};                                         \
        g_exception__              = &exception__;                                                                     \
        if (setjmp(g_exception__->jb) == 0) {                                                                          \
            try_stat                                                                                                   \
        }                                                                                                              \
        g_exception__ = exception__.previous;                                                                          \
        if (exception__.status != 0) {                                                                                 \
            catch_stat                                                                                                 \
        }                                                                                                              \
    }
#define tl_safetry(stat)                                                                                               \
    {                                                                                                                  \
        tl_exception_t exception__ = {.previous = g_exception__, .status = 0};                                         \
        g_exception__              = &exception__;                                                                     \
        if (setjmp(g_exception__->jb) == 0) {                                                                          \
            stat                                                                                                       \
        }                                                                                                              \
        g_exception__ = exception__.previous;                                                                          \
    }
#define tl_throw(n) g_exception__->status = n, longjmp(g_exception__->jb, 1)
#define current_es  exception__.status

#endif
