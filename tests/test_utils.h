#pragma once

#include "utils.h"

bool
test_comp_mask() {
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

