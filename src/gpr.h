#include <elf.h>

typedef struct {
    uint8_t id;
    typedef union {
        uint32_t dword;             //. EAX
        struct {
            unsigned x      : 0x10; //. AX
            unsigned        : 0x10;
        };
        struct {
            unsigned l      : 0x08; //. AL
            unsigned h      : 0x08; //. AH
            unsigned        : 0x10;
        };
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
        }; //. EFLAGS
    } memory;
} register32_t;
