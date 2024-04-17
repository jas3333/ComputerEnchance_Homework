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
    int16_t reg_data_16;
    int8_t reg_data_8;
} opcode_t;

typedef struct {
    uint16_t pc;                // Program counter
    uint8_t *memory;
} chip_t;

// Sets w_bit, reg, mod_field, r/m register, and d_bit
void setbits_register_to_memory(opcode_t *opcode) {
    opcode->w_bit = (opcode->opcode >> 8) & 0x01;
    opcode->reg = (opcode->opcode & 0x38) >> 3;
    opcode->mod_field = (opcode->opcode >> 6) & 0x3;
    opcode->rm = (opcode->opcode & 0x07);
    opcode->d_bit = (opcode->opcode >> 9) & 0x01;
}

// Sets w_bit and reg bit
void set_immediate_to_register_bits(opcode_t *opcode) {
    opcode->w_bit = (opcode->opcode >> 11) & 0x01;
    opcode->reg = (opcode->opcode >> 8) & 0x07;

}

void disassemble(chip_t *chip, char *filename) {

    char *rm_field[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

    char *reg_w[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};    // w_bit = 1
    char *reg[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};      // w_bit = 0;

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
        /*
            move dest, source

            d_bit - 0: Source goes into reg filed, 
                    1: Dest goes in reg field
            w_bit - 0: Instruction operates on byte data(8 bits)
                    1: Instruction operates on word data(16 bits)

           Immediate to register - 
           [1011WREG][data][data if w=1]

        */
        if ((opcode.opcode >> 12) == 0b1011) {
            set_immediate_to_register_bits(&opcode);
            switch (opcode.w_bit) {
                case 0: 
                    opcode.reg_data_8 = opcode.opcode & 0xFF;                   // Get last 8 bits from instruction
                    printf("mov %s, %d\n", reg[opcode.reg], opcode.reg_data_8);
                    break;
                case 1:
                    opcode.reg_data_16 = (chip->memory[chip->pc] << 8) | (opcode.opcode & 0xFF); 
                    printf("mov %s, %d\n", reg_w[opcode.reg], opcode.reg_data_16);
                    chip->pc++;
                    break;
                default: break;
            }
        }

        /*
         Register/memory to/from register

         A lot of checks on this one spaghetti incoming. Need to check d_bit as well as w_bit.

         d_bit - determines source/dest reg
         w_bit - determines width of data, 8bit/16bit

        */
        else if ((opcode.opcode >> 10) == 0b100010) {
            setbits_register_to_memory(&opcode);
        /*
           Check modfield:
            00: Memory Mode, no displacement, unless R/M == 110 then 16 bit displament follows
            01: Memory Mode, 8bit displacement follows
            10: Memory Mode, 16 bit displacement follows 
            11: Register Mode, no displacement

           d_bit = 0 [100010dw][mod r/m reg][DISP-LO][DISP-HI]
           d_bit = 1 [100010dw][mod reg r/m][DISP-LO][DISP-HI]

        */
            switch (opcode.mod_field) {
                case 0b00: 
                    // TODO: check if R/M == 110
                    if (opcode.rm == 0b110) {
                        chip->pc += 2;
                    }
                    else if (opcode.d_bit == 0 && opcode.w_bit == 0) printf("mov [%s], %s\n", rm_field[opcode.rm], reg[opcode.reg]);
                    else if (opcode.d_bit && opcode.w_bit == 0) printf("mov %s, [%s]\n", reg[opcode.reg], rm_field[opcode.rm]);
                    else if (opcode.d_bit == 0 && opcode.w_bit == 1) printf("mov [%s], %s\n", rm_field[opcode.rm], reg_w[opcode.reg]);
                    else if (opcode.d_bit && opcode.w_bit == 1) printf("mov %s, [%s]\n", reg_w[opcode.reg], rm_field[opcode.rm]);
                    break;
                case 0b01: 
                    opcode.displacement_8 = chip->memory[chip->pc];
                    
                    if (opcode.d_bit == 0) {
                        if (opcode.displacement_8 == 0) {
                            printf("mov [%s], %s\n", rm_field[opcode.rm], reg[opcode.reg]);
                        } else {
                            printf("mov [%s + %d], %s\n", rm_field[opcode.rm], opcode.displacement_8, reg[opcode.reg]);
                        }
                    } else if (opcode.d_bit) {
                        if (opcode.displacement_8 == 0) {
                            printf("mov %s, [%s]\n", reg_w[opcode.reg], rm_field[opcode.rm]);
                        } else {
                            printf("mov %s, [%s + %d]\n", reg_w[opcode.reg], rm_field[opcode.rm], opcode.displacement_8);
                        }
                    } 
                    chip->pc += 1;
                    break;
                case 0b10:
                    opcode.displacement_16 = (chip->memory[chip->pc+1] << 8) | (chip->memory[chip->pc]);
                    if (opcode.d_bit == 0) {
                        printf("mov [%s + %d], %s\n", rm_field[opcode.rm], opcode.displacement_16, reg[opcode.reg]);
                    } else {
                        printf("mov %s, [%s + %d]\n", reg[opcode.reg], rm_field[opcode.rm], opcode.displacement_16);
                    }
                    chip->pc+=2;
                    break;
                case 0b11: 
                    if (opcode.w_bit == 0) {
                        printf("mov %s, %s\n", reg[opcode.rm], reg[opcode.reg]);
                    } else if (opcode.w_bit) {
                        printf("mov %s, %s\n", reg_w[opcode.rm], reg_w[opcode.reg]);
                    } 
                    break;

                default: break;
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
