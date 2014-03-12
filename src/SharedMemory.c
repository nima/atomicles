#include "SharedMemory.h"

#define NOP 0

//. /usr/include/linux/shm.h
#define SHMMAX 0x2000000		 /* max shared seg size (bytes) */
#define SHMMIN 1	     		 /* min shared seg size (bytes) */

/*
 * any addition, subtraction or dereferencing of a void pointer is undefined
 * because the size of void is undefined. dereferencing a void pointer would
 * imply an evaluation to a void object. addition to a void pointer would
 * imply incrementations of n*sizeof(void) (because n + 1 should point to the
 * next void object).
 */
SharedMemory* SharedMemory$new(key_t key, size_t size, unsigned short attach) {
    int success = EXIT_FAILURE;

    //. Shared Memory...
    SharedMemory *this = (SharedMemory *)malloc(sizeof(SharedMemory));
    if(this == NULL)
        return this;

    this->addr = NULL;
    this->shsem = NULL;
    this->key = key;
    key_t k = SHMKEY(key);
    this->size = size;
    this->id = shmget(k, this->size, 0666|IPC_CREAT|IPC_EXCL);
    if(this->id != -1) {
        this->addr = shmat(this->id, NULL, 0);
        if(this->addr != (int *)-1)
            success = EXIT_SUCCESS;
    } else {
        success = errno;
        switch(success) {
            case EACCES:
                /*
                 * The user does not have permission to access the shared memory
                 * segment, and does not have the CAP_IPC_OWNER capability.
                 */
                perror("EACCES");
            break;

            case EEXIST:
                /*
                 * IPC_CREAT | IPC_EXCL was specified and the segment exists.
                 */
                if(attach) {
                    this->id = shmget(k, NOP, 0666|IPC_EXCL);
                    if(this->id != -1) {
                        this->addr = shmat(this->id, NULL, 0);
                        if(this->addr != (int *)-1)
                            success = EXIT_SUCCESS;
                        else
                            perror("Failed to reattach");
                    } else
                        success = errno;
                } else perror("EEXIST");
            break;

            case EINVAL:
                /*
                 * A  new  segment was to be created and size < SHMMIN or
                 * size > SHMMAX, or no new segment was to be created, a segment
                 * with given key existed, but size is greater than the size of
                 * that segment.
                 */
                fprintf(stderr, "(%i < %lu < %u)?\n", SHMMIN, this->size, SHMMAX);
                perror("EINVAL");
            break;

            case ENFILE:
                /*
                 * The system limit on the total number of open files has been reached.
                 */
                perror("ENFILE");
            break;

            case ENOENT:
                /*
                 * No segment exists for the given key, and IPC_CREAT was not specified.
                 */
                perror("ENFILE");
            break;

            case ENOMEM:
                /*
                 * No memory could be allocated for segment overhead.
                 */
                perror("ENOMEM");
            break;

            case ENOSPC:
                /*
                 * All possible shared memory IDs have been taken (SHMMNI), or
                 * allocating a segment of the requested size would cause the
                 * system to exceed  the  systemwide limit on shared memory
                 * (SHMALL).
                 */
                perror("ENOSPC");
            break;

            case EPERM:
                /*
                * The SHM_HUGETLB flag was specified, but the caller was not
                * privileged (did not have the CAP_IPC_LOCK capability).
                */
                perror("EPERM");
            break;
        }
    }

    if(success == EXIT_SUCCESS) {
        success = EXIT_FAILURE;
        //fprintf(stderr, "id:%i, addr:%08x\n", this->id, (unsigned int)this->addr);

        //. Create a semaphore set of size 1, with counter set to 1
        this->shsem = Semaphore$new(key + 128, 1, 1, attach);

        if(this->shsem != NULL) {
            success = mlockall(MCL_FUTURE);
            if(success != EXIT_SUCCESS) {
                perror("mlockall");
                success=(mlock(this->addr, this->size) != EXIT_SUCCESS);
                if(success != EXIT_SUCCESS)
                    perror("mlock");
            }
        }
        /*
        if(mlock(this->addr, this->size) != EXIT_SUCCESS)
            perror("mlock");
        */
    }

    if(success != EXIT_SUCCESS)
        SharedMemory$delete(&this, 0);
    else
        assert(this->shsem != NULL);

    return this;
}

void SharedMemory$delete(SharedMemory **this, short remove_shm_too) {
    if(remove_shm_too) {
        if((*this)->shsem != NULL)
            Semaphore$delete(&((*this)->shsem), 1);

        memset((*this)->addr, 0, (*this)->size);

        if(munlockall() != EXIT_SUCCESS)
            perror("shm:delete:munlockall");
        /*
        if(munlock((*this)->addr, (*this)->size))
            perror("shm:delete:munlock");
        */

        if(shmdt((*this)->addr) != 0)
            perror("shm:delete:shmdt");

        if(shmctl((*this)->id, IPC_RMID, 0))
            perror("shm:delete:shmctl");
    } else {
        if((*this)->shsem != NULL)
            Semaphore$delete(&((*this)->shsem), 0);
    }

    free(*this);
    *this = NULL;
}

SharedMemory* SharedMemory$attach(key_t key) {
    SharedMemory *this = NULL;
    key_t k = SHMKEY(key);
    int id = shmget(k, NOP, 0666|IPC_EXCL);
    if(id != -1) {
        this = (SharedMemory *)malloc(sizeof(SharedMemory));
        this->key = key;
        this->id = id;
        this->addr = shmat(this->id, NULL, 0);
        if(this->addr != (int *)-1) {
            this->shsem = Semaphore$attach(key + 128);
        } else perror("shm:attach:shmat");
    } // else perror("shm:attach:shmget");
    return this;
}

size_t SharedMemory$get_size(SharedMemory* this) {
    return this->size;
}

void SharedMemory$dump(SharedMemory *this) {
    printf("key: %d\n", this->key);
    printf("id: %d\n", this->id);
    printf("size: %zu\n", this->size);
    printf("addr: %p\n", this->addr);
    printf("shsem: %p\n", this->shsem);
}

void SharedMemory$write(SharedMemory* this, unsigned short offset, const char *fmt, ...) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    va_list ap;
    va_start(ap, fmt);
    char *chr_ptr = this->addr;
    vsnprintf(&(chr_ptr[offset]), this->size - offset, fmt, ap);
    va_end(ap);
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);
}
void SharedMemory$read(SharedMemory* this, unsigned short offset, char *buffer) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    char *chr_ptr = this->addr;
    strncpy(buffer, &(chr_ptr[offset]), this->size - offset);
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);
}


void SharedMemory$write_uint(SharedMemory* this, unsigned short offset, unsigned int n) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    unsigned int *shmem = this->addr;
    shmem[offset] = n;
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);
}
unsigned int SharedMemory$read_uint(SharedMemory* this, unsigned short offset) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    unsigned int *shmem = this->addr;
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);

    return shmem[offset];
}

void SharedMemory$write_uint32(SharedMemory* this, unsigned short offset, uint32_t n) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    unsigned int *shmem = this->addr;
    shmem[offset] = n;
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);
}
uint32_t SharedMemory$read_uint32(SharedMemory* this, unsigned short offset) {
    short e = Semaphore$lock(this->shsem, 0, 0, 0);
    assert(e == 0);

    uint32_t *shmem = this->addr;
    e = Semaphore$unlock(this->shsem, 0, 0);
    assert(e == 0);

    return shmem[offset];
}

