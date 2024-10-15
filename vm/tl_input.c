#include "tl_input.h"
#include "tl_types.h"

#ifdef HOST_DEBUG
#include <stdio.h>
#define wait_char() (char)getc(stdin)
#else
extern char wait_char(void);
#endif

int tl_getline(char* buf, tl_size_t size)
{
    int len = 0;

    while (true) {
        char c = wait_char();
        if (c == '\r') {   // ignore
            continue;
        }
        if (c == '\b') {
            if (len > 0)
                len--;
            continue;
        }
        if (c == '\n' || len == size - 1) {
            buf[len] = '\0';
            break;
        } else {
            buf[len++] = c;
        }
    }

    return len;
}
