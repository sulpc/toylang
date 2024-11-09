#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tl_main.h"
#include "util_misc.h"

typedef enum {
    MODE_INTERACTION,
    MODE_INTERPRETE,
    MODE_FORMAT,
} work_mode_t;

work_mode_t work_mode = MODE_INTERACTION;

char tl_waitchar(void)
{
    return (char)getc(stdin);
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
