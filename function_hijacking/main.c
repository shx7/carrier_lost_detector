/**************************************************************
*
*   PURPOSE: Test for translation assembler instruction like:
*            mov %rax, 0xffff ffff .... ....,
*            movabs %rax, 0x.... .... .... ....
*            where '.' - hex data into machine code 
*            for x64 arch
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#define SIZE 9
#define ADDRESS 0xffffffffa066f000 

enum CmdType {
    CMD_MOV,
    CMD_MOVABS
};

void print_arr(unsigned char* arr, int length);

void translate_movabs(unsigned long addr, unsigned char* out_buff);
void translate_mov(unsigned long addr, unsigned char* out_buff);
void translate_jmp(unsigned char* out_buff);

void translate_into_machine_code(unsigned long addr, unsigned char* out_buff);
void _translate_into_machine_code(CmdType cmd_type, unsigned long addr, unsigned char* out_buff);

int main(int argv, char** arhc) {
    unsigned char bytes[SIZE] = {0};
    unsigned long addr = (unsigned long)(ADDRESS);
    printf("assembler code:\n mov rax, 0x%x;\n jmp rax;\n", addr);
    translate_into_machine_code(addr, bytes);
    printf("\nmachine code:\n");
    print_arr(bytes, SIZE);
};


void translate_into_machine_code(unsigned long addr, unsigned char* out_buff) {
    if (addr < 0xffffffff00000000) {
        _translate_into_machine_code(CMD_MOVABS, addr, out_buff);
    } else {
        _translate_into_machine_code(CMD_MOV, addr, out_buff);
    }
}

void _translate_into_machine_code(CmdType cmd_type,
       unsigned long addr, 
       unsigned char* out_buff) 
{
    switch (cmd_type) {
        case CMD_MOV:
            translate_mov(addr, out_buff);
            break;

        case CMD_MOVABS:
            translate_movabs(addr, out_buff);
            break;

        default:
            printf("Error: unknown unstruction\n");
            exit(-1);
    }
}

void translate_mov(unsigned long addr, unsigned char* out_buff) {
#ifdef DEBUG
    printf("translate_mov:\n");
    printf("Addr: %x\n", addr);
#endif

    // First bytes of signature MOV instruction
    out_buff[0] = 0x48;
    out_buff[1] = 0xC7;
    out_buff[2] = 0xC0; 

    for (int i = 0; i < 4; i++) {
#ifdef DEBUG
        printf("Next byte: %x\n", (addr & (0xFF << i*8)));
        printf("Mask: %x\n", (0xFF << i*8));
#endif
        out_buff[i + 3] = (unsigned char)((addr & (0xFF << i*8)) >> i*8);
    }
    translate_jmp(out_buff + 3 + 4);
}

void translate_jmp(unsigned char* out_buff) {
#ifdef DEBUG
    printf("translate_jmp:\n");
#endif 
    out_buff[0] = 0xFF;
    out_buff[1] = 0xE0; 
}

void translate_movabs(unsigned long addr, unsigned char* out_buff) {
#ifdef DEBUG
    printf("translate_movabs\n");
#endif 
    printf("feature not implemented yet\n");
}

void print_arr(unsigned char* arr, int length) {
    for (int i = 0; i < length; i++) {
        printf("%x ", arr[i]);
    }
    printf("\n");
}
