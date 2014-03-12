#include "registers.h"
#include "SharedMemory.h"

#include <stdint.h>

#define OFFSET_EFLAGS 0
#define OFFSET_EAX    1
#define OFFSET_EBX    2
#define OFFSET_ECX    3
#define OFFSET_EDX    4

static eflags32_t *__registers$eflags;
static gpr32_t *__registers$eax;
static gpr32_t *__registers$ebx;
static gpr32_t *__registers$ecx;
static gpr32_t *__registers$edx;

static register32_t **__registers$REGISTERS[0xFFF];

#define SHMEM_KEY 32 //+IPC_PRIVATE
static SharedMemory *__registers$shmem;
void registers$init() {
    __registers$shmem = SharedMemory$new(SHMEM_KEY, 5 * sizeof(uint32_t), 1);
    assert(__registers$shmem != NULL);

    __registers$eflags = malloc(sizeof(register32_t));
    __registers$eflags->dword = SharedMemory$read_uint32(__registers$shmem, OFFSET_EFLAGS);
    __registers$eflags->id = REG32_EFLAGS;
    __registers$eflags->r$0x01 = 1;
    __registers$eflags->r$0x03 = 0;
    __registers$eflags->r$0x05 = 0;
    __registers$eflags->r$0x0F = 0;

    __registers$eax = malloc(sizeof(register32_t));
    __registers$eax->dword = SharedMemory$read_uint32(__registers$shmem, OFFSET_EAX);
    __registers$eax->id = REG32_GPR_EAX;

    __registers$ebx = malloc(sizeof(register32_t));
    __registers$ebx->dword = SharedMemory$read_uint32(__registers$shmem, OFFSET_EBX);
    __registers$ebx->id = REG32_GPR_EBX;

    __registers$ecx = malloc(sizeof(register32_t));
    __registers$ecx->dword = SharedMemory$read_uint32(__registers$shmem, OFFSET_ECX);
    __registers$ecx->id = REG32_GPR_ECX;

    __registers$edx = malloc(sizeof(register32_t));
    __registers$edx->dword = SharedMemory$read_uint32(__registers$shmem, OFFSET_EDX);
    __registers$edx->id = REG32_GPR_EDX;

    __registers$REGISTERS[REG32_EFLAGS]  = (register32_t **)&__registers$eflags;

    __registers$REGISTERS[REG32_GPR_EAX] = (register32_t **)&__registers$eax;
    __registers$REGISTERS[REG16_GPR_AX]  = (register32_t **)&__registers$eax;
    __registers$REGISTERS[REG16_GPR_AL]  = (register32_t **)&__registers$eax;
    __registers$REGISTERS[REG16_GPR_AH]  = (register32_t **)&__registers$eax;

    __registers$REGISTERS[REG32_GPR_EBX] = (register32_t **)&__registers$ebx;
    __registers$REGISTERS[REG16_GPR_BX]  = (register32_t **)&__registers$ebx;
    __registers$REGISTERS[REG16_GPR_BL]  = (register32_t **)&__registers$ebx;
    __registers$REGISTERS[REG16_GPR_BH]  = (register32_t **)&__registers$ebx;

    __registers$REGISTERS[REG32_GPR_ECX] = (register32_t **)&__registers$ecx;
    __registers$REGISTERS[REG16_GPR_CX]  = (register32_t **)&__registers$ecx;
    __registers$REGISTERS[REG16_GPR_CL]  = (register32_t **)&__registers$ecx;
    __registers$REGISTERS[REG16_GPR_CH]  = (register32_t **)&__registers$ecx;

    __registers$REGISTERS[REG32_GPR_EDX] = (register32_t **)&__registers$edx;
    __registers$REGISTERS[REG16_GPR_DX]  = (register32_t **)&__registers$edx;
    __registers$REGISTERS[REG16_GPR_DL]  = (register32_t **)&__registers$edx;
    __registers$REGISTERS[REG16_GPR_DH]  = (register32_t **)&__registers$edx;
}

void registers$delete() {
    /*
    int i;
    for(i=0;i<0xfff;i++)
        if(__registers$REGISTERS[i])
            printf("%i: %08x\n", i, *__registers$REGISTERS[i]);
    */
    SharedMemory$write_uint32(__registers$shmem, OFFSET_EFLAGS, __registers$eflags->dword);
    SharedMemory$write_uint32(__registers$shmem, OFFSET_EAX, __registers$eax->dword);
    SharedMemory$write_uint32(__registers$shmem, OFFSET_EBX, __registers$ebx->dword);
    SharedMemory$write_uint32(__registers$shmem, OFFSET_ECX, __registers$ecx->dword);
    SharedMemory$write_uint32(__registers$shmem, OFFSET_EDX, __registers$edx->dword);
    SharedMemory$delete(&__registers$shmem, 0);
    free(__registers$eflags);
    free(__registers$eax);
    free(__registers$ebx);
    free(__registers$ecx);
    free(__registers$edx);
}

register32_t *registers$register(uint32_t id) {
    register32_t *r = *__registers$REGISTERS[id];
    r->id = id;
    return r;
}

register32_t *registers$register_by_name(const char *operand) {
    uint32_t id = REG32_EFLAGS;
    if(memcmp(operand, "eflags", 7) != 0) {
        char c = operand[strlen(operand) - 2];
        id = 0x100 | ((c - 'a') << 4);

        c = operand[1];
        switch(c) {
            case 'l': id |= 1; break;
            case 'h': id |= 1; break;
            case 'x': id |= 2; break;
            default:  id |= 4; break;
        }
    }

    return registers$register(id);
}

#define __registers$printGPR(gpr) {\
    printf(\
        "%s [%c]   : 0x%08X\n", #gpr,\
        ((operands[0] == &__registers$##gpr->dword) \
        && (operands[1] == &__registers$##gpr->dword))\
            ? '*'\
            : ((operands[OPERAND_SRC] == &__registers$##gpr->dword)\
                ? 'S'\
                : ((operands[OPERAND_DST] == &__registers$##gpr->dword)\
                    ? 'D'\
                    : ' ')),\
        __registers$##gpr->dword\
    );\
}

void registers$print(uint32_t *operands[2], uint32_t immediate) {
    printf(
        "eflags    : 0x%08X [status:%c%c%c%c%c%c] [control:%c]\n",
        __registers$eflags->dword,
        __registers$eflags->CF ? 'C' : 'c',
        __registers$eflags->PF ? 'P' : 'p',
        __registers$eflags->AF ? 'A' : 'a',
        __registers$eflags->ZF ? 'Z' : 'z',
        __registers$eflags->SF ? 'S' : 's',
        __registers$eflags->OF ? 'O' : 'o',
        __registers$eflags->DF ? 'D' : 'd'
    );

    printf("immediate : 0x%08x\n", immediate);

    __registers$printGPR(eax);
    __registers$printGPR(ebx);
    __registers$printGPR(ecx);
    __registers$printGPR(edx);
}
