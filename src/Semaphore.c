#include "Semaphore.h"

#define SFMT (\
    (sizeof(key_t) == sizeof(int)) \
    ? \
        "Id:           %i\n" \
        "Key:          %i\n" \
        "Semaphore:    Semaphore %i of %i\n"\
        "Value:        %i\n" \
        "Last Changed: %s" \
    : \
        "Id:           %i\n" \
        "Key:          %li\n" \
        "Semaphore:    Semaphore %i of %i\n"\
        "Value:        %i\n" \
        "Last Changed: %s" \
)

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
int Semaphore$init(Semaphore *this, short initial);

/* Public Methods */
Semaphore* Semaphore$new(key_t key, short size, short initial, bool attach) {
    /*
    @abstract: Create a new Semaphore object

    @discussion: The new object will contain a semaphore set of size `size'.
    If the object of id `key' already exists and if the user has explicitly
    requested - then this method will attempt to reattach to that shsem instead
    of failing.

    @return: On success, this method returns a pointer to the new object,
    otherwise it will return NULL.
    */

    Semaphore *this = (Semaphore *)malloc(sizeof(Semaphore));
    if(this == NULL) return NULL;

    int success = -1;
    this->size = -1;
    this->key = key;
    key_t k = SEMKEY(key);
    assert(k != -1);

    this->id = semget(k, size, 0666|IPC_CREAT|IPC_EXCL);
    if(this->id != -1) { //. New semaphores...
        //. Initialize the semaphores initial values...
        this->size = size;
        success = Semaphore$init(this, initial);
    } else if(attach) { //. Existing semaphore, attach if user has allowed...
        if(errno == EEXIST) { //. Reattach...
            this->id = semget(k, NOP, 0666|IPC_EXCL);
            if(this->id != -1) success = EXIT_SUCCESS;
            else log_errr("semget@reattach");
        } else log_errr("semget@create");
    } else {
        err_atomicles |= FLAG_SHSEM|FLAG_EXISTS;
        log_errr("semget@new");
    }

    if(success != EXIT_SUCCESS)
        Semaphore$delete(&this, false);

    return this;
}

void Semaphore$delete(Semaphore **this, bool remove_sem_too) {
    /*
    @abstract: Destructor method.

    @discussion: This method takes a second argument.  If this argument is set
    to a non-zero value, other than the semaphore object itself which has
    been mallocated, the shsem segment in kernel memory will too be removed.

    Either way, the malloc will be freed though.
    */
    if(remove_sem_too)
        if(semctl((*this)->id, NOP, IPC_RMID))
            log_errr("semctl@delete");

    free(*this);
    *this = NULL;
}

int Semaphore_exists(key_t key) {
    key_t k = SEMKEY(key);
    int id = semget(k, NOP, 0666|IPC_EXCL);

    int exists = (id != -1);
    if(!exists)
        log_errr("semget@exists");

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
        this->size = Semaphore$size(this);
    } else {
        err_atomicles |= FLAG_SHSEM|FLAG_MISSING;
        log_errr("semget@attach");
    }

    return this;
}

int Semaphore$current(Semaphore *this, unsigned short index) {
    /*
    @abstract: Returns the current value of the semaphore.

    @discussion: Returns the current value of the semaphore for the semaphore
    at the given index in the semaphore set.
    */
    return semctl(this->id, index, GETVAL);
}

time_t Semaphore$ctime(Semaphore *this) {
    struct semid_ds t_semid_ds;
    semctl(this->id, NOP, IPC_STAT, &t_semid_ds);
    return t_semid_ds.sem_ctime;
}

void Semaphore$desc(Semaphore *this, unsigned short index) {
    /*
    @abstract: Describe the state of the semaphore and dump to stdout.
    */
    struct semid_ds t_semid_ds;
    semctl(this->id, NOP, IPC_STAT, &t_semid_ds);

    fprintf(
        stdout, SFMT,
        this->id, this->key,
        index, this->size,
        Semaphore$current(this, index),
        ctime(&(t_semid_ds.sem_ctime))
    );
}

int Semaphore$size(Semaphore *this) {
    /*
    @abstract: Simple accessor method, returns the semaphore set size.

    @discussion:  Where the user has reattached to an existing semaphore
    object, this method is required to figure out what the set-size of the
    semaphore-set was at the time of its creation (via the Semaphore$new()
    method).

    A size of 1 indicates that this is a binary semaphore, otherwise known as a
    mutex.  A size greater than 1 declares this a counting semaphore.
    */
    if(this->size == -1) {
        assert(this->id != -1);
        struct semid_ds t_semid_ds;
        semctl(this->id, NOP, IPC_STAT, &t_semid_ds);
        this->size = t_semid_ds.sem_nsems;
    }

    return this->size;
}

int Semaphore$init(Semaphore *this, short initial) {
    /*
    @abstract: Sets the initial values of all semaphore in the semaphore set.
    */
    int success = EXIT_FAILURE;

    short sarray[this->size];
    int i;
    for(i=0; i<this->size; i++)
        sarray[i] = initial;

    if(semctl(this->id, NOP, SETALL, sarray))
        log_errr("semctl@init");
    else success = EXIT_SUCCESS;

    return success;
}

//! Decrement and block if the result if negative
//! Monikers: decrement, wait, P (proberen (to test)), and lock
int Semaphore$lock(Semaphore *this, unsigned short index, bool persist, time_t timeout) {
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

    struct sembuf op;
    op.sem_num = index;
    op.sem_op = -1;
    op.sem_flg = persist?0:SEM_UNDO;

    if(timeout == LOCK_BLOCKING) {
        //. Block...
        if(semop(this->id, &op, (size_t)1)) {
            if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                log_errr("semop@lock");
                err_atomicles |= FLAG_SHSEM|FLAG_UNKNOWN;
            }
        }
    } else if(timeout == LOCK_NONBLOCK) {
        //. Instantaeneos (Non-Blocking)...
        op.sem_flg |= IPC_NOWAIT;
        if(semop(this->id, &op, (size_t)1)) {
            if(errno == EAGAIN) {
                //. Timed out, still can't lock...
                err_atomicles |= FLAG_SHSEM|FLAG_UNAVAIL;
            } else if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                log_errr("semop@lock");
                err_atomicles |= FLAG_SHSEM|FLAG_UNKNOWN;
            }
        }
    } else {
        //. Blocking for at most `timeout' seconds...

#ifdef _GNU_SOURCE
        struct timespec t;
        t.tv_sec = timeout;
        t.tv_nsec = 0;
        if(semtimedop(this->id, &op, (size_t)1, &t)) {
            if(errno == EAGAIN) {
                //. Timed out, still can't lock...
                err_atomicles |= FLAG_SHSEM|FLAG_TIMEOUT;
                dbg_warn("timed out");
            } else if(errno != EIDRM) {
                //. EIDRM: The semaphore set is removed from the system.
                log_warn("semtimedop@lock");
                err_atomicles |= FLAG_SHSEM|FLAG_UNKNOWN;
            }
        }
#else
        time_t t;
        op.sem_flg |= IPC_NOWAIT;
        err_atomicles |= FLAG_SHSEM|FLAG_UNKNOWN;
        for(t=0; t<timeout; t++) {
            if(semop(this->id, &op, (size_t)1)) {
                if(errno == EAGAIN) {
                    sleep(1);
                    continue;
                } else if(errno != EIDRM) {
                    //. EIDRM: The semaphore set is removed from the system.
                    err_atomicles |= FLAG_SHSEM|FLAG_UNKNOWN;
                    log_errr("semop@lock");
                    break;
                }
            } else {
                //. Success
                break;
                err_atomicles = 0;
            }
        }
#endif
    }

    return err_atomicles;
}

//! Increment and awaken any sleeping processes
//! Monikers: increment, signal, V (verhogen (to increment)), unlock
int Semaphore$unlock(Semaphore *this, unsigned short index, bool persist) {
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

    struct sembuf op;
    op.sem_num = index;
    op.sem_op = +1;
    op.sem_flg = persist?0:SEM_UNDO;
    if(semop(this->id, &op, (size_t)1))
        log_errr("semop@unlock");
    else
        e = 0;

    return e;
}
