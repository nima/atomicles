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
    err_atomicles = 0;

    SharedMemory *shmem = SharedMemory$attach(shsem->key);

    if(shmem == NULL) {
        log_warn("SharedMemory %d does not exist", shsem->key);
    } else if(shmem->shsem == NULL) {
        log_warn("SharedMemory %d does have a valid Sempahore", shsem->key);
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

        SharedMemory$delete(&shmem, false);
    }

    return err_atomicles;
}

int create(unsigned int key, unsigned int semaphores, short initial, time_t expire) {
    err_atomicles = 0;

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
            log_warn("A shmem segment with key %i already exists", key);
        }

        Semaphore$delete(&shsem, false);
    } else {
        log_warn("A shsem with key %i already exists", key);
    }

    return err_atomicles;
}

int delete(unsigned int key) {
    err_atomicles = 0;

    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        //. SUCCESS 1 of 2
        Semaphore$delete(&shsem, 1);
    } else {
        log_warn("No shsem with key %d", key);
    }

    SharedMemory *shmem = SharedMemory$attach(key);
    if(shmem != NULL) {
        //. SUCCESS 2 of 2
        SharedMemory$delete(&shmem, 1);
    } else {
        log_warn("No shmem with key %d", key);
    }

    return err_atomicles;
}

int status(unsigned int key) {
    err_atomicles = 0;

    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        int u;
        if(expiry && delta > 0) {
            u = Semaphore$current(shsem, SHSEM_INDEX);
            printf("%d %li\n", u, delta);
        } else if(!expiry) {
            u = Semaphore$current(shsem, SHSEM_INDEX);
            printf("%d -\n", u);
        }
    }

    return err_atomicles;
}

int summary(unsigned int key) {
    err_atomicles = 0;

    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        Semaphore$desc(shsem, SHSEM_INDEX);
        if(expiry) {
            if(delta > 0)
                printf("Expires In:   %lis\n", delta);
            else
                printf("Semaphore has expired\n");
        } else
            printf("Never expires\n");
        Semaphore$delete(&shsem, false);
    } else log_warn("No semaphore set in memory with key %u\n", key);

    return err_atomicles;
}

int lock(unsigned int key, int timeout) {
    err_atomicles = 0;

    if(Semaphore_exists(key)) {
        Semaphore *shsem = Semaphore$attach(key);
        if(shsem != NULL) {
            init(shsem);
            if(expiry) {
                if(delta > 0) {
                    int t = timeout==0 ? delta : min(delta, timeout);
                    Semaphore$lock(shsem, SHSEM_INDEX, 1, t);
                } else delete(key);
            } else Semaphore$lock(shsem, SHSEM_INDEX, 1, timeout);
            Semaphore$delete(&shsem, false);
        } else log_warn("Semaphores %d does exist, but could not be attached to", key);
    } else {
        err_atomicles |= FLAG_SHSEM|FLAG_MISSING;
        log_errr("Semaphores %d does not exist", key);
    }

    return err_atomicles;
}

int unlock(unsigned int key) {
    err_atomicles = 0;

    Semaphore *shsem = Semaphore$attach(key);
    if(shsem != NULL) {
        init(shsem);
        Semaphore$unlock(shsem, SHSEM_INDEX, 1);
        Semaphore$delete(&shsem, false);
    } else {
        err_atomicles |= FLAG_SHSEM|FLAG_MISSING;
        log_errr("Semaphores %d does not exist", key);
    }

    return err_atomicles;
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

    log_info("Test create"); {
        if(create(key, semaphores, initial, expire) != 0)
            log_errr("create failed");
    }

    log_info("Test recreate"); {
        if(create(key, semaphores, initial, expire) != (FLAG_SHSEM|FLAG_EXISTS))
            log_errr("recreate didn't fail");
    }

    log_info("Test unlock"); {
        if(unlock(key) != 0)
            log_errr(
                "unlock failed with code %d",
                err_atomicles
            );
    }

    log_info("Test lock"); {
        if(lock(key, LOCK_BLOCKING) != 0)
            log_errr(
                "lock failed with code %d",
                err_atomicles
            );
    }


    log_info("Test lock non-block/unavail"); {
        if(lock(key, LOCK_NONBLOCK) != (FLAG_SHSEM|FLAG_UNAVAIL))
            log_errr(
                "lock nonblock code %d did not match expected %d",
                err_atomicles,
                FLAG_SHSEM|FLAG_UNAVAIL
            );
    }

    log_info("Test lock timeout"); {
        if(lock(key, 1) != (FLAG_SHSEM|FLAG_TIMEOUT))
            log_errr(
                "lock timeout code %d did not match expected %d",
                err_atomicles,
                FLAG_SHSEM|FLAG_TIMEOUT
            );
    }

    log_info("Test delete"); {
        if(delete(key) != 0)
            log_errr("delete failed");
    }

    log_info("Test redelete"); {
        if(delete(key) != (FLAG_SHSEM|FLAG_MISSING))
            log_errr("redelete didn't fail");
    }

    log_info("Test lock pre-creation"); {
        if(lock(key, LOCK_BLOCKING) != (FLAG_SHSEM|FLAG_MISSING))
            log_errr(
                "lock failed with code %d",
                err_atomicles
            );
    }

    log_info("Test unlock"); {
        if(unlock(key) != (FLAG_SHSEM|FLAG_MISSING))
            log_errr(
                "unlock failed with code %d",
                err_atomicles
            );
    }

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
