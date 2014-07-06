#include "lock.h"

static signed long delta;
static bool expiry;

int unit(void);
int init(Semaphore *shsem);

int main(int argc, char *argv[]) {
    int e = 0;

    if(argc > 2) {
        static char shortopts[] = "s:t:vx:";

#ifdef GETOPT_LONG
        static struct option longopts[] = {
            { "semaphores", required_argument,  NULL,     's' },
            { "timeout",    required_argument,  NULL,     't' },
            { "expire",     required_argument,  NULL,     'x' },
            { NULL,         0,                  NULL,     0 }
        };
#endif
        extern char *optarg;
        extern int optopt;
        extern int opterr;
        //extern int optreset;

        short timeout = 0;
        unsigned short semaphores = SHSEM_SIZE; //. Mutex by default
        time_t expire = 0;                      //. In seconds
        delta = 0;
        expiry = false;

        char o;
#ifdef GETOPT_LONG
        while((o = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
#else
        while((o = getopt(argc, argv, shortopts)) != (char)-1) {
#endif
            switch(o) {
                case 's':
                    semaphores = atoi(optarg);
                    break;
                case 't':
                    timeout = atoi(optarg);
                    break;
                case 'x':
                    expire = atoi(optarg);
                    expiry = true;
                    break;
                case '?':
                default:
                    e=1;
                    break;
            }
        }

        if(e == 0) {
            e = -9;

            extern int optind;
            argc -= optind;
            argv += optind;

            unsigned int key = atoi(argv[0]);
            const char* action = argv[1];

            if(delta < 0) {
                //. FIXME
                unlock(key);
            }

            if(strncmp(action, "on", 3) == 0) {
                e = lock(key, timeout);
            } else if(strncmp(action, "off", 4) == 0) {
                e = unlock(key);
            } else if(strncmp(action, "summary", 8) == 0) {
                e = summary(key);
            } else if(strncmp(action, "status", 7) == 0) {
                e = status(key);
            } else if(strncmp(action, "delete", 7) == 0) {
                e = delete(key);
            } else if(strncmp(action, "create", 7) == 0) {
                short initial = 0;
                if(argc == 3) initial = atoi(argv[2]);
                e = create(key, semaphores, initial, expire);
            } else if(strncmp(action, "help", 5) == 0) {
                usage();
                e = 0;
            } else {
                usage();
                e = -1;
            }
        } else {
            usage();
            e = -2;
        }
    } else if(argc == 2 && strncmp(argv[1], "unit", 5) == 0) {
        e = unit();
    } else {
        usage();
        e = 0;
    }

    return e;
}

int init(Semaphore *shsem) {
    int e = -1;

    SharedMemory *shmem = SharedMemory$attach(shsem->key);

    if(shmem == NULL) {
        log_warn("SharedMemory %d does not exist.\n", shsem->key);
    } else if(shmem->shsem == NULL) {
        log_warn("SharedMemory %d does have a valid Sempahore.\n", shsem->key);
    } else {
        /* LEGACY TD - should be deleted:
        size_t size = SharedMemory$read_uint(shmem, 0);
        Semaphore$set_size(shsem, size);
        */

        expiry = SharedMemory$read_uint(shmem, 1);
        if(expiry) {
            time_t now = time(NULL);
            time_t expire = Semaphore$ctime(shsem) + SharedMemory$read_uint(shmem, 2);
            delta = expire - now;
        }

        SharedMemory$delete(&shmem, 0);
        e = 0;
    }

    return e;
}

int create(unsigned int key, unsigned int semaphores, short initial, time_t expire) {
    Semaphore *shsem = Semaphore$new(key, SHSEM_COUNT, initial, false);
    if(shsem != NULL) {
        SharedMemory *shmem = SharedMemory$new(
            key,
            (size_t)(4 * sizeof(unsigned int)),
            false
        );

        if(shmem != NULL) {
            //. SUCCESS
            SharedMemory$write_uint(shmem, 0, semaphores);
            SharedMemory$write_uint(shmem, 1, expire?true:false);
            SharedMemory$write_bool(shmem, 2, expire);
            SharedMemory$write_uint(shmem, 3, 0);
            SharedMemory$delete(&shmem, false);
        } else {
            log_warn("A shmem segment with key %i already exists.\n", key);
        }

        Semaphore$delete(&shsem, false);
    } else {
        log_warn("A shsem with key %i already exists.\n", key);
    }

    return err_atomicles;
}

int delete(unsigned int key) {
    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        //. SUCCESS 1 of 2
        Semaphore$delete(&shsem, 1);
    } else {
        log_warn("No shsem with key %d.\n", key);
    }

    SharedMemory *shmem = SharedMemory$attach(key);
    if(shmem != NULL) {
        //. SUCCESS 2 of 2
        SharedMemory$delete(&shmem, 1);
    } else {
        log_warn("No shmem with key %d.\n", key);
    }

    return err_atomicles;
}

int status(unsigned int key) {
    int e = -1;
    int d = -1;
    int u = -1;
    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        if(expiry) {
            if(delta > 0) {
                d = delta;
                e = 0;
            }
        } else e = 0;

        if(e == 0)
            u = Semaphore$current(shsem, SHSEM_INDEX);
    }
    printf("%d %d\n", u, d);
    return e;
}

int summary(unsigned int key) {
    int e = -1;
    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        Semaphore$desc(shsem, SHSEM_INDEX);
        if(expiry) {
            if(delta > 0)
                printf("Expires In:   %lis\n", delta);
            else
                printf("Semaphore has expired.\n");
        } else
            printf("Never expires.\n");
        e = 0;
        Semaphore$delete(&shsem, 0);
    } else log_warn("No semaphore set in memory with key %u\n", key);

    return e;
}

int lock(unsigned int key, int timeout) {
    int e = -1;
    if(Semaphore_exists(key)) {
        Semaphore *shsem = Semaphore$attach(key);
        if(shsem != NULL) {
            init(shsem);
            if(expiry) {
                if(delta > 0) {
                    int t = timeout==0 ? delta : min(delta, timeout);
                    e = Semaphore$lock(shsem, SHSEM_INDEX, 1, t);
                } else delete(key);
            } else e = Semaphore$lock(shsem, SHSEM_INDEX, 1, timeout);
            if(e == -1) log_warn("No more semaphores to lock.\n");
            Semaphore$delete(&shsem, 0);
        } else log_warn("Semaphores %d does exist, but could not be attached to.\n", key);
    } else log_warn("Semaphores %d does not exist.\n", key);
    return e;
}

int unlock(unsigned int key) {
    int e = -1;
    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        e = Semaphore$unlock(shsem, SHSEM_INDEX, 1);
        if(e == -1) log_warn("No more semaphores to unlock.\n");
        Semaphore$delete(&shsem, 0);
    } else log_warn("Semaphores %d does not exist.\n", key);

    return e;
}

int unit() {
    signed int key;
    unsigned int semaphores;
    short initial;
    time_t expire;

    key = 15;
    semaphores = 1;
    initial = 0;
    expire = 0;

    printf("Test 1\n");
    if(create(key, semaphores, initial, expire) != 0)
        log_err("create failed");
    err_atomicles = 0;

    printf("Test 2\n");
    if(create(key, semaphores, initial, expire) != 10)
        log_err("recreate didn't fail");
    err_atomicles = 0;

    printf("Test 3\n");
    if(delete(key) != 0)
        log_err("delete failed");
    err_atomicles = 0;

    if(delete(key) != 9)
        log_err("redelete didn't fail");
    err_atomicles = 0;

    /*
    check(test_check("ex20.c") == 0, "failed with ex20.c");
    check(test_check(argv[1]) == -1, "failed with argv");
    check(test_sentinel(1) == 0, "test_sentinel failed.");
    check(test_sentinel(100) == -1, "test_sentinel failed.");
    check(test_check_mem() == -1, "test_check_mem failed.");
    check(test_check_debug() == -1, "test_check_debug failed.");
    */

    return 0;
}

void usage() {
    printf("Usage:\n");
    printf("  lock <key> <action>\n");
    printf("    NOTE: <key> must be in the range 0..127\n");
    printf("\n");
    printf("  lock <key> create [-x|expire] <ttl> [-s|--semaphores <nsems>] <initial>\n");
    printf("    <semaphore-size> values:\n");
    printf("       1: Binary Semaphore (Mutext)\n");
    printf("      >1: Counting Semaphore\n");
    printf("\n");
    printf("  lock <key> on [-t|--timeout <timeout>]\n");
    printf("    <timeout> values:\n");
    printf("      -1: Non-Blocking (Immediate return)\n");
    printf("       0: Blocking (Default)\n");
    printf("      +N: Block for at most `N' seconds\n");
    printf("\n");
    printf("  lock <key> off\n");
    printf("\n");
    printf("  lock <key> delete\n");
    printf("\n");
    printf("  lock <key> status\n");
    printf("\n");
    printf("  lock <key> summary\n");
}
