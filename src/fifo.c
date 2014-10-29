#include "barrier.h"

Semaphore* door0 = NULL;
Semaphore* door1 = NULL;
Semaphore* mutex = NULL;

short err_atomicles = 0;

pthread_t threads[THREADS];
int sem_timeout = 0;
int sem_persist = 1;
int sem_index   = 0;
key_t sem_key   = 19;
int count       = 0;

#define WASTE_TIME (nanosleep((struct timespec[]){{0, rand() % 100000000}}, NULL))
#define STMT_A ( printf("[#%0#10lx:%x] %02d AAAAAAAA\n", self, ((((unsigned int)self)&0xf000)>>12), count) )
#define STMT_B ( printf("[#%0#10lx:%x] %02d         BBBBBBBB\n", self, ((((unsigned int)self)&0xf000)>>12), count) )

void *fifo(unsigned int *index) {
    int i;
    pthread_t self = pthread_self();

    for(i=0; i<3; i++) {
        Semaphore$lock(mutex, 0, 1, 0); {
            count++;
            STMT_A;
            if(count == THREADS) {
                Semaphore$lock(door1, sem_index, sem_persist, 0);
                Semaphore$unlock(door0, sem_index, sem_persist);
            }
        } Semaphore$unlock(mutex, 0, 1);

        Semaphore$lock(door0, sem_index, sem_persist, 0);
        Semaphore$unlock(door0, sem_index, sem_persist);

        Semaphore$lock(mutex, 0, 1, 0); {
            count--;
            STMT_B;
            if(count == 0) {
                Semaphore$lock(door0, sem_index, sem_persist, 0);
                Semaphore$unlock(door1, sem_index, sem_persist);
            }
        } Semaphore$unlock(mutex, 0, 1);

        Semaphore$lock(door1, sem_index, sem_persist, 0);
        Semaphore$unlock(door1, sem_index, sem_persist);

    }
    return NULL;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    unsigned int i;

    //. Set-up
    unsigned short size  = 1;
    bool attach = true;

    if((door0 = Semaphore$new(sem_key+2, size, 0, attach)) == NULL) {
        printf("! Failed to create mutex\n");
    }

    if((door1 = Semaphore$new(sem_key+1, size, 1, attach)) == NULL) {
        printf("! Failed to create mutex\n");
    }

    if((mutex = Semaphore$new(sem_key, size, 1, attach)) == NULL) {
        printf("! Failed to create semaphore\n");
    }

    //. Creat threads
    for(i=0; i<THREADS; i++) {
        int x = i;
        if(!pthread_create(&threads[i], NULL, (void *(*)(void *))&barrier, &x) == 0) {
            printf("! Failed to create thread #%d\n", i);
        }
    }

    //. Wait for threads
    int* returns[THREADS];
    for(i=0; i<THREADS; i++) {
        //printf("~ Waiting for %d...\n", i);
        pthread_join(threads[i], (void**)&(returns[i]));
        //printf("Thread 1 returned %d\n", *(returns[i]));
    }

    //. Clean-up
    Semaphore$delete(&mutex, 1);
    Semaphore$delete(&door0, 1);
    Semaphore$delete(&door1, 1);

    return 0;
}
