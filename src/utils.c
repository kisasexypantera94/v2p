#include <cpuid.h>

#include "utils.h"

uint64_t
check_bit(const uint64_t x, const uint8_t N) {
    return x & (1U << N);
}

// Compute bit-mask with bits[l:r] set to 1
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

uint8_t
min(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

void
get_features(bool *pat, uint8_t *maxphyaddr) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    // If CPUID.01H:EDX.PAT [bit 16] = 1, the 8-entry page-attribute table (PAT) is supported.
    if (__get_cpuid(0x1, &eax, &ebx, &ecx, &edx)) {
        *pat = check_bit(edx, 16);
    }

    // CPUID.80000008H:EAX[7:0] reports the physical-address width supported by the processor.
    // This width is referred to as MAXPHYADDR.
    if (__get_cpuid(0x80000008, &eax, &ebx, &ecx, &edx)) {
        *maxphyaddr = eax & comp_mask(7, 0);
    }
}
