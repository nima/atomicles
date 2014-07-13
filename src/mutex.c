#include "mutex.h"

short err_atomicles = 0;

Semaphore* m;
pthread_t threads[THREADS];
int sem_timeout = 0;
int sem_persist = 1;
int sem_index   = 0;
int count       = 0;

void *mutex() {
    pthread_t self = pthread_self();

    Semaphore$lock(m, sem_index, sem_persist, sem_timeout);
    dbg_info("[#%0#10lx] count: %d -> %d", self, count, count+1);
    count++;
    Semaphore$unlock(m, sem_index, sem_persist);

    return NULL;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    unsigned int i;

    //. Set-up
    unsigned short count  = 1;
    unsigned short size   = 1;
    unsigned short attach = 1;
    if((m = Semaphore$new(22, count, size, attach)) == NULL) {
        dbg_errr("! Failed to create a mutex");
    } else {
        dbg_dbug("+ Created a mutex");
    }

    //. Creat threads
    for(i=0; i<THREADS; i++) {
        if(pthread_create(&threads[i], NULL, (void *(*)(void *))&mutex, NULL) == 0)
            dbg_dbug("+ Created thread #%d", i);
        else
            dbg_errr("! Failed to create thread #%d", i);
    }

    //. Wait for threads
    int* returns[THREADS];
    for(i=0; i<THREADS; i++) {
        dbg_dbug("~ Waiting for %d...", i);
        pthread_join(threads[i], (void**)&(returns[i]));
    }

    //. Clean-up
    for(i=0; i<THREADS; i++) {
        if(m) {
            dbg_dbug("- Cleaning semaphore #%d", i);
            Semaphore$delete(&m, 1);
        }
    }

    return 0;
}
