#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <Windows.h>
#include <conio.h> 


//Memory Mapped Registers
enum
{
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02  
};


//Trap codes
enum
{
    TRAP_GETC = 0x20,
    TRAP_OUT = 0x21,  
    TRAP_PUTS = 0x22,  
    TRAP_IN = 0x23,    
    TRAP_PUTSP = 0x24, 
    TRAP_HALT = 0x25  
};


//Memory storage
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; 


//Registers
// Total 10 registers: 8 general purpose registers (R0-R7) - 1 program counter (PC) register - 1 condition flags (COND)
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, 
    R_COND,
    R_COUNT
};
uint16_t reg[R_COUNT];


//16 Opcodes instruction set
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};


//Condition Flags
enum
{
    FL_POS = 1 << 0, 
    FL_ZRO = 1 << 1, 
    FL_NEG = 1 << 2, 
};

const int R_PC_CONST = R_PC;
const int R_COND_CONST = R_COND;
const int FL_ZRO_CONST = FL_ZRO;

//Input Buffering
HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

void disable_input_buffering()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode);
    fdwMode = fdwOldMode
            ^ ENABLE_ECHO_INPUT 
            ^ ENABLE_LINE_INPUT; 
    SetConsoleMode(hStdin, fdwMode); 
    FlushConsoleInputBuffer(hStdin); 
}
void restore_input_buffering()
{
    SetConsoleMode(hStdin, fdwOldMode);
}
uint16_t check_key()
{
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}
void flush_stdin() {
    while (_kbhit()) _getch();
}

void sleep_short_time() {
    Sleep(1);
}

//Handle interrupt
int interrupted = 0;
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    interrupted = 1;
    exit(-2);
}


//Sign Extend
uint16_t sign_extend(uint16_t x, int bit_count)
    {
        if ((x >> (bit_count - 1)) & 1) {
            x |= (0xFFFF << bit_count);
        }
        return x;
    }


//Swap
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}


//Update Flags
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) 
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

//Read Image File
void read_image_file(FILE* file)
{
    /* where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}


//Read Image
int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}


//Memory access
void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}


//Executes instructions
int execute_instruction()
{
    uint16_t instr = mem_read(reg[R_PC]++);
    uint16_t op = instr >> 12;


    switch (op)
    {
        case OP_ADD:
            {
                /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;
                /* first operand (SR1) */
                uint16_t r1 = (instr >> 6) & 0x7;
                /* whether we are in immediate mode */
                uint16_t imm_flag = (instr >> 5) & 0x1;
                if (imm_flag)
                {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }
                else
                {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }

                update_flags(r0);
            }
            break;
        case OP_AND:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t imm_flag = (instr >> 5) & 0x1;
                if (imm_flag)
                {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] & imm5;
                }
                else
                {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
            }
            break;
        case OP_NOT:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[r0] = ~reg[r1];
                update_flags(r0);
            }
            break;
        case OP_BR:
            {
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                uint16_t cond_flag = (instr >> 9) & 0x7;
                if (cond_flag & reg[R_COND])
                {
                    reg[R_PC] += pc_offset;
                }
            }
            break;
        case OP_JMP:
            {
                /* Also handles RET */
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1];
            }
            break;
        case OP_JSR:
            {
                uint16_t long_flag = (instr >> 11) & 1;
                reg[R_R7] = reg[R_PC];
                if (long_flag)
                {
                    uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                    reg[R_PC] += long_pc_offset;  /* JSR */
                }
                else
                {
                    uint16_t r1 = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[r1]; /* JSRR */
                }
            }
            break;
        case OP_LD:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                reg[r0] = mem_read(reg[R_PC] + pc_offset);
                update_flags(r0);
            }
            break;
        case OP_LDI:
            {
                    /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;
                /* PCoffset 9*/
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                /* add pc_offset to the current PC, look at that memory location to get the final address */
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
            }
            break;
        case OP_LDR:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F, 6);
                reg[r0] = mem_read(reg[r1] + offset);
                update_flags(r0);
            }
            break;
        case OP_LEA:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                reg[r0] = reg[R_PC] + pc_offset;
                update_flags(r0);
            }
            break;
        case OP_ST:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                mem_write(reg[R_PC] + pc_offset, reg[r0]);
            }
            break;
        case OP_STI:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
            }
            break;
        case OP_STR:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F, 6);
                mem_write(reg[r1] + offset, reg[r0]);
            }
            break;
        case OP_TRAP:
            {
                reg[R_R7] = reg[R_PC];
                switch (instr & 0xFF)
                {
                    case TRAP_GETC:
                        {
                            if (_kbhit()) {
                                char c = _getch();

                                // Check for quit key
                                if (c == 'q' || c == 'Q' || interrupted) {
                                    restore_input_buffering();
                                    printf("\nQuit requested.\n");
                                    exit(0);  // Or break out of run loop if you prefer
                                }

                                reg[R_R0] = (uint16_t)c;
                                update_flags(R_R0);
                            } else {
                                reg[R_R0] = 0;  // No input
                                update_flags(R_R0);
                            }
                        }
                        break;
                    case TRAP_OUT:
                        {
                            putc((char)reg[R_R0], stdout);
                            fflush(stdout);
                        }
                        break;
                    case TRAP_PUTS:
                        {
                            /* one char per word */
                            uint16_t* c = memory + reg[R_R0];
                            while (*c)
                            {
                                putc((char)*c, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }
                        break;
                    case TRAP_IN:
                        {
                            printf("Enter a character: ");
                            char c = _getch();
                            putc(c, stdout);
                            fflush(stdout);
                            reg[R_R0] = (uint16_t)c;
                            update_flags(R_R0);           
                        }
                        break;
                    case TRAP_PUTSP:
                        {
                            /* one char per byte (two bytes per word)
                            here we need to swap back to
                            big endian format */
                            uint16_t* c = memory + reg[R_R0];
                            while (*c)
                            {
                                char char1 = (*c) & 0xFF;
                                putc(char1, stdout);
                                char char2 = (*c) >> 8;
                                if (char2) putc(char2, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }
                        break;
                    case TRAP_HALT:
                        {
                            puts("HALT");
                            fflush(stdout);
                            restore_input_buffering();
                            exit(0);
                        }
                        break;
                }
            }
            break;
        case OP_RES:
        case OP_RTI:
        default:
            printf("Unknown opcode: 0x%X\n", op);
            abort();
            break;
    }

    return 1;

}

void print_registers() {
    printf("\n--- REGISTERS ---\n");
    for (int i = 0; i < 8; i++) {
        printf("R%d: 0x%04X\n", i, reg[i]);
    }
    printf("PC: 0x%04X\n", reg[R_PC]);
    printf("COND: %s\n", 
        reg[R_COND] == FL_POS ? "POS" : 
        reg[R_COND] == FL_ZRO ? "ZRO" : "NEG");
}

void print_memory(uint16_t start, int count) {
    printf("\n--- MEMORY ---\n");
    for (int i = 0; i < count; i++) {
        printf("0x%04X: 0x%04X\n", start + i, memory[start + i]);
    }
}


