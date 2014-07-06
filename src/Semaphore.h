#ifndef __SEMAPHORE__
#define __SEMAPHORE__

#include "common.h"

#include <time.h>       /* ctime */
#include <unistd.h>     /* sleep */
#include <errno.h>      /* perror */
#include <stdio.h>      /* perror */
#include <stdlib.h>     /* malloc */
#include <semaphore.h>  /* Semaphore */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define LOCK_NONBLOCK -1
#define LOCK_BLOCKING 0

extern short err_atomicles;

typedef struct _Semaphore {
    key_t key;
    int id;
    signed short size;
} Semaphore;

/*
Create a semaphore set of size `size' < SSHRT_MAX.
*/
Semaphore* Semaphore$new(
    key_t key,
    signed short size,
    signed short initial,
    bool attach
);
Semaphore *Semaphore$attach(key_t key);
void Semaphore$delete(Semaphore **this, short remove_sem_too);

short Semaphore$lock(
    Semaphore *this,
    unsigned short index,
    bool persist,
    time_t timeout
);
short Semaphore$unlock(
    Semaphore *this,
    unsigned short index,
    bool persist //. persist even if last process dies out
);

int Semaphore_exists(key_t key);
int Semaphore$value(Semaphore *this);
int Semaphore$size(Semaphore *this);
int Semaphore$current(Semaphore *this, unsigned short index);
time_t Semaphore$ctime(Semaphore *this);
void Semaphore$desc(Semaphore *this, unsigned short index);

#endif
