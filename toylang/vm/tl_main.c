#include "tl_env.h"
#include "tl_exception.h"
#include "tl_gc.h"
#include "tl_input.h"
#include "tl_interpreter.h"
#include "tl_parser.h"

#include "util_misc.h"


const char* split_line = "------------------------------------------\n";
const char* banner =
    " ______               __                    \n"
    "/_  __/ ___   __ __  / /  ___ _  ___   ___ _\n"
    " / /   / _ \\ / // / / /__/ _ `/ / _ \\ / _ `/\n"
    "/_/    \\___/ \\_, / /____/\\_,_/ /_//_/ \\_, / \n"
    "            /___/                    /___/  \n";


void tl_startup(void)
{
    util_printf(banner);
    util_printf(split_line);

    tl_trycatch(
        {
            tl_env_init();
            tl_interpreter_start();
        },
        {
            util_printf("catch exception<%d> when init\n", current_es);
            return;
        });
}

void tl_cleanup(void)
{
    util_printf(split_line);
    tl_trycatch(
        {
            tl_interpreter_exit();
            tl_gc_cleanall();
        },
        {
            util_printf("catch exception<%d> when clean\n", current_es);
            return;
        });

    tl_trycatch(
        {
            tl_env_deinit();
            if (tl_mem_used()) {
                util_printf("!!! memory leak! gc=%d, all=%d\n", tl_gc_memused(), tl_mem_used());
                tl_env_display();
            }
        },
        {
            util_printf("catch exception<%d> when deinit\n", current_es);
            tl_env_display();
            return;
        });
}

/**
 * @brief
 *
 * @param code
 * @return int : <0: error or exit(must quit the interpreter), 0: ok, >0: warning(could ignore in interaction mode)
 */
int tl_interpret(const char* code, bool format)
{
    static tl_parser_t  parser;
    static tl_program_t program;

    if (strcmp(code, "exit") == 0) {
        util_printf("bye\n");
        return -1;
    }
    if (strcmp(code, "envinfo") == 0) {
        tl_env_display();
        return 0;
    }
    if (strcmp(code, "free") == 0) {
        tl_parser_free_error();
        tl_gc_cleanall();
        return 0;
    }

    tl_trycatch(
        {
            tl_parser_init(&parser, code);
            program = tl_parser_parse(&parser);
        },
        {
            tl_parser_free_error();
            tl_gc_cleanall();
            return 1;
        });

#ifdef HOST_DEBUG
    if (format) {
        tl_trycatch(
            {
                // format
                tl_parser_dump(&program);
            },
            {
                // util_printf("catch exception<%d> when dump\n", current_es);
            });
    } else
#endif
    {
        tl_trycatch(
            {
                // run
                tl_interpreter_interpret(&program);
            },
            {
                // util_printf("catch exception<%d> when interpret\n", current_es);
            });
    }

    tl_trycatch(
        {
            // free ast tree
            tl_parser_free(&program);
        },
        {
            util_printf("catch exception<%d> when free\n", current_es);
            return -1;
        });

    tl_trycatch(
        {
            // gc
            tl_gc_cleanall();
        },
        {
            util_printf("catch exception<%d> when cleanall\n", current_es);
            return -1;
        });

    return 0;
}
