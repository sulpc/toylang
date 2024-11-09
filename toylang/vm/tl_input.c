#include "tl_input.h"
#include "tl_types.h"

int tl_getline(char* buf, tl_size_t size)
{
    int len = 0;

    while (true) {
        char c = tl_waitchar();
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
