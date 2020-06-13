#include <stdio.h>
#include <stdbool.h>

#include "v2p.c"

bool
test_get_mask() {
    typedef struct {
        uint8_t l;
        uint8_t r;
        uint32_t want;
    } test_case;

    test_case t[] = {
            {31, 0,  0xffffffff},
            {31, 12, 0xfffff000},
            {31, 22, 0xffc00000},
            {0,  0,  1},
            {31, 31, 0x80000000},
    };
    int n = sizeof(t) / sizeof(test_case);

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        uint32_t got = get_mask(t[i].l, t[i].r);
        if (got != t[i].want) {
            printf("error: for l=%u, r=%u\ngot:  %u\nwant: %u\n\n", t[i].l, t[i].r, got, t[i].want);
            ok = false;
        }
    }

    return ok;
}

bool
test_va2pa() {
    typedef struct {
        uint32_t virt_addr;
        uint8_t level;
        uint32_t root_addr;
        uint32_t want_phys;
    } test_case;

    test_case t[] = {
            {123456789, LEGACY, 777777777, 0b101110010110111111110100010101},
    };
    int n = sizeof(t) / sizeof(test_case);

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        uint32_t phys = 0;
        va2pa_legacy(t[i].virt_addr, t[i].root_addr, &phys);
        if (phys != t[i].want_phys) {
            printf("error: for virt_addr=%u, root_addr=%u\ngot:  %u\nwant: %u\n\n",
                   t[i].virt_addr, t[i].root_addr, phys, t[i].want_phys);
            ok = false;
        }
    }

    return ok;
}

void print_binary(uint32_t number) {
    if (number) {
        print_binary(number >> 1);
        putc((number & 1) ? '1' : '0', stdout);
    }
}

int main() {
    bool ok = true;
    ok &= test_get_mask();
    ok &= test_va2pa();

    if (ok) {
        printf("OK\n");
    } else {
        printf("Errors occurred");
    }
}