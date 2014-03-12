#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<elf.h>
#include<getopt.h>
#include<math.h>

#include "bits.h"

typedef enum {
    SAL, SAR, SHL, SHR,
    MOV,
    ADD, SUB,
    OR, AND, XOR,
    JA, JAE, JB, JBE, JC, JCXZ, JECXZ, JE, JG, JGE, JL, JLE, JNA, JNAE, JNB,
    JNBE, JNC, JNE, JNG, JNGE, JNL, JNLE, JNO, JNP, JNS, JNZ, JO, JP, JPE,
    JPO, JS, JZ,
    TEST, CMP
} OPCODE;
#define IS(optarg, opcode) (memcmp(optarg, #opcode, strlen(#opcode)+1) == 0)
int set_instruction(uint32_t *opcode, const char *opcodestr) {
    int e = EXIT_SUCCESS;
    if(     IS(opcodestr, SAL)) *opcode = SAL;
    else if(IS(opcodestr, SAR)) *opcode = SAR;
    else if(IS(opcodestr, SHL)) *opcode = SHL;
    else if(IS(opcodestr, SHR)) *opcode = SHR;
    else if(IS(opcodestr, JA)) *opcode = JA;
    else if(IS(opcodestr, JAE)) *opcode = JAE;
    else if(IS(opcodestr, JB)) *opcode = JB;
    else if(IS(opcodestr, JBE)) *opcode = JBE;
    else if(IS(opcodestr, JC)) *opcode = JC;
    else if(IS(opcodestr, JCXZ)) *opcode = JCXZ;
    else if(IS(opcodestr, JECXZ)) *opcode = JECXZ;
    else if(IS(opcodestr, JE)) *opcode = JE;
    else if(IS(opcodestr, JG)) *opcode = JG;
    else if(IS(opcodestr, JGE)) *opcode = JGE;
    else if(IS(opcodestr, JL)) *opcode = JL;
    else if(IS(opcodestr, JLE)) *opcode = JLE;
    else if(IS(opcodestr, JNA)) *opcode = JNA;
    else if(IS(opcodestr, JNAE)) *opcode = JNAE;
    else if(IS(opcodestr, JNB)) *opcode = JNB;
    else if(IS(opcodestr, JNBE)) *opcode = JNBE;
    else if(IS(opcodestr, JNC)) *opcode = JNC;
    else if(IS(opcodestr, JNE)) *opcode = JNE;
    else if(IS(opcodestr, JNG)) *opcode = JNG;
    else if(IS(opcodestr, JNGE)) *opcode = JNGE;
    else if(IS(opcodestr, JNL)) *opcode = JNL;
    else if(IS(opcodestr, JNLE)) *opcode = JNLE;
    else if(IS(opcodestr, JNO)) *opcode = JNO;
    else if(IS(opcodestr, JNP)) *opcode = JNP;
    else if(IS(opcodestr, JNS)) *opcode = JNS;
    else if(IS(opcodestr, JNZ)) *opcode = JNZ;
    else if(IS(opcodestr, JO)) *opcode = JO;
    else if(IS(opcodestr, JP)) *opcode = JP;
    else if(IS(opcodestr, JPE)) *opcode = JPE;
    else if(IS(opcodestr, JPO)) *opcode = JPO;
    else if(IS(opcodestr, JS)) *opcode = JS;
    else if(IS(opcodestr, JZ)) *opcode = JZ;
    else if(IS(opcodestr, MOV)) *opcode = MOV;
    else if(IS(opcodestr, AND)) *opcode = AND;
    else if(IS(opcodestr, XOR)) *opcode = XOR;
    else if(IS(opcodestr, OR)) *opcode = OR;
    else if(IS(opcodestr, ADD)) *opcode = ADD;
    else if(IS(opcodestr, SUB)) *opcode = SUB;
    else if(IS(opcodestr, TEST)) *opcode = TEST;
    else {
        *opcode = 0;
        e = EXIT_FAILURE;
    }

    return e;
}

#include "registers.h"

#define EFLAGS_RESET 0x00000020
static eflags32_t *eflags;

uint32_t shift(int ins, uint32_t dst, uint32_t cnt) {
    uint32_t cnt_tmp = cnt;
    cnt_tmp &= 0x1F;
    while(cnt_tmp != 0) {
        if(ins == SAL || ins == SHL) {
            eflags->CF = MSB(dst);
            dst <<= 1;
        } else { //instruction is SAR or SHR
            eflags->CF = LSB(dst);
            if(ins == SAR) { //Signed divide, rounding toward negative infinity
                uint32_t signum = dst;
                signum &= 0x80000000;
                dst >>= 1;
                dst |= signum;
            } else //Instruction is SHR
                dst >>= 1; //Unsigned divide
        }
        cnt_tmp -= 1;
    }

    if((cnt & 0x1F) == 1) { //Determine overflow
        if(ins == SAL || ins == SHL)
            eflags->OF = MSB(dst) ^ eflags->CF;
        else if(ins == SAR)
            eflags->OF = 0;
        else //ins == SHR
            eflags->OF = MSB(dst);
    } /* else {
        //eflags->OF = UNDEF;
        0;
    } */
    return dst;
}

uint32_t buffer;
uint8_t target;
register32_t *registers[2] = {
    NULL, //. Destination Register
    NULL, //. Source Register
};
uint32_t *operands[3] = {
    NULL, //. Destination Operand
    NULL, //. Source Operand
    &buffer
};

void setEFLAGS(eflags32_t *eflags, unsigned int opcode) {
    switch(opcode) {
        case SUB:
        case CMP:
            if(*operands[OPERAND_DST] < *operands[OPERAND_SRC])
                eflags->CF = 1;
            break;
        case ADD:
            if(*operands[OPERAND_BUF] < *operands[OPERAND_DST])
                eflags->CF = 1;
        case TEST:
            //. The TEST operation sets the flags CF and OF to zero.
            eflags->CF = 0;
            eflags->OF = 0;
            //. The SF is set to the MSB of the result of the AND.
            eflags->SF = MSB(*operands[OPERAND_BUF]);
            //. If the result of the AND is 0, the ZF is set to 1, otherwise set to 0.
            eflags->ZF = !(*operands[OPERAND_BUF]);
            //The parity flag is set to the bitwise XNOR of the result of the AND.
            //byte_t tmp8;
            //tmp8.BYTE = (uint8_t)and;
            //eflags->PF = BitwiseXNOR(tmp8);
            eflags->PF = BitwiseXNOR32((dword_t)*operands[OPERAND_BUF]);
            //. The value of AF is undefined.
            //eflags->AF = Undefined;
    }

    eflags->ZF = !(*operands[OPERAND_BUF]);
}
void commit(void) {
    *operands[OPERAND_DST] = *operands[OPERAND_BUF];
    *operands[OPERAND_BUF] = 0xFFFFFFFF;
}

int check_operands(opcode) {
    int e = EXIT_FAILURE;
    switch(opcode) {
        case SAR:
        case SHR:
        case SAL:
        case SHL:
            if(operands[OPERAND_DST] == NULL)
                fprintf(stderr, "E: Specify the DST register first!\n");
            else e = EXIT_SUCCESS;
            break;
        default:
            if((operands[OPERAND_DST] == NULL) || (operands[OPERAND_SRC] == NULL))
                fprintf(stderr, "E: Specify the SRC and DST register first!\n");
            else e = EXIT_SUCCESS;
            if(e == EXIT_SUCCESS) {
                uint8_t dsize = registers$size(registers[OPERAND_SRC]);
                uint8_t ssize = registers$size(registers[OPERAND_DST]);
                if(dsize != ssize) {
                    fprintf(stderr, "E: Incompatible TEST operands: %d-bit vs %d-bit!\n", ssize, dsize);
                    e = EXIT_FAILURE;
                }
            }
        break;
    }

    return e;
}

short jump;
static uint32_t immediate;
int main(int argc, char *argv[]) {
    int v_f = 0;
    int h_f = 0;

    int e = 0;
    char c = 0;
    uint8_t count = 1;
    unsigned int opcode = 0;
    uint32_t **operand_ptr = NULL;

    registers$init();

    eflags = (eflags32_t *)registers$register(REG32_EFLAGS);
    register32_t *r = NULL;
    gpr32_t *r32 = NULL;
    gpr16_t *r16 = NULL;
    //gpr8l_t *r8l = NULL;
    //gpr8h_t *r8h = NULL;

    int option_index = 0;
    const char short_options[] = "hsdir:v:";
    struct option long_options[] = {
        /* These options set a flag. */
        //{"verbose"  , no_argument      , &v_f,   0},
        {"help"     , no_argument      , &h_f, 'h'},
        {"src"      , no_argument      ,    0, 's'},
        {"dst"      , no_argument      ,    0, 'd'},
        {"immediate", no_argument      ,    0, 'i'},
        {"register" , required_argument,    0, 'r'},
        {"value"    , required_argument,    0, 'v'},
        {0          ,                 0,    0,   0}
    };
    while((c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch(c) {
            case 0:
                v_f = 1;
            break;

            case 'h':
                h_f = 1;
            break;

            case 'd':
            case 's':
                target = (c == 'd') ? OPERAND_DST : OPERAND_SRC;
                operand_ptr = &operands[target];
            break;

            case 'i':
                if(target == OPERAND_SRC)
                    *operand_ptr = &immediate;
                else {
                    fprintf(stderr, "E: Destination cannot be an immediate!\n");
                    e = 1;
                }
            break;

            case 'r':
                if(operand_ptr != NULL) {
                    r = registers$register_by_name(optarg);
                    if(r != NULL) {
                        *operand_ptr = &r->dword;
                        registers[target] = (register32_t *)r;
                    } else {
                        fprintf(stderr, "E: %s is not a valid register!\n", optarg);
                        e = 1;
                    }
                } else {
                    fprintf(stderr, "E: Set the register first!\n");
                    e = 1;
                }
            break;

            case 'v':
                if(*operand_ptr != NULL) {
                    uint32_t operand_val;
                    if(memcmp(optarg, "RST", 4) != 0) { //. Is the value supplied not a reset request
                        char *ptr = NULL;
                        operand_val = strtoul(optarg, &ptr, 0);
                        uint8_t size = registers$size(registers[target]);
                        if(operand_val <= pow(2, size)) {
                            if(*ptr != '\0') {
                                operand_val = 0;
                                ptr = optarg;
                                do {
                                    operand_val |= hctoc(*ptr);
                                    ptr++;
                                    if(*ptr != '\0')
                                        operand_val <<= 4;
                                } while(*ptr != '\0');
                            }
                        } else {
                            e = EXIT_FAILURE;
                            fprintf(stderr, "E: Overflow!\n");
                        }
                    } else if(*operand_ptr == &eflags->dword) {
                        operand_val = EFLAGS_RESET;
                    }

                    **operand_ptr = operand_val;
                } else {
                    e = EXIT_FAILURE;
                    fprintf(stderr, "E: Specify the register first!\n");
                }
            break;

            default:
                e = EXIT_FAILURE;
                fprintf(stderr, "E: Invalid option (as reported).\n");
            break;
        }
    }

    if(e == EXIT_SUCCESS && optind < argc) {
        e = set_instruction(&opcode, argv[optind]);
        if(e == EXIT_SUCCESS) {
            char *ptr = NULL;
            switch(opcode) {
                case SAR:
                case SHR:
                case SAL:
                case SHL:
                    if(optind == argc - 2) {
                        char *countstr = argv[argc-1];
                        count = strtoul(countstr, &ptr, 0);
                        if(*ptr != '\0') {
                            fprintf(stderr, "E: Invalid count: `%s', expecting <int>.\n", countstr);
                            e = EXIT_FAILURE;
                        }
                    } else goto toomanyargs;
                break;

toomanyargs:
                default:
                    if(optind != argc - 1) {
                        fprintf(stderr, "E: Invalid syntax, too many arguments for instruction.\n");
                        e = EXIT_FAILURE;
                    }
                break;
            }
        } else {
            h_f = 1;
            fprintf(stderr, "E: Unsupported opcode/instruction `%s'\n", argv[optind]);
        }
    } else {
        fprintf(stderr, "E: Specify the IA32 Instruction too, dim-witted one.\n");
        e = EXIT_FAILURE;
    }

    if(h_f) {
        printf("Usage: %s <options> <opcode>\n", argv[0]);
        printf("Options:\n");
        printf(" -h|--help\n");
        printf(" -r|--register eax|ebx|ecx|edx|eflags\n");
        printf(" -v|--value    RST | <BAADBEEF>\n");
    } else if(e == 0) {
        switch(opcode) {
            case SAR:
            case SHR:
            case SAL:
            case SHL:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] = shift(SAR, *operands[OPERAND_DST], count);
                    setEFLAGS(eflags, opcode);
                    commit();
                }
            break;

            case MOV:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] = *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    commit();
                }
            break;

            case SUB:
            case CMP:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] -= *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    if(opcode == SUB)
                        commit();
                }
            break;

            case AND:
            case TEST:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] = *operands[OPERAND_DST] & *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    if(opcode == AND)
                        commit();
                }
            break;


            case ADD:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] += *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    commit();
                }
            break;

            case OR:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] |= *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    commit();
                }
            break;

            case XOR:
                if((e = check_operands(opcode)) == EXIT_SUCCESS) {
                    *operands[OPERAND_BUF] ^= *operands[OPERAND_SRC];
                    setEFLAGS(eflags, opcode);
                    commit();
                }
            break;

            case JCXZ:
                r16 = (gpr16_t *)registers$register(REG16_GPR_CX);
                jump = (r32->value == 0x0000);
                goto jump;

            case JECXZ:
                r32 = (gpr32_t *)registers$register(REG32_GPR_ECX);
                jump = (r32->value == 0x00000000);
                goto jump;

            case JNB:
            case JNC:
            case JAE: jump = !eflags->CF; goto jump;

            case JNO: jump = !eflags->OF; goto jump;

            case JPO:
            case JNP: jump = !eflags->PF; goto jump;

            case JNS: jump = !eflags->SF; goto jump;

            case JNE:
            case JNZ: jump = !eflags->ZF; goto jump;

            case JC:
            case JNAE:
            case JB:  jump = eflags->CF; goto jump;

            case JO:  jump = eflags->OF; goto jump;

            case JPE:
            case JP:  jump = eflags->PF; goto jump;

            case JS:  jump = eflags->SF; goto jump;

            case JE:
            case JZ:  jump = eflags->ZF; goto jump;

            case JNBE:
            case JA:  jump = (!eflags->CF && !eflags->ZF); goto jump;

            case JBE:
            case JNA: jump = (eflags->CF || eflags->ZF); goto jump;

            case JNLE:
            case JG:  jump = (!eflags->ZF && (eflags->SF == eflags->OF)); goto jump;

            case JNG:
            case JLE: jump = (eflags->ZF || (eflags->SF != eflags->OF)); goto jump;

            case JNL:
            case JGE: jump = (eflags->SF == eflags->OF); goto jump;

            case JNGE:
            case JL:  jump = (eflags->SF != eflags->OF); goto jump;
jump:
                printf("Instruction will %s.\n", jump ? "jump" : "not jump");
            break;

            default:
                printf("S: Not Implemented. Sorry.\n");
                e = EXIT_FAILURE;
        };
        if(e == EXIT_SUCCESS)
            registers$print(operands, immediate);
    }

    registers$delete();

    return 0;
}
