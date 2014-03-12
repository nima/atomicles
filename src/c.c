#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <limits.h> /* CHAR_MIN, CHAR_MAX */
#include <stddef.h> /* wchar_t */
#include <ctype.h>

#define SIZE_OF(type) {\
    int i = sizeof(type);\
    printf("%16s: %2i ", #type, i);\
    while(i--) printf("#");\
    printf("\n");\
}

void chk_char(void) {
    printf("%12s: %d..%d\n", "char", CHAR_MIN, CHAR_MAX);
}

#define PTRCMP(cmp1, cmp2) {\
    int a1[2] = { 1, 7 };\
    int a2[2] = { 1, 7 };\
    void *addr1[2];\
    void *addr2[2];\
    int *p;\
    p = a1; addr1[0] = p; cmp1; addr1[1] = p;\
    p = a2; addr2[0] = p; cmp2; addr2[1] = p;\
    printf(\
        "%s == %s : %s\n",\
        #cmp1, #cmp2,\
        (\
            (a1[0] == a2[0])\
            && (a1[1] == a2[1])\
            && ((addr1[1] - addr1[0]) == (addr2[1] - addr2[0]))\
        ) ? "YES" : "NO"\
    );\
}

#define HOLY_STRUCT(s, e1, e2, e3) {\
    struct s ms;\
    memset(&ms, '4', sizeof ms);\
    ms.e1 = 5;\
    ms.e2 = 5;\
    ms.e3 = 5;\
    unsigned int i;\
    unsigned int *ptr;\
    printf("%p..%p\n", ptr, ptr + sizeof ms);\
    for(ptr=&ms; ptr<&ms + sizeof ms / sizeof(unsigned int); ptr++)\
        printf("%p %08x\n", ptr, (unsigned int)(*ptr));\
}

void(*fsm[])(int, float) = {
};
//fsm[1](1, 3.4);

void fn(int arg){printf("%d\n", arg);}
int main(int argc, char *argv[], char *envp[]) {
    chk_char();
    SIZE_OF(char);
    SIZE_OF(wchar_t);
    SIZE_OF(short);
    SIZE_OF(int);
    SIZE_OF(long);
    SIZE_OF(long long);
    SIZE_OF(float);
    SIZE_OF(double);
    SIZE_OF(sizeof(void *));

    { int i=0; i += ++i; assert(i == 2); }
    { int i=0; i += i++; assert(i == 1); }
    { char c = 'free'; assert(c == 101); }
    {
        unsigned long  l = 0x12345678;
        unsigned short s = 0x1234;
        printf("s:%x, l:%lx\n", s, l);

        s = ((struct { unsigned short $; }){(l)}.$);
        printf("s:%x, l:%lx\n", s, l);

        s = (unsigned short)l;
        printf("s:%x, l:%lx\n", s, l);

        s = l;
        printf("s:%x, l:%lx\n", s, l);
    }


    {
        float fa[8], *fp1, *fp2;
        fp1 = fp2 = fa; /* address of first element */
        while(fp2++ != &fa[8]){
            printf(
                "\\___ptr-delta:%3li vs int-delta:%3li\n",
                (long)(fp2 - fp1),
                (long)fp2 - (long)fp1
            );
        }
    }

    PTRCMP(++*p, ++(*p));
    PTRCMP(*++p, *(++p));
    PTRCMP(*p++, *(p++));
    PTRCMP(*p++, (*p)++);

    {
        size_t sz = sizeof(sz);
        printf("size of sizeof is %lu\n", (unsigned long)sz);
    }

    {
        void (*fp1)(int), (*fp2)(int);
        fp1 = &fn;
        fp2 = fn;

        (*fp1)(1); fp2(2);
        (*fp1)(3); fp2(4);
    }

    {
        struct mystruct {
            short si1;
            long si2;
            short si3;
        };
        HOLY_STRUCT(mystruct, si1, si2, si3);
    }

    printf("$ is ok: %i\n", isalpha('$'));

    exit(EXIT_SUCCESS);
}

