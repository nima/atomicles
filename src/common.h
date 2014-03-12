#ifndef __COMMON__
#define __COMMON__

#define SHMKEYPATH "/dev/zero"
#define SEMKEYPATH "/dev/null"

#define SHMKEY(key) ftok(SHMKEYPATH, (key))
#define SEMKEY(key) ftok(SEMKEYPATH, (key))

#endif
