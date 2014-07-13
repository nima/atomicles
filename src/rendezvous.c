#include "rendezvous.h"

short err_atomicles = 0;

Semaphore* semaphores[THREADS];
pthread_t threads[THREADS];
int sem_timeout = 0;
int sem_persist = 1;
int sem_index   = 0;

void *rendezvous(unsigned int *key) {
    pthread_t self = pthread_self();

    unsigned int k = *key;
    Semaphore *s = semaphores[k];

    dbg_info("[#%0#10lx] thread-#%d AAAAAAAA", self, k);

    Semaphore$unlock(s, sem_index, sem_persist);

    unsigned int i = (k == 0) ? 1 : 0;
    Semaphore$lock(semaphores[i], sem_index, sem_persist, sem_timeout);

    dbg_info("[#%0#10lx] thread-#%d         BBBBBBBB", self, k);

    return NULL;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    unsigned int i;

    key_t keys[THREADS];

    //. Set-up
    unsigned short count  = 1;
    unsigned short size   = 1;
    unsigned short attach = 1;
    for(i=0; i<THREADS; i++) {
        keys[i] = i;
        if((semaphores[i] = Semaphore$new(keys[i], count, size, attach)) == NULL) {
            dbg_errr("! Failed to create semaphore #%d", i);
        } else {
            Semaphore$lock(semaphores[i], sem_index, sem_persist, sem_timeout);
            dbg_dbug("+ Created semaphore #%d and locked", i);
        }
    }

    //. Creat threads
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            if(pthread_create(&threads[i], NULL, (void *(*)(void *))&rendezvous, &keys[i]) == 0)
                dbg_dbug("+ Created thread #%d", i);
            else
                dbg_errr("! Failed to create thread #%d", i);
        }
    }

    //. Wait for threads
    int* returns[THREADS];
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            dbg_dbug("~ Waiting for %d...", i);
            pthread_join(threads[i], (void**)&(returns[i]));
        }
    }

    //. Clean-up
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            dbg_dbug("- Cleaning semaphore #%d", i);
            Semaphore$delete(&(semaphores[i]), 1);
        }
    }

    return 0;
}
