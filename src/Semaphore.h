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

typedef struct _Semaphore {
    key_t key;
    int id;
    unsigned short count;
    unsigned short size;
} Semaphore;

/*
* Create a semaphore set of `count', each of size `size'...
*/
Semaphore* Semaphore$new(
    key_t key,
    unsigned short count,
    unsigned short size,
    unsigned short attach
);
Semaphore *Semaphore$attach(key_t key);
void Semaphore$delete(Semaphore **this, short remove_sem_too);

short Semaphore$lock(
    Semaphore *this,
    unsigned short index,
    unsigned short persist,
    time_t timeout
);
short Semaphore$unlock(
    Semaphore *this,
    unsigned short index,
    unsigned short persist
);
//. persist: Persist even if last process dies out

int Semaphore_exists(key_t key);
int Semaphore$count(Semaphore *this);
int Semaphore$used(Semaphore *this, unsigned short index);
int Semaphore$size(Semaphore *this);
time_t Semaphore$ctime(Semaphore *this);
void Semaphore$set_size(Semaphore *this, unsigned short size);
void Semaphore$desc(Semaphore *this, unsigned short index);

#endif
