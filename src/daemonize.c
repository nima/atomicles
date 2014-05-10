#include "daemonize.h"

#define SFMT (sizeof(key_t) == sizeof(int)) \
    ? "%8i %c:%-8d %c:%-8d %d" \
    : "%8li %c:%-8d %c:%-8d %d"

static void handler(int sig, siginfo_t* siginfo, void* ucontext) {
    syslog(LOG_ERR, "Got signal %d at address: 0x%lx\n", sig, (long)siginfo->si_addr);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sa.sa_flags = SA_NODEFER | SA_RESETHAND;
    sigaction(sig, &sa, NULL);

    longjmp(g_state_buf, sig);
}

static void cleanup(SharedMemory *shmem, const char *msg) {
    syslog(LOG_CRIT, "%s, %s", msg, strerror(errno));
    closelog();
    set_pid(shmem, 0);
    set_cpid(shmem, 0);
    SharedMemory$delete(&shmem, 1);
}

static void die(SharedMemory *shmem, const char *msg) {
    syslog(LOG_CRIT, "%s, %s", msg, strerror(errno));
    closelog();
    set_pid(shmem, 0);
    set_cpid(shmem, 0);
    if(shmem!=NULL) {
        deactivate(shmem);
        SharedMemory$delete(&shmem, 1);
    }

    _exit(EXIT_FAILURE);
}

int main(int argc, char *argv[], char *envp[]) {
    int e = -1;
    SharedMemory *shmem = NULL;

    if(argc > 2) {
        static char shortopts[] = "S:K:s:h-";
        static struct option longopts[] = {
            { "start",   required_argument,  NULL, 'S' },
            { "stop",    required_argument,  NULL, 'K' },
            { "status",  required_argument,  NULL, 's' },
            { "help",    no_argument,        NULL, 'h' },
            { "command", no_argument,        NULL, '-' },
            { NULL,     0,                   NULL,  0  }
        };

        extern char *optarg;
        extern int optopt;
        extern int opterr;
        //extern int optreset;

        int options = 0;
        key_t key = 0;

        char o;
        int stop_getopt = 0;
        while((!stop_getopt) && ((o = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)) {
            switch(o) {
                case 'S':
                    options |= DAEMON_START;
                    key = atoi(optarg);
                    if(key > 0) e = 0;
                    break;
                case 'K':
                    options |= DAEMON_STOP;
                    key = atoi(optarg);
                    if(key > 0) e = 0;
                    break;
                case 's':
                    options |= DAEMON_STATUS;
                    key = atoi(optarg);
                    if(key > 0) e = 0;
                    break;
                case 'h':
                    options |= DAEMON_HELP;
                    break;
                case '-':
                    stop_getopt = 1;
                    break;
                case '?':
                default:
                    e=1;
                    break;
            }
        }

        if(e == 0) {
            e = -1;

            extern int optind;
            argc -= optind;
            argv += optind;

            if(options&DAEMON_HELP) {
                e = 0;
                usage("daemonize", e);
            } else if(((options&DAEMON_START) == DAEMON_START) && argc > 0) {
                if((shmem = SharedMemory$new(abs(key), SHMEM_SIZE, 1)))
                    e = start(argc, argv, envp, shmem, key);
            } else if(((options&DAEMON_STOP) == DAEMON_STOP) && argc == 0) {
                if((shmem = SharedMemory$attach(abs(key))))
                    e = stop(argc, argv, envp, shmem, key);
                else
                    printf("Failed to attach to shmem(key=%d)\n", abs(key));
            } else if(((options&DAEMON_STATUS) == DAEMON_STATUS) && argc == 0) {
                shmem = SharedMemory$attach(abs(key));
                e = status(argc, argv, envp, shmem, key);
            } else {
                e = -1;
                usage("daemonize", e);
            }
        } else {
            e = -1;
            usage("daemonize", e);
        }
    } else {
        e = 0;
        usage("daemonize", e);
    }
    return e;
}

int usage(const char *cmd, int e) {
    printf("Usage:\n");
    printf("    %s -S|--start  <channel> -- <command> <arguments>\n", cmd);
    printf("    %s -K|--stop   <channel>\n", cmd);
    printf("    %s -s|--status <channel>\n", cmd);
    printf("\n");
    printf("Note:\n");
    printf("    Any signal sent to the daemon that can be caught is passed through to the\n");
    printf("    encapsulated process, so to kill the daemon itself, send it USR1, or USR2.\n");
    printf("\n");
    printf("    While SIGUSR1 does a complete die (killing the encapsulated processes, and then\n");
    printf("    the daemon wrapper, SIGUSR2 only kills the daemon process, and leaves the child\n");
    printf("    to be adopted by init.\n");
    return e;
}

int start(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key) {
    int e = -1;

    pid_t pid = get_pid(shmem);
    pid_t cpid = get_cpid(shmem);
    unsigned int count = get_count(shmem);

    char buffer[1024];

    if(cpid && pid) {
        daemonstr(buffer, shmem->key, pid, cpid, count);
        printf("Daemon%s already running.\n", buffer);
    } else {
        e = 0;

        printf("Daemonizing...");
        fflush(stdout);

        daemonize(shmem, argc, argv, envp);

        int c = 5; //. Timeout
        while(c--) {
            pid = get_pid(shmem);
            cpid = get_cpid(shmem);
            if(pid && cpid) break;
            sleep(1);
        }
        daemonstr(buffer, shmem->key, pid, cpid, count);
        //SharedMemory$delete(&shmem, 0);

        printf("Done%s\n", buffer);
    }

    return e;
}

int stop(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key) {
    int e = -1;

    pid_t pid = get_pid(shmem);
    pid_t cpid = get_cpid(shmem);
    unsigned int count = get_count(shmem);

    char buffer[1024];
    daemonstr(buffer, shmem->key, pid, cpid, count);

    if(!kill(cpid, 0)) {
        printf("Killing child daemon%s(%d)...", buffer, pid);
        if(kill(cpid, SIGUSR1)) {
            e = 2;
            printf("FAIL\n");
            perror("kill");
        } else {
            printf("DONE\n");
        }
    }

    if((e <= 0) && !kill(pid, 0)) {
        printf("Killing parent daemon%s(%d)...", buffer, pid);
        if(kill(pid, SIGUSR1)) {
            e = 3;
            printf("FAIL\n");
            perror("kill");
        } else {
            printf("DONE\n");
            cleanup(shmem, "kill");
            e = 0;
        }
    } else {
        cleanup(shmem, "kill");
    }

    return e;
}

int status(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key) {
    int e = 0;

    pid_t pid = get_pid(shmem);
    pid_t cpid = get_cpid(shmem);
    unsigned int count = get_count(shmem);

    char buffer[1024];
    daemonstr(buffer, key, pid, cpid, count);

    if(pid && cpid) {
        if(kill(pid, 0))
            printf("%s Dead\n", buffer);
        else if(kill(cpid, 0))
            printf("%s Recovering\n", buffer);
        else
            printf("%s Alive\n", buffer);
    } else {
        printf("%s Missing\n", buffer);
    }

    return e;
}

void daemonstr(char *buffer, key_t key, pid_t pid, pid_t cpid, unsigned int count) {
    char ch_pid = 'P';
    char ch_cpid = 'C';

    if(kill(pid, 0)) ch_pid = 'p';
    if(kill(cpid, 0)) ch_cpid = 'c';
    sprintf(buffer, SFMT, key, ch_pid, pid, ch_cpid, cpid, count);
}


void close_file_descriptors() {
    struct rlimit rl;

    //. Get maximum number of file descriptors...
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
        die(NULL, "getrlimit");

    //. Close all file descriptors...
    if(rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;

    int i;
    for(i=0; i<rl.rlim_max; i++)
        close(i);
}

void wait_for_signal(pid_t cpid) {
    //pause();

    int status;
    pid_t wpid = waitpid(cpid, &status, WUNTRACED|WCONTINUED);
    if(wpid==-1)
        syslog(LOG_ERR, "waitpid: Failed to wait on child.");

    do {
        if(WIFEXITED(status))
            syslog(LOG_ERR, "Child exited, status=%d.", WEXITSTATUS(status));
        else if(WIFSIGNALED(status))
            syslog(LOG_ERR, "Child killed (signal %d).", WTERMSIG(status));
        else if(WIFSTOPPED(status))
            syslog(LOG_ERR, "Child stopped (signal %d).", WSTOPSIG(status));
        else if(WIFCONTINUED(status))
            syslog(LOG_ERR, "Child continued.");
        else
            syslog(LOG_CRIT, "Unexpected status (0x%x).", status);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
}

void daemonize(SharedMemory* shmem, int argc, char* argv[], char *envp[]) {
    //1. Clear file creation mask...
    umask(0);

    //2. Become session leader to lose controlling tty...
    pid_t pid = fork(); /* fork I */
    if(pid<0)
        perror("fork");
    else if(pid!=0) /* parent */
        return;
    setsid();

    //3. Ensure future opens will not allocate controlling terminals...
    pid = fork(); /* fork II */
    if(pid<0)
        perror("fork");
    else if(pid!=0) /* parent */ {
        set_pid(shmem, pid);
        _exit(0);
    }

    //4. Change working directory to root...
    if(chdir("/") < 0)
        perror("chdir");

    //5. Close all file descriptors...
    close_file_descriptors();

    openlog(argv[0], LOG_CONS|LOG_PID, LOG_DAEMON);

    //. Declare a sigaction structure named sa and a signal handler named
    //. sa_handler;
    //. Fill in the sigaction structure: first by calling the sigemptyset
    //. function to initialize the signal set to exclude all signals,
    //. then clearing the sa_flags member and moving the address of the
    //. signal handler into the sa_handler member.
    struct sigaction sa;
    //. void handler(int, siginfo_t*, void*);
    sa.sa_sigaction = handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO|SA_NOCLDWAIT;

    unsigned int timer = 0;
    time_t failure_time = 0;
    time_t exec_time = 0;
    do {
        int sig_rcvd = setjmp(g_state_buf);

        if(sig_rcvd == 0) { //. Normal Operation...
            if(failure_time - exec_time > 5) {
                //. If time delta is more than 5 seconds, reset the decayed
                //. sleep timer back to zero...
                timer = 0;
            }

            //! Call the sigaction function to set up the signal handler so that it
            //. is called when the process receives the SIGHUP and other signals.
            if(sigaction(SIGTERM, &sa, NULL)) die(shmem, "sigaction:SIGTERM");
            if(sigaction(SIGUSR1, &sa, NULL)) die(shmem, "sigaction:SIGUSR1");
            if(sigaction(SIGUSR2, &sa, NULL)) die(shmem, "sigaction:SIGUSR2");
            if(sigaction(SIGINT,  &sa, NULL)) die(shmem, "sigaction:SIGINT");
            if(sigaction(SIGHUP,  &sa, NULL)) die(shmem, "sigaction:SIGHUP");
            if(sigaction(SIGCHLD, &sa, NULL)) die(shmem, "sigaction:SIGCHLD");
            if(sigaction(SIGALRM, &sa, NULL)) die(shmem, "sigaction:SIGCHLD");

            exec_time = time(0);
            pid_t cpid = fork();
            if(cpid==0) /* fork III */ {
                inc_count(shmem);
                syslog(
                    LOG_INFO,
                    "Executing subprocess after a %0.0fs sleep; attempt %d...",
                    exp(timer) - 1,
                    get_count(shmem)
                );
                execvp(argv[0], argv);
                //. execve should never return, if it does, die...
                die(shmem, "error");
            } else /* parent */ {
                syslog(LOG_INFO, "Awaiting child...");

                set_cpid(shmem, cpid);

                //. Activate and then wait for a signal...
                activate(shmem);
                wait_for_signal(cpid);

                //. If we get this far, it means that the process we daemonized has failed...
                failure_time = time(0);
                syslog(LOG_INFO, "Parent dead!");
/*
pid_t pid = get_pid(shmem);
pid_t cpid = get_cpid(shmem);
if(kill(pid, 0)) col_pid = g_RED;
if(kill(cpid, 0)) col_cpid = g_RED;
*/
            }
        } else if(sig_rcvd == SIGUSR1) {
            pid_t cpid = get_cpid(shmem);
            kill(cpid, sig_rcvd);
            die(shmem, "Received the SIGUSR1 signal, kill child, kill daemon.");
        } else if(sig_rcvd == SIGUSR2) {
            die(shmem, "Received the SIGUSR2 signal, kill daemon (but leave child to init).");
        } else {
            syslog(LOG_ERR, "Received signal %d, passing signal onto child.", sig_rcvd);
            pid_t cpid = get_cpid(shmem);
            kill(cpid, sig_rcvd);
        }

        syslog(LOG_ERR, "End of loop, will I loop again? %s.", active(shmem)?"Yes":"No");
        if(!active(shmem)) {
            cleanup(shmem, "Exit cleanup called.");
        } else {
            sleep(exp(timer++) - 1);
        }
    } while(active(shmem));

    return;
}

