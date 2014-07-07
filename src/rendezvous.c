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

    //printf("[#%0#10lx] thread-#%d starting\n", self, k);
    unsigned long l;
    for(l=0; l<(0xffff&self); l++);

    //printf("[#%0#10lx] thread-#%d statement(A)\n", self, k);
    printf("[#%0#10lx] thread-#%d AAAAAAAA\n", self, k);

    //printf("[#%0#10lx] thread-#%d unlocking thread-#%d\n", self, k, k);
    Semaphore$unlock(s, sem_index, sem_persist);

    unsigned int i;
    for(i=0; i<THREADS; i++) {
        if(i != k) {
            //printf("[#%0#10lx] thread-#%d locking on thread-#%d\n", self, k, i);
            Semaphore$lock(semaphores[i], sem_index, sem_persist, sem_timeout);
        }
    }

    printf("[#%0#10lx] thread-#%d         BBBBBBBB\n", self, k);
    //printf("[#%0#10lx] thread-#%d statement(B)\n", self, k);
    for(l=0; l<(0xffff&self); l++);

    //printf("[#%0#10lx] thread-#%d stopping\n", self, k);

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
            printf("! Failed to create semaphore #%d\n", i);
        } else {
            Semaphore$lock(semaphores[i], sem_index, sem_persist, sem_timeout);
            printf("+ Created semaphore #%d and locked\n", i);
        }
    }

    //. Creat threads
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            if(pthread_create(&threads[i], NULL, (void *(*)(void *))&rendezvous, &keys[i]) == 0)
                printf("+ Created thread #%d\n", i);
            else
                printf("! Failed to create thread #%d\n", i);
        }
    }

    //. Wait for threads
    int* returns[THREADS];
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            printf("~ Waiting for %d...\n", i);
            pthread_join(threads[i], (void**)&(returns[i]));
            //printf("Thread 1 returned %d\n", *(returns[i]));
        }
    }

    //. Clean-up
    for(i=0; i<THREADS; i++) {
        if(semaphores[i]) {
            printf("- Cleaning semaphore #%d\n", i);
            Semaphore$delete(&(semaphores[i]), 1);
        }
    }

    return 0;
}
