#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "v2p.h"

inline uint64_t
check_bit(uint64_t x, uint8_t N);

inline uint64_t
comp_mask(uint8_t l, uint8_t r);

inline uint8_t
min(uint8_t a, uint8_t b);

error_t
check_access(bool is_supervisor_addr,
             const config_t *cfg,
             uint32_t *page_fault);

void
get_features(bool *pat, uint8_t *maxphyaddr);
