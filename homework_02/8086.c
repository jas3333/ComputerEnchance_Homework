#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint16_t opcode;
    uint16_t op_carry;
    uint8_t d_bit : 1;
    uint8_t w_bit: 1;
    uint8_t reg: 3;
    uint8_t rm: 3;
    uint8_t mod_field: 2;
    int16_t displacement_16;
    int8_t displacement_8;
    int16_t data;
} opcode_t;

typedef struct {
    uint16_t pc;                // Program counter
    uint8_t *memory;
} chip_t;


void disassemble(chip_t *chip, char *filename) {

    char *w_true[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    char *w_false[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

    char *mod_00[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "direct", "bx"};
    char *mod_01[] = {"bx + si ", "bx + di ", "bp + si ", "bp + di", "si", "di", "bp", "bx"};
    char *mod_10[] = {"bx + si", "bx + di", "bp + si", "bp +_ di", "si", "di", "bp", "bx"};
    char *mod_11_w[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};    // w_bit = 1
    char *mod_11[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};      // w_bit = 0;

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
    if (chip->memory == NULL) {
        printf("Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    fread(chip->memory, 1, filesize, file);
    fclose(file);

    while (chip->pc < filesize) {
        opcode.opcode = (chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];
        chip->pc += 2;

        if ((opcode.opcode >> 12) == 0b1011) {
            opcode.w_bit = (opcode.opcode >> 11) & 0x01;
            opcode.reg = (opcode.opcode >> 8) & 0x07;
            int8_t immediate_value = opcode.opcode & 0xFF;

            if (opcode.w_bit == 0) {
                printf("mov %s, %d\n", w_false[opcode.reg], immediate_value);
            } else if (opcode.w_bit == 1) {
                opcode.data = ((chip->memory[chip->pc]) << 8) | (opcode.opcode & 0xFF);
                printf("mov %s, %d\n", w_true[opcode.reg], opcode.data);
                chip->pc += 1;
            } 
        } else if ((opcode.opcode >> 10) == 0b100010) {
            opcode.w_bit = (opcode.opcode >> 8) & 0x01;
            opcode.reg = (opcode.opcode & 0x38) >> 3;
            opcode.mod_field = (opcode.opcode >> 6) & 0x3;
            opcode.rm = (opcode.opcode & 0x07);
            opcode.d_bit = (opcode.opcode >> 9) & 0x01;

            if (opcode.mod_field == 0b00) {

                if (opcode.rm == 0b110) {
                    opcode.displacement_16 = (chip->memory[chip->pc+1] << 8) | chip->memory[chip->pc];
                    printf("Unimplemented: %b\n", opcode.displacement_16);
                    chip->pc += 2;

                } else {
                    printf("mov [%s]\n", mod_00[opcode.rm]);
                }
            } else if (opcode.mod_field == 0b01) {
                    opcode.displacement_8 = chip->memory[chip->pc];
                    printf("mov [%s + %d]\n", mod_01[opcode.rm], opcode.displacement_8);
                    chip->pc += 1;
            } else if (opcode.mod_field == 0b10) {
                    opcode.displacement_16 = (chip->memory[chip->pc+1] << 8) | chip->memory[chip->pc];
                    printf("mov %s, [%s + %d]\n", mod_11[opcode.reg], mod_10[opcode.rm] ,opcode.displacement_16);
                    chip->pc += 2;
            } else if (opcode.mod_field == 0b11) {
                if (opcode.w_bit == 0) {
                    printf("mov %s, %s\n", mod_11[opcode.rm], mod_11[opcode.reg]);
                } else {
                    printf("mov %s, %s\n", mod_11_w[opcode.rm], mod_11_w[opcode.reg]);
                }
            } 
        } 
    }
    // printf("\n\n");

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
