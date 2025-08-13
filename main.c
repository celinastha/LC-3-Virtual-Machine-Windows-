#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
//include <unistd.h>
#include <Windows.h>
#include <conio.h> 

extern void disable_input_buffering();
extern void restore_input_buffering();
extern uint16_t check_key();
extern void handle_interrupt(int signal);
extern int interrupted;
extern uint16_t sign_extend(uint16_t x, int bit_count);
extern uint16_t swap16(uint16_t x);
extern void update_flags(uint16_t r);
extern void read_image_file(FILE* file);
extern int read_image(const char* image_path);
extern void mem_write(uint16_t address, uint16_t val);
extern uint16_t mem_read(uint16_t address);
extern int execute_instruction();
extern void print_registers();
extern void print_memory(uint16_t start, int count);
extern uint16_t reg[];
extern const int R_COND_CONST;
extern const int R_PC_CONST;
extern const int FL_ZRO_CONST;
extern void flush_stdin();
extern void sleep_short_time();


void enable_ansi_support() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}


void show_menu() {
    printf("\nWelcome to the LC-3 VM!\n");
    printf("=========================\n");
    printf("Select a program to run:\n");
    printf("1. 2048\n");
    printf("2. Exit\n");
    printf("Enter choice [1-2]: ");
}

int main(int argc, const char* argv[]) {
    int choice;
    char *program = NULL;

    while (1) {
        show_menu();
        int choice;
        char ch = _getch();
        putchar(ch);  // Echo the character
        printf("\n");
        printf("\n");


        if (!isdigit(ch)) {
            printf("\nInvalid input.\n");
            exit(1);
        }

        choice = ch - '0';  // Convert char to int

        flush_stdin();

        switch (choice) {
            case 1: program = "2048.obj"; break;
            case 2: printf("Goodbye!\n"); exit(0);
            default:
                printf("Invalid choice. Try again.\n");
                continue;
        }

        if (!read_image(program)) {
            printf("Failed to load image: %s\n", program);
            continue;
        }

        printf("Running %s...\nPress 'q' or Ctrl+C to quit!\n\n", program);

        signal(SIGINT, handle_interrupt);

        reg[R_COND_CONST] = FL_ZRO_CONST;
        enum { PC_START = 0x3000 };
        reg[R_PC_CONST] = PC_START;

        int running = 1;

        printf("LC-3 VM Ready. Commands:\n");
        printf("  s = step\n  r = run\n  p = print registers\n  m = memory visualization\n  q = quit\n");

    while (running) {
        printf("\nEnter command: ");
        char cmd = _getch();
        putchar(cmd);  // echo the key
        printf("\n");

        flush_stdin();

        switch (cmd) {
            case 's':
                disable_input_buffering();
                execute_instruction();
                restore_input_buffering();
                break;

            case 'r':
                flush_stdin();
                disable_input_buffering();
                enable_ansi_support();

                while (1) {
                    if (!execute_instruction()) {
                        printf("Program halted.\n");
                        break;
                    }
                }
                restore_input_buffering();
                break;

            case 'p':
                print_registers();  // You’ll need to define this
                break;
            case 'm':
                /* Show memory near PC */
                print_memory(reg[R_PC_CONST], 10);
                break;
            case 'q':
                restore_input_buffering();
                printf("Quitting...");
                exit(0);

            default:
                printf("Unknown command: %c\n", cmd);
                break;
        }
    }
        break;
    }
    return 0;
}