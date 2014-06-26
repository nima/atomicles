#include "Semaphore.h"

#define SFMT (\
    (sizeof(key_t) == sizeof(int)) \
    ? \
        "Id:           %i\n" \
        "Key:          %i\n" \
        "Resource:     %i of %i semaphores used\n" \
        "Semaphore:    Semaphore %i of %i\n" \
        "Last Changed: %s" \
    : \
        "Id:           %i\n" \
        "Key:          %li\n" \
        "Resource:     %i of %i semaphores used\n" \
        "Semaphore:    Semaphore %i of %i\n" \
        "Last Changed: %s" \
)
#include <assert.h>

/*******************************************************************************
Note
----

Obviouslt there are no `classes' in C, and thus no `methods' either.  That
doesn't mean we can't pretend that there are!

Next, let's define a semaphore; a semaphore is like an integer, with 3
differences:

    * You can initialize a semaphore to any integer value, just like you can
      with an integer type.  However unlike an integer, once the semaphore has
      been initialized, the only mutations allowed as `increment', or
      `decrement'.  Furthermore, you can not access the current value of a
      semaphore.
    * When a thread decrements a semaphore, if the result is negative, the
      thread blobks itself (sleeps), and cannot continue until another thread
      increments the semaphore.
    * When a thread increments a semaphore, if there are other threads waiting,
      one of the threads will get unblocked (awakended).


Note that the semaphore object itself cannot store user-data such as the expiry
date if one is desired, or the size of a smaphore.
*******************************************************************************/

#define NOP 0

/* Private Methods */
int Semaphore$extract_count(Semaphore *this);
int Semaphore$init(Semaphore *this, unsigned short size);

/* Public Methods */
Semaphore* Semaphore$new(key_t key, unsigned short count, unsigned short size, unsigned short attach) {
    /*
    @abstract: Create a new Semaphore object

    @discussion: The new object will contain a semaphore set of size `count`,
    and each on of those will be of size `size'.  If the object with the
    user-supplied key already exists, then this method will attempt to
    reattach to that shsem instead of failing - if the user has allowed this.

    @return: On success, this method returns a pointer to the new object,
    otherwise it will return NULL.
    */

    Semaphore *this = (Semaphore *)malloc(sizeof(Semaphore));

    if(this == NULL)
        return this;

    this->count = count;
    this->size = size;

    int success = -1;
    this->key = key;
    key_t k = SEMKEY(key);
    assert(k != -1);

    this->id = semget(k, this->count, 0666|IPC_CREAT|IPC_EXCL);
    if(this->id != -1) { //. New semaphores...
        //. Initialize the semaphores initial values...
        success = Semaphore$init(this, size);
    } else if(attach) { //. Existing semaphore, attach if user has allowed...
        if(errno == EEXIST) { //. Reattach...
            this->id = semget(k, NOP, 0666|IPC_EXCL);
            if(this->id != -1) success = EXIT_SUCCESS;
            else perror("sem:new:semget@reattach");
        } else perror("sem:new:semget@create");
    } else {
        perror("[ERROR:semget()]");
        fprintf(stderr, "[ERROR:semget()]: Semaphore %u already exists\n", k);
    }
    /* FIXME - This happens when 2 terminals are started concurrently sometimes...
       Waiting on lock...sem:new:semget@exists: File exists
    */

    if(success != EXIT_SUCCESS)
        Semaphore$delete(&this, 0);

    return this;
}

void Semaphore$delete(Semaphore **this, short remove_sem_too) {
    /*
    @abstract: Destructor method.

    @discussion: This method takes a second argument.  If this argument is set
    to a non-zero value, other than the semaphore object itself which has
    been malloced on the heap, the shsem segment in kernel memory will too be
    removed.  Either way, the malloc will be freed though.
    */
    if(remove_sem_too)
        if(semctl((*this)->id, NOP, IPC_RMID))
            perror("semctl:delete");

    free(*this);
    *this = NULL;
}

int Semaphore_exists(key_t key) {
    key_t k = SEMKEY(key);
    int id = semget(k, NOP, 0666|IPC_EXCL);

    int exists = (id != -1);
    if(!exists)
        perror("semget:exists");

    return exists;
}

Semaphore *Semaphore$attach(key_t key) {
    /*
    @abstract: Attach to an already existing semaphore...

    @discussion: This method attempts to re-attach to a shsem that is
    assumed to already exist in memory.

    @returns: On success, the semaphore object is returned, otherwise NULL.
    */

    Semaphore *this = NULL;
    key_t k = SEMKEY(key);
    int id = semget(k, NOP, 0666|IPC_EXCL);
    if(id != -1) {
        this = (Semaphore *)malloc(sizeof(Semaphore));
        this->id = id;
        this->key = key;

        //. Extract `count' from shsem...
        this->count = Semaphore$extract_count(this);

        //. Set size to 1 (mutex)...  The user will need to readjust this value
        //. manually if they're after a counting semaphore instead of this
        //. default binary semaphore (mutex) via the Semaphore$set_size()
        //. method.
        this->size = 1;
    } else perror("sem:attach:semget");
    return this;
}

int Semaphore$used(Semaphore *this, unsigned short index) {
    /*
    @abstract: Returns the number of used semaphores.

    @discussion: Returns the number of used semaphores for the semaphore at the
    given index in the semaphore set.
    */
    int used = this->size - semctl(this->id, index, GETVAL);
    return used;
}

time_t Semaphore$ctime(Semaphore *this) {
    struct semid_ds t_semid_ds;
    semctl(this->id, NOP, IPC_STAT, &t_semid_ds);
    return t_semid_ds.sem_ctime;
}

void Semaphore$desc(Semaphore *this, unsigned short index) {
    /*
    @abstract: Describe the state of the semaphore and dumpt this to stdout.
    */
    struct semid_ds t_semid_ds;
    semctl(this->id, NOP, IPC_STAT, &t_semid_ds);

    assert(this->count == Semaphore$count(this));
    int used = Semaphore$used(this, index);
    fprintf(
        stdout, SFMT,
        this->id, this->key,
        used, this->size,
        index+1, this->count,
        ctime(&(t_semid_ds.sem_ctime))
    );
}


int Semaphore$size(Semaphore *this) {
    /*
    @abstract: Simple accessor method, returns the semaphore size.

    @discussion: A size of 1 indicates that this is a binary semaphore, otherwise
    known as a mutex.  A size greater than 1 indicates that this is in fact a
    counting semaphore.
    */
    return this->size;
}

void Semaphore$set_size(Semaphore *this, unsigned short size) {
    /*
    @abstract: Set the size (limit) of all semaphores in the semaphore set.
    */
    this->size = size;
}

int Semaphore$count(Semaphore *this) {
    /*
    @abstract: Simple accessor method, returns the semaphore set size.
    */
    return this->count;
}

int Semaphore$init(Semaphore *this, unsigned short size) {
    /*
    @abstract: Sets the initial values of all semaphore in the semaphore set.
    */
    int success = EXIT_FAILURE;

    short sarray[this->count];
    int i;
    for(i=0; i<this->count; i++)
        sarray[i] = size;

    if(semctl(this->id, NOP, SETALL, sarray))
        perror("semctl:new");
    else success = EXIT_SUCCESS;

    return success;
}

int Semaphore$extract_count(Semaphore *this) {
    /*
    @abstract: Extract the size of the semaphore set

    @discussion:  Where the user has reattached to an existing semaphore
    object, this method is required to figure out what the set-size of the
    semaphore-set was at the time of its creation (via the Semaphore$new()
    method).
    */
    assert(this->id != -1);
    struct semid_ds t_semid_ds;
    semctl(this->id, NOP, IPC_STAT, &t_semid_ds);
    return t_semid_ds.sem_nsems;
}

//! Decrement and block if the result if negative
//! decrement, wait, P, lock
short Semaphore$lock(Semaphore *this, unsigned short index, unsigned short persist, time_t timeout) {
    /*
    @abstract: Lock 1 unit of this semaphores resource count.

    @discussion: For a mutex, this 1 is enough to deplete it, so the next call
    will block, unless a positive timeout is set.  A positive timeout indicates
    how many seconds the method will block for before giving up.  A negative 1
    (-1) indicates that the call should not block at all.

    The `persist' option when set, indicates that the kernel should not maintain
    the UNDO data structures for this shsem.  That means that if the process
    that called this method was killed while this method was in the critical
    area, the kernel will not revert to the previous state.

    @returns: On success, this method returns 0.  Success means that the lock
    was successfully made.  Where the call is non-blocking, a return value of 1
    indicates that the timeout was reached, and no locks could be made.  A -1
    indicates that something unexpected occured, which should never happen in
    normal use.
    */

    short e = 0;

    struct sembuf op;
    op.sem_num = index;
    op.sem_op = -1;
    op.sem_flg = persist?0:SEM_UNDO;

    if(timeout == LOCK_BLOCKING) {
        //. Block...
        if(semop(this->id, &op, (size_t)1)) {
            if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                perror("semop:lock");
                e = -1;
            }
        }
    } else if(timeout == LOCK_NONBLOCK) {
        //. Instantaeneos (Non-Blocking)...
        op.sem_flg |= IPC_NOWAIT;
        if(semop(this->id, &op, (size_t)1)) {
            if(errno == EAGAIN) {
                //. Timed out, still can't lock...
                e = 1;
            } else if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                perror("semop:lock");
                e = -1;
            }
        }
    } else {
        //. Blocking for at most `timeout' seconds...

#ifdef Linux
        struct timespec t;
        t.tv_sec = timeout;
        t.tv_nsec = 0;
        if(semtimedop(this->id, &op, (size_t)1), t) {
            if(errno == EAGAIN) {
                //. Timed out, still can't lock...
                e = 1;
            } else if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                perror("semtimedop:lock");
                e = -1;
            }
        }
#else
        time_t t;
        op.sem_flg |= IPC_NOWAIT;
        e = 1;
        for(t=0; t<timeout; t++) {
            if(semop(this->id, &op, (size_t)1)) {
                if(errno == EAGAIN) {
                    sleep(1);
                    continue;
                } else if(errno != EIDRM) {
                    //. EIDRM: The semaphore set is removed from the system.
                    perror("semop:lock");
                    e = -1;
                    break;
                }
            } else {
                e = 0;
                break;
            }
        }
#endif
    }
    return e;
}

//! Increment and awaken any sleeping processes
//! increment, signal, V, unlock
short Semaphore$unlock(Semaphore *this, unsigned short index, unsigned short persist) {
    /*
    @abstract: Unlock 1 unit of resource in indexed semaphore in semaphore set.

    @discussion: If sem_op is a positive integer, the operation adds this value
    to the semaphore value (semval). Furthermore, if SEM_UNDO is specified for
    this operation, the system updates the process undo count (semadj) for this
    semaphore. This operation can always proceed -- it never forces a process
    to wait. The calling process must have alter permission on the semaphore
    set.

    @returns: This method returns a 0 on success, or otherwise a -1 indicating
    that there is no more resources to unlock, i.e., all resources are already
    unlocked.
    */

    int e = -1;

    if(Semaphore$used(this, index) > 0) {
        struct sembuf op;
        op.sem_num = index;
        op.sem_op = +1;
        op.sem_flg = persist?0:SEM_UNDO;
        if(semop(this->id, &op, (size_t)1)) {
            perror("semop:unlock");
        } else {
            e = 0;
        }
    }

    return e;
}
