#include "utils.h"

uint64_t
check_bit(const uint64_t x, const uint8_t N) {
    return x & (1U << N);
}

uint64_t
comp_mask(const uint8_t l, const uint8_t r) {
    uint64_t a = r != 0 ? ~((1UL << r) - 1) : 0xffffffffffffffff;
    uint64_t b;
    if (l == 0) {
        b = 0x1;
    } else if (l == 63) {
        b = 0xffffffffffffffff;
    } else {
        b = (1UL << (l + 1U)) - 1;
    }
    return a & b;
}
