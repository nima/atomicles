#include <elf.h>
#include <unistd.h>

#define OPERAND_DST 0
#define OPERAND_SRC 1
#define OPERAND_BUF 2

////////////////////////////////////. clss id   size
#define FILTER_REG32_EFLAGS 0x010 //. 0000 0000 0100
#define REG32_EFLAGS        0x010 //. 0000 0000 0100

#define FILTER_REG32_GPR    0x104 //. 0001 0000 0100
#define REG32_GPR_EAX       0x104 //. 0001 0000 0100
#define REG32_GPR_EBX       0x114 //. 0001 0001 0100
#define REG32_GPR_ECX       0x124 //. 0001 0010 0100
#define REG32_GPR_EDX       0x134 //. 0001 0011 0100

#define FILTER_REG16_GPR    0x102 //. 0001 0000 0010
#define REG16_GPR_AX        0x102 //. 0001 0000 0010
#define REG16_GPR_BX        0x112 //. 0001 0001 0010
#define REG16_GPR_CX        0x122 //. 0001 0010 0010
#define REG16_GPR_DX        0x132 //. 0001 0011 0010

#define FILTER_REG08_GPR    0x101 //. 0001 0000 0001
#define REG16_GPR_AL        0x101 //. 0001 0000 0001
#define REG16_GPR_BL        0x111 //. 0001 0001 0001
#define REG16_GPR_CL        0x121 //. 0001 0010 0001
#define REG16_GPR_DL        0x131 //. 0001 0011 0001
#define REG16_GPR_AH        0x141 //. 0001 0100 0001
#define REG16_GPR_BH        0x151 //. 0001 0101 0001
#define REG16_GPR_CH        0x161 //. 0001 0110 0001
#define REG16_GPR_DH        0x171 //. 0001 0111 0001

#define FILTER_ID           0xFFF //. 1111 1111 1111
#define FILTER_CLS          0xF00 //. 1111 0000 0000
#define FILTER_SIZE         0x00F //. 0000 0000 1111
#define SHIFT_CLS               6
#define SHIFT_SIZE              0

#define registers$size(r) (((r)->id & FILTER_SIZE) << 3);

typedef struct {
    uint32_t id;
    union {
        uint32_t dword;
        uint32_t value;
    };
} register32_t;

typedef struct {
    uint32_t id;
    union {
        uint32_t dword;             //. EAX
        uint32_t value;             //. EAX
        struct {
            unsigned x      : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned l      : 0x08; //. AL
            unsigned h      : 0x08; //. AH
            unsigned        : 0x10;
        };
    };
} gpr32_t;

typedef struct {
    uint32_t id;
    union {
        uint32_t dword;             //. EAX
        struct {
            unsigned value  : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned x      : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned l      : 0x08; //. AL
            unsigned h      : 0x08; //. AH
            unsigned        : 0x10;
        };
    };
} gpr16_t;

typedef struct {
    uint32_t id;
    union {
        uint32_t dword;             //. EAX
        struct {
            unsigned x      : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned value  : 0x08; //. AL
            unsigned        : 0x08;
            unsigned        : 0x10;
        };
        struct {
            unsigned l      : 0x08; //. AL
            unsigned h      : 0x08; //. AH
            unsigned        : 0x10;
        };
    };
} gpr8l_t;

typedef struct {
    uint32_t id;
    union {
        uint32_t dword;             //. EAX
        struct {
            unsigned x      : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned        : 0x08;
            unsigned value  : 0x08; //. AH
            unsigned        : 0x10;
        };
        struct {
            unsigned l      : 0x08; //. AL
            unsigned h      : 0x08; //. AH
            unsigned        : 0x10;
        };
    };
} gpr8h_t;


typedef struct {
    uint32_t id;
    union {
        uint32_t dword;
        struct {
            unsigned CF     : 0x1;
            unsigned r$0x01 : 0x1;
            unsigned PF     : 0x1;
            unsigned r$0x03 : 0x1;
            unsigned AF     : 0x1;
            unsigned r$0x05 : 0x1;
            unsigned ZF     : 0x1;
            unsigned SF     : 0x1;

            unsigned TF     : 0x1;
            unsigned IF     : 0x1;
            unsigned DF     : 0x1;
            unsigned OF     : 0x1;
            unsigned IOPL   : 0x2;
            unsigned NT     : 0x1;
            unsigned r$0x0F : 0x1;

            unsigned RF     : 0x1;
            unsigned VM     : 0x1;
            unsigned AC     : 0x1;
            unsigned VIF    : 0x1;
            unsigned VIP    : 0x1;
            unsigned ID     : 0x1;
            unsigned r$0x16 : 0xA;
        };
    };
} eflags32_t;

register32_t *registers$eflags(void);
register32_t *registers$eax(void);
register32_t *registers$ebx(void);
register32_t *registers$ecx(void);
register32_t *registers$edx(void);

void registers$init(void);
void registers$delete(void);
void registers$print(uint32_t *operands[2], uint32_t immediate);
register32_t *registers$register_by_name(const char *);
register32_t *registers$register(uint32_t);
