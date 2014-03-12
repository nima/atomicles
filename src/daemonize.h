// TODO: shm_open, mmap, mlock, sigaction() and sigqueue()
// TODO: http://mij.oltrelinux.com/devel/unixprg/#ipc__posix_shm
// TODO: http://www.linuxprogrammingblog.com/code-examples/sigaction

#include "SharedMemory.h"

#include <math.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DAEMON_HELP   2 << 0
#define DAEMON_STOP   2 << 1
#define DAEMON_START  2 << 2
#define DAEMON_STATUS 2 << 3

#define OFFSET_ACTIVE 0
#define OFFSET_PID 1
#define OFFSET_CPID 2
#define SHMEM_SIZE (3*sizeof(unsigned int)) //. bytes to store the above three values

int status(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key);
int stop(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key);
int start(int argc, char *argv[], char *envp[], SharedMemory *shmem, key_t key);

void daemonize(SharedMemory* shmem, int argc, char* argv[], char *envp[]);

int usage(const char *cmd, int e);
void daemonstr(char *buffer, key_t key, pid_t pid, pid_t cpid);

static void activate(SharedMemory *shmem) { SharedMemory$write_uint(shmem, OFFSET_ACTIVE, 1); }
static void deactivate(SharedMemory *shmem) { SharedMemory$write_uint(shmem, OFFSET_ACTIVE, 0); }
static unsigned int active(SharedMemory *shmem) { return SharedMemory$read_uint(shmem, OFFSET_ACTIVE); }

static void set_pid(SharedMemory *shmem, pid_t pid) { SharedMemory$write_uint(shmem, OFFSET_PID, pid); }
static pid_t get_pid(SharedMemory *shmem) { return shmem ? SharedMemory$read_uint(shmem, OFFSET_PID) : 0; }

static void set_cpid(SharedMemory *shmem, pid_t cpid) { SharedMemory$write_uint(shmem, OFFSET_CPID, cpid); }
static pid_t get_cpid(SharedMemory *shmem) { return shmem ? SharedMemory$read_uint(shmem, OFFSET_CPID) : 0; }

//. Handler functions usually don't do very much. The best practice is to write
//. a handler that does nothing but set an external variable that the program
//. checks regularly, and leave all serious work to the program. This is best
//. because the handler can be called at asynchronously, at unpredictable times;
//. perhaps in the middle of a system call, or even between the beginning and the
//. end of a C operator that requires multiple instructions. The data structures
//. being manipulated might therefore be in an inconsistent state when the
//. handler function is invoked. Even copying one int variable into another can
//. take two instructions on most machines.
//.
//. Reference: http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_21.html
volatile sig_atomic_t killme;
jmp_buf g_state_buf;
