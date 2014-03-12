#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#if!defined(__STDC__)
#error("What? No STDC?")
#endif

//#define NDEBUG

#define KILO ((unsigned long)(1 << 10))
#define MEGA ((unsigned long)(1 << 20))
#define GIGA ((unsigned long)(1 << 30))

#define PRIO 19

#define CALLOC_SIZE 32768
#define CALLOC_MAX_COUNT 1048576

#if IMPACT_LO
#define CHK_OPEN_THRESHOLD  10000 //. Opens per second
#define CHK_FORK_THRESHOLD   1000 //. Forks per second
#define CHK_CALLOC_THRESHOLD  200 //. Megabytes per second
#else
#define CHK_OPEN_THRESHOLD   7000
#define CHK_FORK_THRESHOLD    500
#define CHK_CALLOC_THRESHOLD  100
#endif

#define STR_ENOMEM "Kernel memory exhausted"
#define STR_EAGAIN "NPROC resource limit exhausted"
#define STR_EMFILE "Process open files limit exhausted"
#define STR_ENFILE "System open files limit exhausted"
#define STR_THRSHD "System performance impact risk"
#define STR_UNKNWN "Failure due to unknown reason"

#ifndef NDEBUG
#define static_cast(type,var)(assert(sizeof(type)>=sizeof(var)),(type)(var))
#define def_static_cast(type,dst,src)type dst=(assert(sizeof(type)>=sizeof(src)),(type)(src))
#else
#define static_cast(type,var)(type)(var)
#define def_static_cast(type, dst, src)type dst=(type)(src)
#endif

/******************************************************************************/
/*** Limits *******************************************************************/
/******************************************************************************/
#define get_proc_reading(path, fmt, l) {\
    char buf[32];\
    struct stat s;\
    assert(stat(path, &s) == EXIT_SUCCESS);\
    int fh = open(path, O_RDONLY);\
    read(fh, buf, sizeof(buf));\
    sscanf(buf, fmt, &l);\
    close(fh);\
}
#define get_vm_setting(fmt, l) {\
    get_proc_reading("/proc/sys/vm/"#l, fmt, l);\
}
#define get_sys_limit(fmt, l) {\
    get_proc_reading("/proc/sys/kernel/"#l, fmt, l);\
}
#define get_fs_limit(fmt, fn, l) {\
    get_proc_reading("/proc/sys/fs/"fn, fmt, l);\
}


#define setrlim(limit, _syslim) {\
    def_static_cast(unsigned long, syslim, _syslim);\
    int inf = 0;\
    int change = 0;\
    struct rlimit rlp;\
    getrlimit(RLIMIT_##limit, &rlp);\
    Msg$$fprintf(\
        Msg$$INFO,\
        "I "#limit" syslim:%lu usrlim:%lu:%lu\n",\
        syslim, rlp.rlim_cur, rlp.rlim_max\
    );\
    if(rlp.rlim_cur == RLIM_INFINITY) {\
        inf = 1;\
        Msg$$fprintf(Msg$$WARN, "W \\___User limit is unbound!\n");\
    }\
    if(((syslim) > 0) && (rlp.rlim_cur > (syslim))) {\
        Msg$$fprintf(Msg$$WARN, "W \\___ User limit exceeds system limit!\n");\
        change = 1;\
    }\
    if(change) {\
        if(inf) {\
            Msg$$fprintf(\
                Msg$$WARN,\
                "W   \\___ Changing %s limit from INFINITY to",\
                #limit\
            );\
        } else {\
            Msg$$fprintf(\
                Msg$$WARN,\
                "W   \\___ Changing %s limit from %lu:%lu to",\
                #limit,\
                rlp.rlim_cur,\
                rlp.rlim_max\
            );\
        }\
        /* Change */ {\
            if(((system) > 0) && ((syslim) < (rlp.rlim_cur)))\
                rlp.rlim_cur = syslim;\
            if(((system) > 0) && ((syslim) < (rlp.rlim_max)))\
                rlp.rlim_max = syslim;\
            setrlimit(RLIMIT_##limit, &rlp);\
        }\
        getrlimit(RLIMIT_##limit, &rlp);\
        Msg$$fprintf(\
            Msg$$WARN,\
            " %lu:%lu\n",\
            rlp.rlim_cur,\
            rlp.rlim_max\
        );\
    }\
}

float get_la() {
    FILE *fh = NULL;
    fh = fopen("/proc/loadavg", "r");

    char buf[128];
    fread(buf, sizeof(char), 127, fh);
    fclose(fh);

    float la;
    sscanf(buf, "%f", &la);

    return la;
}

char *read_file_lines(const char *path) {
    static char *buffer = NULL;
    static char *saveptr = NULL;
    char *token;

    if(buffer == NULL) {
        struct stat sb;
        if(stat(path, &sb) == EXIT_SUCCESS) {
            int fh = open(path, O_RDONLY);
            if(fh != -1) {
                if(sb.st_size > 0) {
                    buffer = malloc(sb.st_size);
                    if(buffer)
                        read(fh, buffer, sb.st_size);
                    close(fh);
                } else {
                    /*
                    ** /proc entries don't have reliable file sizes, and often
                    ** just set to 0; hence we will not trust a st_size of 0.
                    */
                    buffer = malloc(BUFSIZ);
                    int readed = read(fh, buffer, BUFSIZ);
                    char *bufptr;
                    while(readed == BUFSIZ) {
                        bufptr = realloc(buffer, BUFSIZ);
                        readed = read(fh, bufptr, BUFSIZ);
                    }
                    close(fh);

                }
            }
        }
        token = strtok_r(buffer, "\n", &saveptr);
    } else
        token = strtok_r(NULL, "\n", &saveptr);

    assert(buffer != NULL);
    if(token == NULL) {
        free(buffer);
        buffer = NULL;
        saveptr = NULL;
    }

    return token;
}


/******************************************************************************/
/*** Messaging ****************************************************************/
/******************************************************************************/
#define RESET       0
#define BRIGHT      1
#define DIM         2
#define UNDERLINE   3
#define BLINK       4
#define REVERSE     7
#define HIDDEN      8

#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define MAGENTA     5
#define CYAN        6
#define WHITE       7

#define Msg$$progress(name, count, limit) {\
    printf(\
        "\rX \\___ %10s [%12lu/%-22lu]",\
        name,\
        static_cast(unsigned long, count),\
        static_cast(unsigned long, limit)\
    );\
    fflush(stdout);\
}

#define Msg$$PASS 1
#define Msg$$DBUG 2
#define Msg$$INFO 3
#define Msg$$WARN 4
#define Msg$$ERRR 5
#define Msg$$TITL 6
#define Msg$$FULL (1 << 8)
#define Msg$$fprintf(flags, fmt, ...) {\
    int type = BRIGHT;\
    int fg = 30 + BLACK;\
    int bg = 40 + BLACK;\
    switch(0xFF&flags) {\
        case Msg$$PASS:\
            fg = 30 + GREEN;\
            break;\
        case Msg$$DBUG:\
            bg = 40 + MAGENTA;\
            fg = 30 + WHITE;\
            break;\
        case Msg$$INFO:\
            fg = 30 + WHITE;\
            break;\
        case Msg$$WARN:\
            fg = 30 + YELLOW;\
            break;\
        case Msg$$ERRR:\
            fg = 30 + RED;\
            break;\
        case Msg$$TITL:\
            fg = 30 + WHITE;\
            bg = 40 + BLUE;\
            break;\
    }\
    if(flags&Msg$$FULL) {\
        char buf[82];\
        memset(buf, ' ', 80);\
        snprintf(buf, 70, fmt, ##__VA_ARGS__);\
        buf[strlen(buf)] = ' ';\
        snprintf(buf+70, 11, "LA:%7.2f", get_la());\
        buf[81] = '\0';\
        assert(strlen(buf) == 80);\
        fprintf(\
            stdout,\
            "%c[%d;%d;%dm%s%c[%dm",\
            0x1B, type, fg, bg,\
            buf,\
            0x1B, RESET\
        );\
    } else {\
        fprintf(\
            stdout,\
            "%c[%d;%d;%dm"fmt"%c[%dm",\
            0x1B, type, fg, bg,\
            ##__VA_ARGS__,\
            0x1B, RESET\
        );\
    }\
    fflush(stdout);\
}

#define Msg$$reset_color()\
    fprintf(stdout, "%c[%dm", 0x1B, RESET);

#define Msg$$line(type)\
    Msg$$fprintf(\
        stdout, type, "%s\n",\
        "================================================================================"\
    );\
    fflush(stdout);\


typedef struct _Msg {
    struct _Msg *next;
    char name[16];
    unsigned long limit;
    unsigned long reach;
    const char *reason;
    struct timeb stime;
    struct timeb etime;
} Msg;

Msg *Msg$addMessage(Msg **this, char *name, struct timeb *stime, unsigned long limit, unsigned long reach, const char *reason) {
    Msg *msg = NULL;
    if(*this == NULL) {
        msg = calloc(1, sizeof(Msg));
        *this = msg;
    } else {
        msg = *this;
        while(msg->next != NULL) msg = msg->next;
        msg->next = calloc(1, sizeof(Msg));
        msg = msg->next;
    }
    memcpy(&msg->stime, stime, sizeof stime);
    ftime(&msg->etime);

    strncpy(msg->name, name, sizeof msg->name - 1);
    msg->limit = limit;
    msg->reach = reach;
    msg->reason = reason;

    return msg;
}

void Msg$dump(Msg **this) {
    Msg *msg = *this;
    Msg$$fprintf(
        (Msg$$TITL|Msg$$FULL),
        "%8s %5s %5s %5s%% %5s %s",
        "SECTION", "LIMIT", "REACH", "REACH", "TIME", "REASON"
    );

    while(msg) {
        double stime = (double)msg->stime.time;
        stime += (double)msg->stime.millitm / 1000.f;

        double etime = (double)msg->etime.time;
        etime += (double)msg->etime.millitm / 1000.f;

        float score = 100.f * msg->reach / msg->limit;

        int level;
        if(score > 70.f) level = Msg$$PASS;
        else if(score > 50.f) level = Msg$$WARN;
        else level = Msg$$ERRR;
        Msg$$fprintf(
            level,
            "%8s %5lu %5lu %5.2f%% %5.1f %s\n",
            msg->name,
            msg->reach,
            msg->limit,
            score,
            etime - stime,
            msg->reason
        );
        msg = msg->next;
    }

    msg = *this;
    while(msg) {
        Msg *next = msg->next;
        free(msg);
        msg = next;
    }
    this = NULL;
}

/******************************************************************************/
/*** Forking ******************************************************************/
/******************************************************************************/
#include "SharedMemory.h"
#define SHMEM_KEY IPC_PRIVATE
static pid_t ppid;

static void sig_handler(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0) sleep(10);
}

static unsigned long forked;
static void chk_fork_cleanup(SharedMemory *children, const unsigned long forked_max) {
    assert(children != NULL);

    short cont = 1;
    while(cont) {
        cont = 0;
        int i;
        for(i=0; i<forked_max; i++) {
            pid_t pid = SharedMemory$read_uint(children, i);
            Msg$$progress("Verifying", i+1, forked_max);
            if(pid == 0) {
                cont = 1;
                printf("...Break\n");
                break;
            }
        }
        sleep(1);
    }
    printf("...Done\n");

#if 1
    Msg$$progress("Killing", forked, forked_max);
    pid_t pgid = -getpgrp();
    assert(pgid != -1);
    int e = kill(pgid, SIGCHLD);
    assert(e == EXIT_SUCCESS);
    forked = 0;
#else
    int i;
    for(i=0; i<forked_max; i++) {
        pid_t pid = SharedMemory$read_uint(children, i);
        assert(pid != ppid);
        assert(pid != 0);
        int killed;
        killed = kill(pid, SIGCHLD);
        while(killed != EXIT_SUCCESS) {
            sleep(0.2);
            killed = kill(pid, SIGCHLD);
        }
        Msg$$progress(Msg$$INFO, "Killing", forked, forked_max);
        forked--;
    }
#endif
    printf("...Done\n");
}

static inline int performance(unsigned long counter, unsigned long divider) {
    int performance = -1;

    //. Reset the counter on any first calls to this function
    static time_t t_last = 0;
    if(counter == 0)
        t_last = 0;

    time_t t_now = time(NULL);
    if(t_last > 0) {
        static unsigned long counter_last = 0;
        if((t_now - t_last) > 5) {
            performance = (counter - counter_last)/(t_now - t_last)/divider;
            t_last = t_now;
            counter_last = counter;
        }
    } else
        t_last = time(NULL);

    return performance;
}

//. Threshold here signifies minimum forks expected per second.
unsigned long do_fork(SharedMemory *children, rlim_t lim, int threshold, char **reason) {
    unsigned long forked_max = 0;
    short cont = 1;
    while(cont) {
        pid_t pid = fork();
        switch(pid) {
            case -1: /* ERROR */
                switch(errno) {
                    case EAGAIN:
                        *reason = STR_EAGAIN;
                    break;

                    case ENOMEM:
                        *reason = STR_ENOMEM;
                    break;
                }
                printf("\n");
                forked_max = forked;
                chk_fork_cleanup(children, forked_max);
                return forked_max;
            break;

            case 0: /* CHILD */
                pause();
                _exit(0);
            break;

            default: /* PARENT */
                if(cont && forked < lim) {
                    assert(children != NULL);
                    SharedMemory$write_uint(children, forked, pid);

                    int performed = performance(forked, 1);
                    Msg$$progress("Spawning", forked+1, lim);
                    if(performed == -1) {
                        forked++;
                    } else if(performed >= threshold) {
                        printf("...%i forks/s", performed);
                    } else {
                        printf("...%i forks/s (!> %i)\n", performed, threshold);
                        *reason = STR_THRSHD;
                        forked_max = forked;
                        chk_fork_cleanup(children, forked_max);
                        return forked_max;
                    }
                } else {
                    printf("\n");
                    forked_max = forked;
                    chk_fork_cleanup(children, forked_max);
                    return forked_max;
                }
            break;
        }
    }
    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
int chk_sock(Msg **msg, int threshold) {
     int e = EXIT_SUCCESS;

     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if(sockfd < 0) {
        perror("ERROR opening socket");
     } else {
        int portno = 1234;
        struct sockaddr_in serv_addr;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        e = bind(
            sockfd,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)
        );
        if(e < 0) {
            perror("ERROR on binding");
        } else {
            struct sockaddr_in cli_addr;
            listen(sockfd, threshold);
            socklen_t clilen = sizeof(cli_addr);
            int newsockfd = accept(
                sockfd,
                (struct sockaddr *)&cli_addr,
                &clilen
            );
            if(newsockfd < 0) {
                perror("ERROR on accept");
            } else {
                char buffer[256];
                bzero(buffer,256);
                ssize_t n = read(newsockfd, buffer, 255);
                if(n < 0) {
                    perror("ERROR reading from socket");
                } else {
                    printf("Here is the message: %s\n",buffer);
                    n = write(newsockfd,"I got your message", 18);
                    if(n < 0) {
                        perror("ERROR writing to socket");
                    }
                }
            }
        }
     }
     return e;
}

int chk_fork(Msg **msg, int threshold) {
    char *reason = STR_UNKNWN;

    struct timeb t;
    ftime(&t);

    struct sigaction act;
    act.sa_handler = sig_handler;
    act.sa_flags = SA_NODEFER | SA_NOCLDWAIT;
    sigemptyset(&act.sa_mask);

    int  e;
    e = sigaction(SIGCHLD, &act, NULL);
    if(e == EXIT_SUCCESS) {
        struct rlimit rlp;
        getrlimit(RLIMIT_NPROC, &rlp);

        SharedMemory *children;
        children = SharedMemory$new(SHMEM_KEY, rlp.rlim_cur * sizeof(pid_t), 1);
        if(children == NULL) {
            children = SharedMemory$new(
                SHMEM_KEY,
                rlp.rlim_cur * sizeof(pid_t),
                0
            );
            assert(children != NULL);
        }

        unsigned long forked_max;
        forked_max = do_fork(children, rlp.rlim_cur, threshold, &reason);
        while(forked != 0) sleep(1);

        Msg$addMessage(msg, "PROC", &t, rlp.rlim_cur, forked_max, reason);

        SharedMemory$delete(&children, 1);
    } else {
        perror("sigaction");
        e = EXIT_FAILURE;
    }

    return e;
}

/******************************************************************************/
/*** Callocing ****************************************************************/
/******************************************************************************/
const int CHUNK = 32;
int chk_calloc(Msg **msg, int calloc_size, int threshold) {
    int e = EXIT_SUCCESS;
    char *reason = STR_UNKNWN;

    struct timeb t;
    ftime(&t);
    struct rlimit rlp;
    getrlimit(RLIMIT_AS, &rlp);

    unsigned long i = 0;
    unsigned long opened_max = 0;
    unsigned long limit = rlp.rlim_cur / MEGA;
    char** heaps = calloc(CALLOC_MAX_COUNT, sizeof(void *));
    int cont = 1;
    while(cont) {
        unsigned long eat = static_cast(unsigned long, calloc_size) * i;
        Msg$$progress("Callocing", eat/MEGA, limit);

        //. We only CALLOC *occasionally*, to ensure we can, most times however
        //. we simply malloc as it's much faster and presses less of a load on
        //. the host.
        heaps[i] = (i % 9) ?  malloc(calloc_size) : calloc(1, calloc_size);
        if(heaps[i] != NULL) {
            int performed = performance(eat, MEGA);
            if(performed == -1) {
                i++;
            } else if(performed >= threshold) {
                printf("...%i MiB/s", performed);
                i++;
            } else {
                reason = STR_THRSHD;
                printf("...%i MiB/s (!> %i MiB/s)\n", performed, threshold);
                cont = 0;
            }
        } else {
            switch(errno) {
                case ENOMEM:
                    reason = STR_ENOMEM;
                break;
            }
            printf("\n");
            cont = 0;
        }
    }

    opened_max = i;
    while(i > 0) {
        free(heaps[--i]);
        Msg$$progress("Freeing", calloc_size * i / MEGA, limit);
    }
    free(heaps);
    printf("...Done\n");

    unsigned long reach = calloc_size * opened_max / MEGA;
    Msg$addMessage(msg, "CALLOC", &t, limit, reach, reason);

    return e;
}

/******************************************************************************/
/*** File Opening *************************************************************/
/******************************************************************************/
#define chk_open_cleanup(i, lim) {\
    opened_max = i + 1;\
    while(i--) {\
        Msg$$progress("Closing", i, rlp.rlim_cur);\
        close(opened[i]);\
    }\
    printf("\n");\
    cont = 0;\
}

int chk_open(Msg **msg, int threshold) {
    struct timeb t;
    ftime(&t);
    int e = EXIT_FAILURE;

    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    int opened[rlp.rlim_cur];
    memset(opened, 0, sizeof(int));

    char *reason = STR_UNKNWN;
    unsigned long opened_max = 0;
    unsigned long i = 0;
    int cont = 1;
    char tmp[] = "/tmp/syscpy";
    while(cont) {
        int fh = open(tmp, O_RDWR|O_CREAT|O_TRUNC, 0600);
        if(fh != -1) {
            Msg$$progress("Opening", i, rlp.rlim_cur);
            int performed = performance(i, 1);
            if(performed == -1) {
                opened[i] = fh;
                i++;
                unlink(tmp);
            } else if(performed >= threshold) {
                printf("...%i opens/s", performed);
            } else {
                printf("...%i opens/s (!> %i)\n", performed, threshold);
                reason = STR_THRSHD;
                chk_open_cleanup(i, rlp.rlim_cur);
            }
        } else {
            switch(errno) {
                case ENFILE:
                    reason = STR_ENFILE;
                break;

                case EMFILE:
                    reason = STR_EMFILE;
                break;

                case ENOMEM:
                    reason = STR_ENOMEM;
                break;
            }
            printf("\n");
            chk_open_cleanup(i, rlp.rlim_cur);
        }
    }
    Msg$addMessage(msg, "OPEN", &t, rlp.rlim_cur, opened_max, reason);

    return e;
}

int chk_calloc_prepare() {
    int e = EXIT_FAILURE;

    int overcommit_memory;
    get_vm_setting("%i", overcommit_memory);
    if(overcommit_memory == 2) {
        Msg$$fprintf(Msg$$INFO, "I Memory overcommit protection is ON; Good.\n");

        char *meminfo;
        char *bufptr;
        unsigned long long mem_total = -1;
        unsigned long long swap_total = -1;
        while((meminfo = read_file_lines("/proc/meminfo")) != NULL)
            if((bufptr = strstr(meminfo, "MemTotal:")) != NULL) {
                sscanf(meminfo + 9, "%llu", &mem_total);
                mem_total *= KILO;
            } else if((bufptr = strstr(meminfo, "SwapTotal:")) != NULL) {
                sscanf(meminfo + 10, "%llu", &swap_total);
                swap_total *= KILO;
            }
        Msg$$fprintf(Msg$$INFO, "I MemTotal         : %llu\n", mem_total);
        Msg$$fprintf(Msg$$INFO, "I SwapTotal        : %llu\n", swap_total);

        int swappiness;
        get_vm_setting("%i", swappiness);
        Msg$$fprintf(Msg$$INFO, "I Swappiness:      : %i%%\n", swappiness);

        unsigned long long memsyslim_ll = swap_total;
        int overcommit_ratio = 0;
        get_vm_setting("%i", overcommit_ratio);
        Msg$$fprintf(Msg$$INFO, "I Overcommit ratio : %i%%\n", overcommit_ratio);

        memsyslim_ll += (mem_total * overcommit_ratio) / 100;
        unsigned long memsyslim = (unsigned long)memsyslim_ll;
        assert((unsigned long long)memsyslim == memsyslim_ll);
        setrlim(AS, memsyslim);
        Msg$$fprintf(Msg$$INFO, "I Overcommit limit : %lu\n", memsyslim);

        e = EXIT_SUCCESS;
    } else {
        //. Too dangerous, OOM killer will start killing off processes, and not just
        //. *our* processes!
        Msg$$fprintf(Msg$$ERRR, "E Memory overcommit protection is OFF!\n");
        Msg$$fprintf(Msg$$ERRR, "E \\___ Cowardly refusing to run CALLOC test.\n");
        Msg$$fprintf(Msg$$INFO, "I \\___ To run this test, run: sysctl vm.overcommit_memory=2\n");
        e = EXIT_FAILURE;
    }

    return e;
}

int chk_fork_prepare() {
    int pid_max = -1;
    get_sys_limit("%i", pid_max);
    setrlim(NPROC, static_cast(unsigned long, pid_max));

    //. The thumb rule is pending signals should be equal to max user processes.
    //. By default pending signals is 1024.
    setrlim(SIGPENDING, static_cast(unsigned long, pid_max));

    return EXIT_SUCCESS;
}

int chk_open_prepare() {
    int prio = getpriority(PRIO_PROCESS, ppid);
    Msg$$fprintf(Msg$$INFO, "I Setting process priority from %i to %i...", prio, PRIO);
    int e = setpriority(PRIO_PROCESS, ppid, PRIO);
    assert(e == EXIT_SUCCESS);
    prio = getpriority(PRIO_PROCESS, ppid);
    if(prio == PRIO)
        printf("Done\n");
    else
        printf("Failed\n");

    //. chk_open
    int fh_max = -1;
    get_fs_limit("%i", "file-max", fh_max);
    setrlim(NOFILE, fh_max);

    return e;
}

/******************************************************************************/
/*** Main *********************************************************************/
/******************************************************************************/
int main() {
    forked = 0;
    ppid = getpid();
    Msg *msg = NULL;

#ifndef NDEBUG
    Msg$$fprintf((Msg$$DBUG|Msg$$FULL), "D Running in debug mode.");
#endif

    int e = EXIT_FAILURE;

    //. chk_open
    Msg$$fprintf((Msg$$TITL|Msg$$FULL), "%s", "OPEN");
    if((e = chk_open_prepare()) == EXIT_SUCCESS)
        e = chk_open(&msg, CHK_OPEN_THRESHOLD);

    //. chk_fork
    Msg$$fprintf((Msg$$TITL|Msg$$FULL), "%s", "FORK");
    if((e = chk_fork_prepare()) == EXIT_SUCCESS)
        e = chk_fork(&msg, CHK_FORK_THRESHOLD);

    //. chk_calloc
    Msg$$fprintf((Msg$$TITL|Msg$$FULL), "%s", "ALOC");
    if((e = chk_calloc_prepare()) == EXIT_SUCCESS)
        e = chk_calloc(&msg, CALLOC_SIZE, CHK_CALLOC_THRESHOLD);

    Msg$dump(&msg);

    exit(e);
}
