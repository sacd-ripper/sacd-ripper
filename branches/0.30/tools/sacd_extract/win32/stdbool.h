#ifndef STDBOOL_WIN32_H
#define STDBOOL_WIN32_H

#ifndef _WIN32
#error "This stdbool.h file should only be compiled under Windows"
#endif

#ifndef __cplusplus

typedef unsigned char bool;

#define true 1
#define false 0

#ifndef CASSERT
#define CASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1];
#endif

CASSERT(sizeof(bool) == 1, bool_is_one_byte)
CASSERT(true, true_is_true)
CASSERT(!false, false_is_false)

#endif

#endif