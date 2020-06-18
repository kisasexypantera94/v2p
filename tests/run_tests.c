#include <stdio.h>
#include <stdbool.h>

#include "test_v2p.h"
#include "test_utils.h"

void
print_binary(uint32_t number) {
    if (number) {
        print_binary(number >> 1U);
        putc((number & 1U) ? '1' : '0', stdout);
    }
}

int
main() {
    bool ok = true;
    ok &= test_comp_mask();
    ok &= test_va2pa();

    if (ok) {
        printf("OK\n");
    } else {
        printf("Errors occurred");
    }
}