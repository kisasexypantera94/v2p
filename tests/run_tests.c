#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "v2p.h"
#include "utils.h"

bool
test_get_mask() {
    typedef struct {
        uint8_t l;
        uint8_t r;
        uint64_t want;
    } test_case;

    test_case t[] = {
            {31, 0,  0xffffffff},
            {31, 12, 0xfffff000},
            {31, 22, 0xffc00000},
            {0,  0,  1},
            {31, 31, 0x80000000},
            {11, 2,  0xffc},
            {63, 0,  0xffffffffffffffff},
            {63, 63, 0x8000000000000000},
            {60, 12, 0x1ffffffffffff000},
    };
    int n = sizeof(t) / sizeof(test_case);

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        uint64_t got = comp_mask(t[i].l, t[i].r);
        if (got != t[i].want) {
            printf("error: for l=%u, r=%u\ngot:  %llu\nwant: %llu\n\n", t[i].l, t[i].r, got, t[i].want);
            ok = false;
        }
    }

    return ok;
}

uint32_t mock_read_func(void *buf, const uint32_t size, const uint64_t physical_addr) {
    // set present flag
    uint64_t val = physical_addr | 1U;
    memcpy(buf, &val, size);
    return sizeof(physical_addr);
}

bool
test_va2pa() {
    typedef struct {
        uint32_t virt_addr;
        uint8_t level;
        uint32_t root_addr;
        PREAD_FUNC read_func;
        uint64_t want_phys;
    } test_case;

    test_case t[] = {
            {123456789, LEGACY, 777777777, mock_read_func, 0b101110010110111111110100010101},
    };
    int n = sizeof(t) / sizeof(test_case);

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        uint64_t phys = 0;
        va2pa(t[i].virt_addr, t[i].level, t[i].root_addr, t[i].read_func, &phys);
        if (phys != t[i].want_phys) {
            printf("error: for virt_addr=%u, root_addr=%u, level=%d\ngot:  %llu\nwant: %llu\n\n",
                   t[i].virt_addr, t[i].root_addr, t[i].level, phys, t[i].want_phys);
            ok = false;
        }
    }

    return ok;
}

void print_binary(uint32_t number) {
    if (number) {
        print_binary(number >> 1U);
        putc((number & 1U) ? '1' : '0', stdout);
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