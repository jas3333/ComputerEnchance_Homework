#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint16_t pc;
    uint16_t opcode;
    uint8_t *buffer;
    uint8_t d_bit : 1;
    uint8_t w_bit: 1;
    uint8_t reg: 3;
    uint8_t rm: 3;
    char *source;
    char *dest;
    
} chip_t;

void disassemble(chip_t *chip, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    int filesize = ftell(file);
    rewind(file);

    chip->buffer = malloc(filesize);

    fread(chip->buffer, 1, filesize, file);
    fclose(file);

    while (chip->pc < filesize) {
        // Combine buffer[pc] + buffer[pc+1]
        chip->opcode = (chip->buffer[chip->pc] << 8) | chip->buffer[chip->pc + 1];
        chip->pc += 2;

        if ((chip->opcode >> 10) == 0b100010) {
            chip->d_bit = (chip->opcode >> 9) & 0x01; 
            chip->rm = (chip->opcode & 0x07);
            chip->reg = (chip->opcode & 0x38) >> 3;
            chip->w_bit = (chip->opcode >> 8) & 0x01;

            // printf("Opcode %06b, Dbit: %b, Wbit: %b, RegBits: %03b, R/M: %03b\n", chip->opcode, chip->d_bit, chip->w_bit, chip->reg, chip->rm);

            if (chip->d_bit == 0 && chip->w_bit == 1) {

                switch (chip->rm) {
                    case 0b000: chip->dest = "ax"; break;
                    case 0b001: chip->dest = "cx"; break;
                    case 0b010: chip->dest = "dx"; break;
                    case 0b011: chip->dest = "bx"; break;
                    case 0b100: chip->dest = "sp"; break;
                    case 0b101: chip->dest = "bp"; break;
                    case 0b110: chip->dest = "si"; break;
                    case 0b111: chip->dest = "di"; break;
                    default: break;
                }

                switch (chip->reg) {
                    case 0b000: chip->source = "ax"; break;
                    case 0b001: chip->source = "cx"; break;
                    case 0b010: chip->source = "dx"; break;
                    case 0b011: chip->source = "bx"; break;
                    case 0b100: chip->source = "sp"; break;
                    case 0b101: chip->source = "bp"; break;
                    case 0b110: chip->source = "si"; break;
                    case 0b111: chip->source = "di"; break;
                    default: break;
                }

            } else if (!chip->d_bit && !chip->w_bit) {

                switch (chip->reg) {
                    case 0b000: chip->source = "al"; break;
                    case 0b001: chip->source = "cl"; break;
                    case 0b010: chip->source = "dl"; break;
                    case 0b011: chip->source = "bl"; break;
                    case 0b100: chip->source = "ah"; break;
                    case 0b101: chip->source = "ch"; break;
                    case 0b110: chip->source = "dh"; break;
                    case 0b111: chip->source = "bh"; break;
                    default: break;
                }

                switch (chip->rm) {
                    case 0b000: chip->dest = "al"; break;
                    case 0b001: chip->dest = "cl"; break;
                    case 0b010: chip->dest = "dl"; break;
                    case 0b011: chip->dest = "bl"; break;
                    case 0b100: chip->dest = "ah"; break;
                    case 0b101: chip->dest = "ch"; break;
                    case 0b110: chip->dest = "dh"; break;
                    case 0b111: chip->dest = "bh"; break;
                    default: break;
                }
            }
            printf("mov %s, %s\n", chip->dest, chip->source);
        }
    }

    free(chip->buffer);
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
