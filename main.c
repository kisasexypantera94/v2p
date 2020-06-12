#include <stdio.h>

#include "trans.h"

void print_binary(unsigned int number) {
    if (number) {
        print_binary(number >> 1);
        putc((number & 1) ? '1' : '0', stdout);
    }
}

unsigned int
get_mask(unsigned int l, unsigned int r) {
    ++l;
    unsigned int a = (1UL << r) - 1;
    unsigned int b = r != 0 ? ~a : 0xffffffff;
    unsigned int tmp = l != 1 ? (1UL << l) - 1 : 1;
    return b & tmp;
}

int main() {
    unsigned int cr3 = 777777777;

    print_binary(cr3);
    printf("\n");
    print_binary(cr3 & PAGE_MASK);
    printf("\n");
    print_binary((cr3 & PAGE_MASK) >> 2);
    printf("\n");
    print_binary((PAGE_MASK));
    printf("\n");
    print_binary(get_mask(31, 31));
    printf("\n");
    print_binary(1);
    printf("\n");
    return 0;
}
