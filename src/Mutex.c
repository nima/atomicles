//. Linux...
sem_t mutex;

//. Initialize...
/*
initialize mutex        //. semaphore is local (>0 -> NOT local)
second param            //. 1: binary semaphore/mutex, More: counting semaphore
*/
if(sem_init(&(this->mutex), 0, this->count))
    perror("sem_init");

//. Destroy...
if(sem_destroy(&((*this)->mutex)))
    perror("sem_destroy");

//. Lock...
sem_wait(&(this->mutex));

//. Unlock...
sem_post(&(this->mutex));
