#ifndef _TL_INPUT_H_
#define _TL_INPUT_H_

#include "tl_types.h"

int tl_getline(char* buf, tl_size_t size);

/**
 * @brief wait a char, should implements by user, CALLOUT
 *
 * @return char
 */
extern char tl_waitchar(void);

#endif
