#include <assert.h>
#include <string.h>
#include <stdarg.h>

#ifdef _AIXVERSION_530
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#else
#define GETOPT_LONG
#endif

#ifdef GETOPT_LONG
#include <getopt.h>
#endif

#include "Semaphore.h"

#include "SharedMemory.h"

int unlock(unsigned int shsem_key);
int lock(unsigned int shsem_key, int timeout);
int status(unsigned int shsem_key);
int summary(unsigned int shsem_key);
int create(unsigned int shsem_key, unsigned int semaphores, time_t expires);
int delete(unsigned int shsem_key);
void usage(void);

#define SHSEM_COUNT 1 //. We only need a semaphore set of size 1
#define SHSEM_SIZE  1 //. How many semaphores, 1 means mutex
#define SHSEM_INDEX 0

#define min(x, y) ((x)<(y)?(x):(y))
