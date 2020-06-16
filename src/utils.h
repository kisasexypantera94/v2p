#pragma once

#include <stdint.h>
#include <stdbool.h>

inline uint64_t
check_bit(uint64_t x, uint8_t N);

inline uint64_t
comp_mask(uint8_t l, uint8_t r);

inline uint8_t
min(uint8_t a, uint8_t b);

void
get_features(bool *pat, uint8_t *maxphyaddr);
