#include <stdio.h>

#include "trans.h"

void print_binary(uint32_t number) {
    if (number) {
        print_binary(number >> 1);
        putc((number & 1) ? '1' : '0', stdout);
    }
}

//111010110111100110100010101
//101110010110111111001001110001
//101110010110111111000001110100
//101110010110111111011011110000
//101110010110111111110100010101
//101110010110111111110100010101
//101110010110111111110100010101

int main() {
    uint32_t linear = 123456789;
    uint32_t cr3 = 777777777;
    uint32_t phys = 0;

    va2pa(linear, LEGACY, cr3, NULL, &phys);

    print_binary(linear);
    printf("\n");
    print_binary(cr3);
    printf("\n");
    print_binary(phys);
    printf("\n");

    return 0;
}
