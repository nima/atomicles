/* http://c.learncodethehardway.org/book/ex20.html */

#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifdef DEBUG
#if DEBUG > 3
#define dbg_dbug(M, ...) fprintf(stderr,\
    ANSI_COLOR_MAGENTA "[DBUG] (%s:%d)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, ##__VA_ARGS__\
)
#else
#define dbg_dbug(M, ...)
#endif

#if DEBUG > 2
#define dbg_info(M, ...) fprintf(stderr,\
    ANSI_COLOR_BLUE    "[INFO] (%s:%d)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, ##__VA_ARGS__\
)
#else
#define dbg_info(M, ...)
#endif

#if DEBUG > 1
#define dbg_warn(M, ...) fprintf(stderr,\
    ANSI_COLOR_YELLOW  "[WARN] (%s:%d)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, ##__VA_ARGS__\
)
#else
#define dbg_warn(M, ...)
#endif

#if DEBUG > 0
#define dbg_errr(M, ...) fprintf(stderr,\
    ANSI_COLOR_RED     "[ERRR] (%s:%d)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, ##__VA_ARGS__\
)
#else
#define dbg_errr(M, ...)
#endif
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_dbug(M, ...) fprintf(stderr,\
    ANSI_COLOR_MAGENTA "[DBUG] (%s:%d; %s)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__\
); errno=0
#define log_errr(M, ...) fprintf(stderr,\
    ANSI_COLOR_RED     "[ERRR] (%s:%d; %s)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__\
); errno=0
#define log_warn(M, ...) fprintf(stderr,\
    ANSI_COLOR_YELLOW  "[WARN] (%s:%d; %s)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__\
); errno=0
#define log_info(M, ...) fprintf(stderr,\
    ANSI_COLOR_BLUE    "[INFO] (%s:%d; %s)"\
    ANSI_COLOR_RESET   " - " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__\
); errno=0

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
