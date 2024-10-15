#include "tl_env.h"
#include "tl_exception.h"
#include "tl_gc.h"
#include "tl_input.h"
#include "tl_interpreter.h"
#include "tl_parser.h"

#include "util_misc.h"

#ifdef HOST_DEBUG
#include <time.h>
#endif

typedef enum {
    MODE_INTERACTION,
    MODE_INTERPRETE,
    MODE_FORMAT,
} work_mode_t;

work_mode_t work_mode  = MODE_INTERACTION;
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


#ifdef HOST_DEBUG
int main(int argc, char* argv[])
{
    static char code[2048];
    const char* prefix = "# ";
    clock_t     time[8];

    if (argc > 1) {
        char* fname = nullptr;
        if (argc > 2 && strcmp(argv[1], "-f") == 0) {
            work_mode = MODE_FORMAT;
            fname     = argv[2];
        } else {
            work_mode = MODE_INTERPRETE;
            fname     = argv[1];
        }

        FILE* file = fopen(fname, "r");
        if (file) {
            fread(code, 1, sizeof(code), file);
        } else {
            util_printf("open file fail\n");
            return 0;
        }
    } else {
        work_mode = MODE_INTERACTION;
    }

    time[0] = clock();

    tl_startup();
    time[1] = clock();

    while (true) {
        if (work_mode == MODE_INTERACTION) {
            util_printf(prefix);
            tl_getline(code, sizeof(code));
        }
        int ret = tl_interpret(code, work_mode == MODE_FORMAT);
        time[2] = clock();

        if (ret < 0) {
            break;
        } else if (work_mode != MODE_INTERACTION) {
            break;
        }
    }

    tl_cleanup();
    time[3] = clock();

    if (work_mode == MODE_INTERPRETE) {
        util_printf(
            "time used   : %ld\n"
            "  startup   : %ld\n"
            "  interpret : %ld\n"
            "  cleanup   : %ld\n",
            time[3] - time[0],   //
            time[1] - time[0],   //
            time[2] - time[1],   //
            time[3] - time[2]);
    }

    return 0;
}
#endif
