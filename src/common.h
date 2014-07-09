#define _GNU_SOURCE

#include "dbg.h"

#ifndef __COMMON__
#define __COMMON__

//. The left and right flags can be ORed: -={
//. 8-bit left flag
#define FLAG_SHSEM   0x80
#define FLAG_SHMEM   0x40

//. 8-bit right flag
#define FLAG_UNKNOWN 0x0F
#define FLAG_UNAVAIL 0x08
#define FLAG_TIMEOUT 0x04
#define FLAG_EXISTS  0x02
#define FLAG_MISSING 0x01
//. }=-

typedef enum { false = 0, true = 1 } bool;

/* Deprecated:
void dbg_printf(FILE *pipe, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(pipe, fmt, ap);
    va_end(ap);
}
*/

#endif
