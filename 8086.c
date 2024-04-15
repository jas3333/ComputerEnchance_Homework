#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint16_t opcode;
    uint8_t d_bit : 1;
    uint8_t w_bit: 1;
    uint8_t reg: 3;
    uint8_t rm: 3;
} opcode_t;

typedef struct {
    uint16_t pc;                // Program counter
    uint8_t *memory;
    char *source;
    char *dest;
    
} chip_t;


void disassemble(chip_t *chip, char *filename) {

    char *w_true[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    char *w_false[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

    opcode_t opcode = {0};

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    int filesize = ftell(file);
    rewind(file);

    chip->memory = malloc(filesize);

    fread(chip->memory, 1, filesize, file);
    fclose(file);

    while (chip->pc < filesize) {
        // Combine buffer[pc] + buffer[pc+1]
        opcode.opcode = (chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];
        chip->pc += 2;

        if ((opcode.opcode >> 10) == 0b100010) {
            opcode.d_bit = (opcode.opcode >> 9) & 0x01;
            opcode.rm = (opcode.opcode & 0x07);
            opcode.reg = (opcode.opcode & 0x38) >> 3;
            opcode.w_bit = (opcode.opcode >> 8) & 0x01;

            switch (opcode.w_bit) {
                case 0: printf("mov %s, %s\n", w_false[opcode.rm], w_false[opcode.reg]); break;
                case 1: printf("mov %s, %s\n", w_true[opcode.rm], w_true[opcode.reg]); break;
            }
        }
    }

    free(chip->memory);
}

int main (int argc, char **argv) {

    if (argc > 1) {
        chip_t chip = {0};
        disassemble(&chip, argv[1]);
    } else {
        printf("No arguments provided. Enter the filename.\n");
    }

        return 0;
}
