#include <assert.h>

#ifndef __COMMON__
#define __COMMON__

#define BIT_SHSEM   0x08
#define BIT_SHMEM   0x04

#define BIT_MISSING 0x01
#define BIT_EXISTS  0x02

#define SHMKEYPATH "/dev/zero"
#define SEMKEYPATH "/dev/null"

#define SHMKEY(key) ftok(SHMKEYPATH, (key))
#define SEMKEY(key) ftok(SEMKEYPATH, (key))

typedef enum { false = 0, true = 1 } bool;

#endif
