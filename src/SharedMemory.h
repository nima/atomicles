#ifndef __SHARED_MEMORY__
#define __SHARED_MEMORY__

#include "common.h"

#define SHMKEY(key) ftok(SHMKEYPATH, (key))

#include "Semaphore.h"

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <string.h>     /* String handling */
#include <stdarg.h>
//#include <elf.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/shm.h>

//. TODO: https://www.opengroup.org/onlinepubs/000095399/functions/shm_open.html

extern short err_atomicles;

typedef struct _SharedMemory {
    key_t key;       /* key to be passed to shmget(ftok()) */
    int id;          /* return value from shmget() */
    size_t size;     /* size to be passed to shmget() */
    void *addr;
    Semaphore *shsem;
} SharedMemory;

typedef u_int32_t uint32_t;

SharedMemory* SharedMemory$new(key_t key, size_t size, bool attach);
SharedMemory* SharedMemory$attach(key_t key);
void SharedMemory$delete(SharedMemory **this, bool remove_shm_too);

void SharedMemory$write(SharedMemory *this, unsigned short offset, const char *fmt, ...);
void SharedMemory$read(SharedMemory *this, unsigned short offset, char *buffer);

void SharedMemory$write_bool(SharedMemory *this, unsigned short offset, bool b);
bool SharedMemory$read_bool(SharedMemory *this, unsigned short offset);

void SharedMemory$write_uint(SharedMemory *this, unsigned short offset, unsigned int n);
unsigned int SharedMemory$read_uint(SharedMemory *this, unsigned short offset);

void SharedMemory$write_uint32(SharedMemory *this, unsigned short offset, uint32_t n);
uint32_t SharedMemory$read_uint32(SharedMemory *this, unsigned short offset);

size_t SharedMemory$size(SharedMemory *this);

void SharedMemory$dump(SharedMemory *this);

#endif
