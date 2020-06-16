#pragma once

#include <stdint.h>

#include "v2p.h"

error_t
va2pa_legacy(uint32_t virt_addr,
             const config_t *cfg,
             uint64_t *phys_addr);
